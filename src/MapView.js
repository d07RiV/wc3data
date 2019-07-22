import React from 'react';
import AppCache from 'data/cache';
import ModelViewer from 'mdx';
import { withAsync } from 'utils';
import { AutoSizer } from 'react-virtualized';
import { Link } from 'react-router-dom';

import { vec3, mat4 } from 'gl-matrix';

import { ObjectIcon } from 'objects/ObjectCtx';
import ObjectTooltip from 'objects/Tooltip';
import pathHash from 'data/hash';

const v3pos = vec3.create(), v3dir = vec3.create(), v3up = vec3.create(), v3sub = vec3.create();
const m4rot = mat4.create();
const f32rot = new Float32Array(1);

/*
YnI3 = level 3 campaign
rde2
YiI7 = level 7 permanent

YYI/ = any item any level
YiI/ = any permanent
YnI/ = any campaign

YYI5 = any level 5
YoI5 = 5 misc
YmI5 = 5 purchasable
YlI5 = 5 artifact
YkI5 = 5 power up
YjI5 = 5 charged

\0\0\0Q = first group
ram3
\0\0\0\0 = none

\0\1\0Q = second group, pos 0
\2\0\0Q = first group, pos 2
*/

const itemGroupTypes = {
  'i': 'Permanent',
  'n': 'Campaign',
  'o': 'Miscellaneous',
  'm': 'Purchasable',
  'l': 'Artifact',
  'k': 'PowerUp',
  'j': 'Charged',
};

function computeDrops(unit, viewer) {
  const drops = [];
  function addTable({items}) {
    const output = [];
    function addItem(id, chance) {
      const item = viewer.unitsData.find(unit => unit.id === id);
      if (item && item.type === "item") {
        output.push({item, chance});
      } else if (id === '\0\0\0\0') {
        return;
      } else if (id[3] === 'Q') {
        const tableId = id.charCodeAt(1) + id.charCodeAt(2) * 256;
        const columnId = id.charCodeAt(0);
        const table = viewer.w3i.randomUnitTables.find(tbl => tbl.id === tableId);
        if (table) {
          table.units.forEach(({chance: chance2, ids}) => {
            if (ids[columnId]) {
              addItem(ids[columnId], chance * chance2 / 100);
            }
          });
        }
      } else if (id[0] === 'Y' && id[2] === 'I') {
        const type = itemGroupTypes[id[1]];
        const level = (id[3] === '/' ? -1 : parseInt(id[3]));
        const items = viewer.unitsData.filter(unit => {
          if (unit.type === 'item') {
            if (level >= 0 && parseInt(unit.data.level) !== level) {
              return false;
            }
            if (type && unit.data.class !== type) {
              return false;
            }
            return true;
          } else {
            return false;
          }
        });
        output.push({name: `Random ${level >= 0 ? 'Level ' + level : 'Any Level'} ${type ? (type === 'PowerUp' ? 'Power Up Item' : type + ' Item') : 'Item'}`, chance, items});
      }
    }
    items.forEach(({id, chance}) => addItem(id, chance));
    drops.push(output);
  }
  if (unit.droppedItemTable >= 0) {
    const table = viewer.w3i.randomItemTables.find(tbl => tbl.id === unit.droppedItemTable);
    if (table) {
      table.sets.forEach(set => addTable(set));
    }
  }
  unit.droppedItemSets.forEach(set => addTable(set));
  return drops;
}

const DropInfo = ({item}) => {
  const [expanded, setExpanded] = React.useState(false);
  const cache = React.useContext(AppCache.DataContext);
  return (
    <li>
      <ObjectTooltip id={item.item && item.item.id}>
        {item.item ? (
          <a href={`/${cache.id}/${item.item.type}/${item.item.id}`} target="_blank">
            <ObjectIcon object={item.item}/>
            {item.chance.toFixed(0)}% - {item.item.name}
          </a>
        ) : (
          <span>
            <ObjectIcon object={{'icon': pathHash('ReplaceableTextures\\WorldEditUI\\Editor-Random-Item.blp')}}/>
            {item.chance.toFixed(0)}% - {item.name}
            {item.items != null && item.items.length > 0 && <span className="listToggle" onClick={() => setExpanded(!expanded)}>{expanded ? `(hide list)` : `(${item.items.length} items)`}</span>}
          </span>
        )}
      </ObjectTooltip>
      {item.items != null && expanded && (
        <ul className="ItemDrop">
          {item.items.map(item => (
            <li>
              <ObjectTooltip id={item.id}>
                <a href={`/${cache.id}/${item.type}/${item.id}`} target="_blank">
                  <ObjectIcon object={item}/> {item.name}
                </a>
              </ObjectTooltip>
            </li>
          ))}
        </ul>
      )}
    </li>
  );
};

