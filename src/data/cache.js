import React from 'react';
import { Cache } from 'utils';
import pathHash, { makeUid } from './hash';
import loadArchive from 'maps/archive';
import MapParser from 'maps/parser';
import { notifyMessage } from 'notify';
import IdbKvStore from 'idb-kv-store';

const postProcess = data => {
  Object.values(data.objects).forEach(obj => {
    const repl = {};
    Object.keys(obj.data).forEach(key => repl[key.toLowerCase()] = obj.data[key]);
    obj.data = repl;
  });
  return data;
};

const readFile = file => new Promise((resolve, reject) => {
  const reader = new FileReader();
  reader.onload = () => resolve(reader.result);
  reader.onerror = () => reject(reader.error);
  reader.onabort = () => reject();
  reader.readAsArrayBuffer(file);
});

class BaseData {
  constructor(cache, build, name) {
    this.cache = cache;
    this.id = build;
    this.name = name;
    this.core = true;
  }

  objects() {
    return this.cache.fetch(`/api/${this.id}.json`, {global: true, process: postProcess});
  }

  listFile() {
    return this.cache.fetch(`/api/rootlist.txt`, {global: true, type: "text"});
  }

  hasFile() {
    return false;
  }

  file() {
    return null;
  }
  binary() {
    return null;
  }
  image(name) {
    return this.cache.image(name);
  }

  jass() {
    return null;
  }

  icon(id) {
    return this.cache.icon(id);
  }
  iconByName(name) {
    return this.cache.iconByName(name);
  }
}

const fileId = name => {
  if (typeof name === "string") {
    return pathHash(name);
  } else if (name != null && typeof name[0] === "number") {
    return name;
  } else {
    return null;
  }
};

class MapData {
  files_ = {};
  images_ = {};

  constructor(cache, archive, id, name) {
    this.cache = cache;
    this.archive = archive;
    this.id = id;
    this.name = name;
    this.isMap = true;
  }

  objects() {
    if (this.objects_) {
      return this.objects_;
    }
    const text = this.archive.loadFile("objects.json");
    return this.objects_ = Promise.resolve(text ? postProcess(JSON.parse(text)) : null);
  }

  listFile() {
    return this.file("listfile.txt");
  }

  hasFile(name) {
    return this.archive.hasFile(fileId(name));
  }

  file(name) {
    if (this.files_.hasOwnProperty(name)) {
      return this.files_[name];
    }
    return this.files_[name] = this.archive.loadFile(name);
  }
  binary(name) {
    return this.archive.loadBinary(name);
  }

  jass(options) {
    return this.archive.loadJASS(options);
  }

  image(name) {
    return this.archive.loadImage(name) || this.cache.image(name);
  }

  icon(id) {
    const image = this.archive.loadImage(id);
    if (image) {
      return {
        backgroundImage: `url(${image})`,
        backgroundSize: "100%",
      };
    } else {
      return this.cache.icon(id);
    }
  }

  iconByName(name) {
    return this.icon(pathHash(name));
  }
}

export default class AppCache extends Cache {
  static Context = React.createContext(null);
  static DataContext = React.createContext(null);
  static MapsContext = React.createContext({});

  constructor(root) {
    super();
    this.root = root;

    try {
      this.dataStore = new IdbKvStore("mapData");
      this.nameStore = new IdbKvStore("mapNames");
    } catch (e) {
      this.dataStore = null;
      this.nameStore = null;
    }

    const proms = [
      this.fetch('/api/images.dat', {type: "binary"}),
      this.fetch('/api/versions.json'),
    ];
    if (this.nameStore) {
      proms.push(this.nameStore.json());
    }
    this.ready = Promise.all(proms).then(([images, versions, names]) => {
      this.icons_ = new Map();
      const imageList = new Uint32Array(images);
      for (let i = 0; i < imageList.length; i += 2) {
        // this will cause precision loss but we have to deal with it
        const uid = makeUid(imageList.subarray(i, i + 2));
        this.icons_.set(uid, (i / 2) + 1000 * 256);
      }
  
      this.versions = versions.versions;

      if (names) {
        this.maps = names;
        this.root.forceUpdate();
      }

      if (versions.custom) {
        this.custom = {};
        Object.entries(versions.custom).forEach(([id, info]) => {
          this.maps[id] = info.name;
          this.custom[id] = info.data;
        });
      }
    });
  }

