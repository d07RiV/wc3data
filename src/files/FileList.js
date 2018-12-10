import React from 'react';
import { Link } from 'react-router-dom';
import classNames from 'classnames';
import { FormControl } from 'react-bootstrap';
import { AutoSizer, List } from 'react-virtualized';
import pathHash from 'data/hash';
import { IdCtx } from './FileCtx';
import AppCache from 'data/cache';

import Panel from 'react-flex-panel';

const toHex = num => {
  if (num < 0) num += 4294967296;
  return num.toString(16).padStart(8, '0');
}

const processFiles = listfile => {
  const root = {
    name: "",
    dirs: {},
    files: []
  };
  const names = listfile.split("\n").filter(n => n.length > 0);
  names.forEach(path => {
    const p = path.split(/[\\/]/);
    let cd = root;
    for (let i = 0; i < p.length - 1; ++i) {
      const pl = p[i].toLowerCase();
      if (!cd.dirs[pl]) {
        cd.dirs[pl] = {
          name: p[i],
          dirs: {},
          files: []
        };
      }
      cd = cd.dirs[pl];
    }
    const unk = path.match(/^Unknown\\([0-9A-F]{8})/);
    const ext = path.match(/\.(\w{1,3})$/);
    cd.files.push({
      name: p[p.length - 1],
      path,
      key: unk ? (parseInt(unk[1], 16)|0) : pathHash(path),
      ext: ext ? ext[1].toLowerCase() : "unknown",
    });
  });
  const files = [];
  const nameCompare = (a, b) => a.name.toLowerCase().localeCompare(b.name.toLowerCase());
  const process = dir => {
    dir.dirs = Object.values(dir.dirs).sort(nameCompare);
    dir.dirs.forEach(sub => process(sub));
    dir.files.sort(nameCompare);
    files.push(...dir.files);
  };
  process(root);
  return {root, files};
}

const FileLink = ({file}) => (
  <AppCache.DataContext.Consumer>
    {data => (
      <IdCtx.Consumer>
        {id => (
          <Link to={`/${data.id}/files/${toHex(file.key)}`} className={classNames("ObjectLink", {selected: id === file.key})}>
            <span className={"Icon file-icon file-" + file.ext}/>
            <span className="ObjectName">{file.name}</span>
          </Link>
        )}
      </IdCtx.Consumer>
    )}
  </AppCache.DataContext.Consumer>
);

class FileItem {
  constructor(file) {
    this.file = file;
  }
  height = 1;
  render() {
    return <FileLink file={this.file}/>;
  }
  renderLine(index) {
    return this.render();
  }
}

class FileDirectory {
  constructor(dir, parent) {
    this.parent = parent;
    this.level = (parent ? parent.level + 1 : 0);
    this.title = dir.name;
    this.count = dir.files.length;
    this.dirs = {};
    this.children = [];
    dir.dirs.forEach(subDir => {
      const sub = new FileDirectory(subDir, this);
      this.children.push(sub);
      this.dirs[subDir.name.toLowerCase()] = sub;
      this.count += sub.count;
    });
    dir.files.forEach(subFile => {
      this.children.push(new FileItem(subFile));
    });
    this.openHeight = parent ? 1 : 0;
    this.children.forEach(c => {
      c.top = this.openHeight;
      this.openHeight += c.height;
    });
    if (!parent) {
      this.expanded = true;
    } else {
      this.expanded = false;
    }
  }

  modHeight(delta, child) {
    let index = child ? this.children.indexOf(child) : -1;
    if (index >= 0) {
      while (++index < this.children.length) {
        this.children[index].top += delta;
      }
    }
    this.openHeight += delta;
    if (this.parent && this.expanded) {
      this.parent.modHeight(delta, this);
    } else if (this.onResize) {
      this.onResize(this.height);
    }
  }

  get height() {
    return this.expanded ? this.openHeight : 1;
  }

  expandFile(file, parts) {
    if (!parts) {
      parts = file.path.split(/[\\/]/);
    }
    if (parts.length > 1) {
      const name = parts[0].toLowerCase();
      const sub = this.dirs[name];
      if (sub) {
        sub.expand();
        return sub.expandFile(file, parts.slice(1)) + sub.top;
      }
    } else {
      const sub = this.children.find(obj => obj.file === file);
      if (sub) {
        return sub.top;
      }
    }
    return 0;
  }

  expand() {
    if (!this.expanded && this.count > 0) {
      this.expanded = true;
      if (this.parent) {
        this.parent.modHeight(this.openHeight - 1, this);
      }
    }
  }