class SelectionWindow extends React.Component {
  coords = {x: 50, y: 50}

  static contextType = AppCache.DataContext;

  unregisterEvents() {
    document.removeEventListener('mousemove', this.onMouseMove, true);
    document.removeEventListener('mouseup', this.onMouseUp, true);
  }
  componentWillUnmount() {
    this.unregisterEvents();
  }

  onMouseDown = e => {
    this.pressed = {x: e.clientX, y: e.clientY};
    document.addEventListener('mousemove', this.onMouseMove, true);
    document.addEventListener('mouseup', this.onMouseUp, true);
  }
  onMouseMove = e => {
    const dx = e.clientX - this.pressed.x, dy = e.clientY - this.pressed.y;
    this.coords.x += dx;
    this.coords.y += dy;
    if (this.element) {
      this.element.style.left = this.coords.x + 'px';
      this.element.style.top = this.coords.y + 'px';
    }
    this.pressed = {x: e.clientX, y: e.clientY};
  }
  onMouseUp = () => {
    delete this.pressed;
    this.unregisterEvents();
  }

  render() {
    const { gameData, units, viewer, deselect } = this.props;
    const cache = this.context;
    const style = {left: this.coords.x, top: this.coords.y};
    if (!units.length) {
      style.display = "none";
    }
    const misc = gameData.getSection('Misc');
    const makeXpFunction = (xpTable, xpA, xpB, xpC) => {
      return function xpFunction(level) {
        if (level <= 0) {
          return 0;
        } if (level <= xpTable.length) {
          return xpTable[level - 1];
        } else {
          return xpA * xpFunction(level - 1) + xpB * level + xpC;
        }
      };
    };
    const xpFunction = makeXpFunction(
      misc.get('grantnormalxp').split(',').map(x => parseInt(x)),
      parseInt(misc.get('grantnormalxpformulaa')),
      parseInt(misc.get('grantnormalxpformulab')),
      parseInt(misc.get('grantnormalxpformulac'))
    );
    let totalXp = 0;
    const body = (
      <ul className="Body">
        {units.map(({unit}) => {
          const data = viewer.unitsData.find(row => row.id === unit.id);
          if (!data) {
            return null;
          }
          let dropDiv = null;
          const drops = computeDrops(unit, viewer);
          if (drops) {
            const makeDrop = table => (
              <ul className="ItemDrop">
                {table.map(item => <DropInfo item={item}/>)}
              </ul>
            );
            if (drops.length > 1) {
              dropDiv = (
                <ul className="ItemDrops">
                  {drops.map((table, idx) => (
                    <li>
                      <span>Dropped Item Set {idx + 1} (Total Chance: {table.reduce((sum, {chance}) => sum + chance, 0).toFixed(0)}%)</span>
                      {makeDrop(table)}
                    </li>
                  ))}
                </ul>
              );
            } else if (drops.length) {
              dropDiv = makeDrop(drops[0]);
            }
          }
          let levelDiv = null;
          const level = parseInt(data.data.level);
          if (!isNaN(level)) {
            const xp = xpFunction(level);
            totalXp += xp;
            levelDiv = <span>&nbsp;({xp} XP)</span>;
          }
          return (
            <li>
              <ObjectTooltip id={unit.id}>
                <a href={`/${cache.id}/${data.type}/${data.id}`} target="_blank">
                  <ObjectIcon object={data}/><span className="name">{data.name}</span> {levelDiv}
                </a>
              </ObjectTooltip>
              {dropDiv}
            </li>
          );
        })}
      </ul>
    );
    return (
      <div className="Selection" style={style} ref={n => this.element = n}>
        <div className="Heading" onMouseDown={this.onMouseDown}>Selection{totalXp > 0 ? ` (${totalXp} Total XP)` : ''}</div>
        {body}
      </div>
    );
  }
}
const AsyncSelectionWindow = withAsync({
  gameData: ({gameData}) => gameData
}, SelectionWindow);

export default class MapHome extends React.Component {
  static contextType = AppCache.DataContext;
  state = {
    notifications: [],
    selection: [],
  }

  yaw = 0;
  pitch = -Math.PI / 4;
  distance = 2000;
  center = vec3.create();
  minDistance = 500;
  maxDistance = 20000;