  baseData_ = {};
  mapNames_ = {};
  mapData_ = {};
  maps = {};

  metaRaw() {
    return this.fetch('/api/meta.gzx', {type: "binary", global: true});
  }
  meta() {
    if (this.meta_) {
      return this.meta_;
    }
    return this.meta_ = this.metaRaw().then(data => loadArchive(data));
  }

  isLocal(build) {
    return !!(this.maps[build] && !this.custom[build]);
  }

  data(build) {
    if (this.maps[build]) {
      if (this.mapData_[build]) {
        return this.mapData_[build];
      }
      if (this.custom && this.custom[build]) {
        return this.mapData_[build] = this.fetch(`/api/${this.custom[build]}`, {type: "binary", global: true})
          .then(data => loadArchive(data))
          .then(arc => new MapData(this, arc, build, this.maps[build]));
      } else {
        return this.mapData_[build] = this.dataStore.get(build)
          .then(blob => readFile(blob))
          .then(data => loadArchive(data))
          .then(arc => new MapData(this, arc, build, this.maps[build]))
          .catch(err => Promise.reject(notifyMessage(err.toString(), "danger")));
      }
    } else if (this.versions[build]) {
      if (this.baseData_[build]) {
        return this.baseData_[build];
      }
      return this.baseData_[build] = Promise.resolve(new BaseData(this, build, this.versions[build]));
    }
    return null;
  }
  hasData(build) {
    return !!(this.maps[build] || this.versions[build]);
  }

  icon(id) {
    const index = this.icons_.get(makeUid(id));
    if (index) {
      const col = index % 16;
      const row = ((index / 16) | 0) % 16;
      const image = (index / 256) | 0;
      return {
        backgroundImage: `url(/api/icons${image}.png)`,
        backgroundPosition: `-${col * 16}px -${row * 16}px`,
      };
    } else {
      return null;
    }
  }
  iconByName(name) {
    return this.icon(pathHash(name));
  }

  image(name) {
    let id = fileId(name);
    const uid = makeUid(id);
    return `/api/images/${uid}`;
  }

  binary(name) {
    let id = fileId(name);
    const uid = makeUid(id);
    return `/api/files/${uid}`;
  }

  loadMap(file) {
    this.abortMap();
    const meta = this.metaRaw();
    readFile(file).then(map => {
      this.parser = new MapParser();
      this.parser.onProgress = stage => this.root.onMapProgress(stage);
      this.root.beginMapLoad(file.name);
      this.parser.parse(meta, map)
        .then(data => {
          delete this.parser;

          let id = 1;
          while (this.maps.hasOwnProperty(`map${id}`)) {
            id += 1;
          }
          id = `map${id}`;
    
          if (this.dataStore) {
            Promise.all([
              this.nameStore.set(id, file.name),
              this.dataStore.set(id, new Blob([data], {type: "application/octet-stream"}))
            ]).catch(() => {
              this.nameStore.remove(id);
              this.dataStore.remove(id);
            });
          }

          this.maps = {...this.maps, [id]: file.name};
          this.mapData_[id] = loadArchive(data).then(arc => new MapData(this, arc, id, file.name))
            .catch(err => Promise.reject(notifyMessage(err.toString(), "danger")));

          this.root.finishMapLoad(id);
        })
        .catch(err => {
          delete this.parser;

          this.root.failMapLoad(typeof err === "string" ? err : false);
          console.error(err);
        });
    });
  }
  abortMap() {
    if (this.parser) {
      this.parser.terminate();
      delete this.parser;
    }
  }

  unloadMap(id) {
    if (this.maps[id]) {
      if (window.confirm(`Are you sure you want to unload ${this.maps[id]}?`)) {
        this.maps = {...this.maps};
        delete this.maps[id];
        delete this.mapData_[id];
        if (this.dataStore) {
          this.dataStore.remove(id);
          this.nameStore.remove(id);
        }
        this.root.forceUpdate();
      }
    }
  }
}