  collapse() {
    if (this.expanded && this.parent) {
      this.expanded = false;
      this.parent.modHeight(1 - this.openHeight, this);
    }
  }

  toggle = () => {
    if (this.expanded) {
      this.collapse();
    } else {
      this.expand();
    }
  }

  render() {
    if (!this.parent) {
      return null;
    }
    return (
      <div className={classNames("ObjectGroup", {expanded: this.expanded})}>
        <span className="toggle" onClick={this.toggle}/>
        <span onDoubleClick={this.toggle}><span className="Icon"/>{this.title}</span>
      </div>
    );
  }

  renderLine(index) {
    if (this.parent && !index) {
      return this.render();
    }
    if (this.expanded) {
      let left = 0, right = this.children.length - 1;
      while (left < right) {
        const mid = (left + right + 1) >> 1;
        if (this.children[mid].top > index) {
          right = mid - 1;
        } else {
          left = mid;
        }
      }
      const subIndex = index - this.children[left].top;
      const line = this.children[left].renderLine(subIndex);
      let lineStyle = null;
      if (this.parent) {
        if (left < this.children.length - 1) {
          if (subIndex === 0) {
            lineStyle = "tLine";
          } else {
            lineStyle = "line";
          }
        } else if (subIndex === 0) {
          lineStyle = "halfLine";
        }
      }
      return (
        <div className={classNames("indent", lineStyle)}>
          {line}
        </div>
      );
    }
  }
}

export class FileList extends React.PureComponent {
  state = {search: "", searchResults: null};

  onSearchKeyDown = (e) => {
    if (e.which === 27) {
      this.setState({search: "", searchResults: null});
    }
  }
  onSearch = (e) => {
    const search = e.target.value.trim();
    let found = null;
    if (search && this.files) {
      const re = new RegExp(search.replace(/[|\\{}()[\]^$+*?.]/g, "\\$&"), "i");
      found = this.files.filter(file => file.path.match(re));
    }
    this.setState({search: e.target.value, searchResults: found});
  }

  onResize = () => {
    this.forceUpdate();
  }

  rowRenderer = ({index, ...options}) => {
    const {key, style} = options;
    return (
      <div className="TreeRow" key={key} style={style}>
        {this.root.renderLine(index)}
      </div>
    );
  }
  rowRendererSearch = ({index, ...options}) => {
    const {key, style} = options;
    const file = this.state.searchResults[index];
    return (
      <div className="TreeRow" key={key} style={style}>
        <FileLink file={file}/>
      </div>
    );
  }

  render() {
    const {data, id, className, ...props} = this.props;
    const {search, searchResults} = this.state;

    if (data !== this.data_) {
      delete this.files_;
      delete this.dirs_;
      this.data_ = data;

      const list = data.file("listfile.txt");
      if (list) {
        const res = processFiles(list);
        this.files = res.files;
        this.root = new FileDirectory(res.root);
        this.root.onResize = this.onResize;
        setTimeout(() => {
          if (this._list) {
            this._list.forceUpdateGrid();
          }
        });
      }
    }
    if (!this.root) {
      return null;
    }

    if (!searchResults && this.root._id !== id) {
      const file = this.files && this.files.find(obj => obj.key === id);
      if (file) {
        this.root.onResize = null;
        const index = this.root.expandFile(file);
        this.root.onResize = this.onResize;
        this.root._id = id;
        setTimeout(() => {
          if (this._list) {
            this._list.scrollToRow(index);
          }
        });
      }
    }
    return (
      <Panel className={classNames(className, "ObjectList")} {...props}>
        <div className="search-box">
          <FormControl type="text" value={search} placeholder="Search" onKeyDown={this.onSearchKeyDown} onChange={this.onSearch}/>
        </div>
        <div className="ObjectListItems">
          <AutoSizer>
            {({width, height}) => (searchResults ?
              <List className="ObjectLines"
                    key="search"
                    ref={node => this._list2 = node}
                    width={width}
                    height={height}
                    rowCount={searchResults.length}
                    rowHeight={18}
                    rowRenderer={this.rowRendererSearch}
              /> :
              <List className="ObjectLines"
                    ref={node => this._list = node}
                    width={width}
                    height={height}
                    rowCount={this.root.height}
                    rowHeight={18}
                    rowRenderer={this.rowRenderer}
              />
            )}
          </AutoSizer>
        </div>
      </Panel>
    );
  }
}