  constructor(props, context) {
    super(props, context);
    const dataPath = "Units\\MiscGame.txt";
    if (context.hasFile(dataPath)) {
      this.gameData = Promise.resolve(context.file(dataPath));
    } else {
      this.gameData = fetch(context.cache.binary(dataPath)).then(b => b.text());
    }
    this.gameData = this.gameData.then(text => new ModelViewer.parsers.ini.IniFile(text));
  }
  
  componentWillUnmount() {
    if (this.frame) {
      cancelAnimationFrame(this.frame);
      delete this.frame;
    }
    this.removeEvents();
  }

  onContextMenu = e => {
    e.preventDefault();
  }
  onMouseDown = e => {
    document.addEventListener("mousemove", this.onMouseMove, true);
    document.addEventListener("mouseup", this.onMouseUp, true);
    this.dragStart = this.dragPos = {x: e.clientX, y: e.clientY};
    this.dragButton = (e.ctrlKey ? 2 : (e.shiftKey ? 1 : 0));
    e.preventDefault();
  }
  onMouseMove = e => {
    if (this.dragPos && this.canvas) {
      const dx = e.clientX - this.dragPos.x,
      dy = e.clientY - this.dragPos.y;
      const dim = (this.canvas.width + this.canvas.height) / 2;
      if (this.dragButton === 2) {
        this.yaw += dx * 2 * Math.PI / dim;
        this.pitch += dy * 2 * Math.PI / dim;
        while (this.yaw > Math.PI) {
          this.yaw -= Math.PI * 2;
        }
        while (this.yaw < -Math.PI) {
          this.yaw += Math.PI * 2;
        }
        this.pitch = Math.min(this.pitch, 0);
        this.pitch = Math.max(this.pitch, -Math.PI / 2 + 0.05);
      } else if (this.dragButton === 1) {
        let rc = this.canvas.getBoundingClientRect();
        let x0 = Math.min(this.dragStart.x, this.dragPos.x) - rc.left;
        let y0 = Math.min(this.dragStart.y, this.dragPos.y) - rc.top;
        let x1 = Math.max(this.dragStart.x, this.dragPos.x) - rc.left;
        let y1 = Math.max(this.dragStart.y, this.dragPos.y) - rc.top;
        if (this.viewer) {
          this.dragSelection = this.viewer.selectUnits(x0, y0, x1, y1);
        }
        if (this.selrect) {
          this.selrect.style.display = 'block';
          this.selrect.style.left = `${x0}px`;
          this.selrect.style.top = `${y0}px`;
          this.selrect.style.width = `${x1 - x0}px`;
          this.selrect.style.height = `${y1 - y0}px`;
        }
      } else if (this.dragButton === 0) {
        const vx = vec3.fromValues(-Math.cos(this.yaw), Math.sin(this.yaw), 0);
        const vy = vec3.fromValues(vx[1], -vx[0], 0);
        const vel = this.distance / dim;
        vec3.scaleAndAdd(this.center, this.center, vx, dx * vel);
        vec3.scaleAndAdd(this.center, this.center, vy, dy * vel);
      }
      this.dragPos = {x: e.clientX, y: e.clientY};
    }
    e.preventDefault();
  }
  onMouseUp = e => {
    if (this.dragPos && this.viewer) {
      if (this.dragButton < 2 && Math.abs(this.dragPos.x - this.dragStart.x) < 5 && Math.abs(this.dragPos.y - this.dragStart.y) < 5) {
        let rc = this.canvas.getBoundingClientRect();
        let units = this.viewer.selectUnit(e.clientX - rc.left, e.clientY - rc.top, this.dragButton);
        this.setState({selection: units});
      }
      delete this.dragPos;
    }
    if (this.dragSelection) {
      this.setState({selection: this.dragSelection});
      delete this.dragSelection;
    }
    if (this.selrect) {
      this.selrect.style.display = 'none';
    }
    this.removeEvents();
    e.preventDefault();
  }
  onMouseWheel = e => {
    if (e.deltaY > 0) {
      this.distance = Math.min(this.distance * 1.2, this.maxDistance);
    } else {
      this.distance = Math.max(this.distance / 1.2, this.minDistance);
    }
  }

  removeEvents() {
    document.removeEventListener("mousemove", this.onMouseMove, true);
    document.removeEventListener("mouseup", this.onMouseUp, true);
  }

  
  animate = ts => {
    console.log(ts - this.prevTs);
    this.prevTs = ts;
    this.frame = requestAnimationFrame(this.animate);
    this.scene.camera.viewport([0, 0, this.canvas.width, this.canvas.height]);

    if (this.viewer.mapSize) {
      const size = this.viewer.mapSize, offset = this.viewer.centerOffset;
      this.center[0] = Math.max(offset[0], Math.min(this.center[0], 128 * size[0] + offset[0]));
      this.center[1] = Math.max(offset[1], Math.min(this.center[1], 128 * size[1] + offset[1]));
      if (!this.rOffsets) {
        this.rOffsets = [];
        for (let i = 0; i < 32; ++i) {
          const ang = Math.random() * Math.PI * 2;
          const dist = Math.random();
          this.rOffsets.push([Math.cos(ang) * dist, Math.sin(ang) * dist]);
        }
      }
      this.center[2] = 0;
      this.rOffsets.forEach(([x, y]) => {
        this.center[2] += this.viewer.heightAt(this.center[0] + x * this.distance / 4, this.center[1] + y * this.distance / 4);
      });
      this.center[2] /= this.rOffsets.length;
    }

    this.scene.camera.perspective(Math.PI / 4, this.canvas.width / this.canvas.height, this.minDistance / 4, this.maxDistance * 2);
    this.scene.camera.setRotationAroundAngles(this.yaw, this.pitch, this.center, this.distance);

    this.viewer.updateAndRender();
  }

  componentWillUnmount() {
    if (this.frame) {
      cancelAnimationFrame(this.frame);
      delete this.frame;
    }
    this.unmounted = true;
  }

  addNotification(text, type, expires) {
    this.setState(({notifications}) => {
      let line = {text, type};
      notifications = [...notifications, line];
      if (expires > 1000) {
        setTimeout(() => {
          if (this.unmounted) return;
          this.setState(({notifications}) => {
            let idx = notifications.indexOf(line);
            if (idx >= 0) {
              notifications = notifications.slice();
              line = {...line, expiring: true};
              notifications[idx] = line;
            }
            return {notifications};
          });
        }, expires - 1000);
      } else {
        line.expiring = true;
      }
      setTimeout(() => {
        if (this.unmounted) return;
        this.setState(({notifications}) => {
          let idx = notifications.indexOf(line);
          if (idx >= 0) {
            notifications = notifications.slice();
            notifications.splice(idx, 1);
          }
          return {notifications};
        });
      }, expires);
      return {notifications};
    });
  }
  
  componentDidMount() {
    if (!this.canvas) {
      return;
    }
    const canvas = this.canvas;

    const data = this.context;
    const resolvePath = (path, tileset) => {
      const ext = typeof path === "string" ? path.substr(path.lastIndexOf(".")).toLowerCase() : ".mdx";
      if ([".blp", ".dds", ".gif", ".jpg", ".jpeg", ".png", ".tga"].indexOf(ext) >= 0) {
        return [data.image(path, tileset), ".png", true];
      } else {
        const bin = data.binary(path);
        if (bin) {
          return [bin.data.buffer, ext, false];
        } else {
          return [data.cache.binary(path, tileset), ext, true];
        }
      }
    };

    this.viewer = new ModelViewer.viewer.handlers.w3x.MapViewer(canvas, resolvePath, data);
    this.viewer.gl.clearColor(0, 0, 0, 1);
    this.viewer.on('error', (target, error, reason) => {
      if (error === "FailedToFetch") {
        this.addNotification(`Failed to load ${target.path}`, 'error', 5000);
      }
    });
    this.viewer.on('idle', () => {
      this.addNotification('Loading complete!', 'success', 5000);
    });
    this.scene = this.viewer.scene;

    this.viewer.loadMap(data);

    this.frame = requestAnimationFrame(this.animate);
  }

  deselect = () => {
    if (this.viewer) {
      this.viewer.deselect();
    }
    this.setState({selection: []});
  }

  render() {
    const { notifications, selection } = this.state;
    return (
      <div className="ModelPage">
        <div className="FileModel">
          <AutoSizer>
            {({width, height}) => (
              <canvas
              onMouseDown={this.onMouseDown}
              onContextMenu={this.onContextMenu}
              onWheel={this.onMouseWheel}
              ref={n => this.canvas = n}
              width={Math.max(width, 100)}
              height={Math.max(height, 100)}
              />
            )}
          </AutoSizer>
          <div className="selRect" ref={n => this.selrect = n}/>
          <ul className="log">
            {notifications.map(({text, type, expiring}, idx) => <li key={idx} className={type + (expiring ? " expiring" : "")}>{text}</li>)}
          </ul>
          <div className="credits">
            Model viewer by <a href="https://github.com/flowtsohg/mdx-m3-viewer" target="_blank">flowtsohg@github</a>
          </div>
          <AsyncSelectionWindow gameData={this.gameData} units={selection} viewer={this.viewer} deselect={this.deselect}/>
        </div>
      </div>
    );
  }
}
