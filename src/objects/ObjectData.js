import React from 'react';
import { RawNames, IdCtx } from './ObjectCtx';
import Panel from 'react-flex-panel';
import classNames from 'classnames';

import Title from 'data/title';
import { ObjectValue, PopupCell } from './ObjectValue';

function nextComma(str, c) {
  if (c >= str.length) return -1;
  let instr = false;
  for (++c; c < str.length; ++c) {
    if (str[c] === '"') {
      instr = !instr;
    }
    if (str[c] === ',' && !instr) {
      return c;
    }
  }
  return c;
}

const UpgradeData = {
  "atdb": "Attack Dice Bonus - Base",
  "atdm": "Attack Dice Bonus - Increment",
  "levb": "Ability Level Bonus - Base",
  "levm": "Ability Level Bonus - Increment",
  "levc": "Ability Affected",
  "hpxb": "Hit Point Bonus - Base",
  "hpxm": "Hit Point Bonus - Increment",
  "mnxb": "Mana Point Bonus - Base",
  "mnxm": "Mana Point Bonus - Increment",
  "mvxb": "Movement Speed Bonus - Base",
  "mvxm": "Movement Speed Bonus - Increment",
  "mnrb": "Mana Regeneration Bonus (%) - Base",
  "mnrm": "Mana Regeneration Bonus (%) - Increment",
  "hpob": "Hit Point Bonus (%) - Base",
  "hpom": "Hit Point Bonus (%) - Increment",
  "manb": "Mana Point Bonus (%) - Base",
  "manm": "Mana Point Bonus (%) - Increment",
  "movb": "Movement Speed Bonus (%) - Base",
  "movm": "Movement Speed Bonus (%) - Increment",
  "atxb": "Attack Damage Bonus - Base",
  "atxm": "Attack Damage Bonus - Increment",
  "lumb": "Lumber Harvest Bonus - Base",
  "lumm": "Lumber Harvest Bonus - Increment",
  "atrb": "Attack Range Bonus - Base",
  "atrm": "Attack Range Bonus - Increment",
  "atsb": "Attack Speed Bonus (%) - Base",
  "atsm": "Attack Speed Bonus (%) - Increment",
  "spib": "Spike Damage - Base",
  "spim": "Spike Damage - Increment",
  "hprb": "Hit Point Regeneration Bonus (%) - Base",
  "hprm": "Hit Point Regeneration Bonus (%) - Increment",
  "sigb": "Sight Range Bonus - Base",
  "sigm": "Sight Range Bonus - Increment",
  "atcb": "Attack Target Count Bonus - Base",
  "atcm": "Attack Target Count Bonus - Increment",
  "adlb": "Attack Damage Loss Bonus - Base",
  "adlm": "Attack Damage Loss Bonus - Increment",
  "minb": "Gold Harvest Bonus - Base",
  "minm": "Gold Harvest Bonus - Increment",
  "raib": "Raise Dead Duration Bonus - Base",
  "raim": "Raise Dead Duration Bonus - Increment",
  "entb": "Gold Harvest Bonus (Entangle) - Base",
  "entm": "Gold Harvest Bonus (Entangle) - Increment",
  "enwb": "Attacks to Enable",
  "audb": "Aura Data Bonus - Base",
  "audm": "Aura Data Bonus - Increment",
  "asdb": "Attack Spill Distance Bonus - Base",
  "asdm": "Attack Spill Distance Bonus - Increment",
  "asrb": "Attack Spill Radius Bonus - Base",
  "asrm": "Attack Spill Radius Bonus - Increment",
  "roob": "Attacks to Enable (Rooted)",
  "urob": "Attacks to Enable (Uprooted)",
  "uart": "New Defense Type",
  "utma": "New Availability",
  "ttma": "Unit Type Affected",
};

const ObjectDataBodyEx = ({object, data, rawNames}) => {
  const type = object.type === "item" ? "unit" : object.type;
  const meta = data.meta[type] || [];

  const getName = meta => {
    if (object.type !== "upgrade") return meta.display;
    const m = meta.field.match(/^(base|mod|code)(\d)$/);
    if (!m) return meta.display;
    const effect = object.data[`effect${m[2]}`] || "";
    const m2 = effect.match(/^r([a-z][a-z][a-z])$/);
    if (!m2) return null;
    const name = UpgradeData[m2[1] + m[1][0]];
    if (!name) return null;
    return meta.display.replace("%s", name);
  };

  const getString = (col, index=-1) => {
    const text = object.data[col.toLowerCase()] || "";
    if (index < 0) {
      const cmp = nextComma(text, -1);
      if (cmp >= 2 && cmp >= text.length && text[0] === '"' && text[cmp - 1] === '"') {
        return text.substr(1, cmp - 2);
      } else {
        return text;
      }
    } else {
      let pos = -1;
      let cur = 0;
      let prev = 0;
      while ((pos = nextComma(text, pos)) >= 0) {
        if (cur === index) {
          if (pos - prev >= 2 && text[prev] === '"' && text[pos - 1] === '"') {
            return text.substr(prev + 1, pos - prev - 2);
          } else {
            return text.substr(prev, pos - prev);
          }
        }
        prev = pos + 1;
        cur++;
      }
      return "";
    }
  };
  const getInt = col => {
    col = col.toLowerCase();
    return object.data[col] && parseInt(object.data[col]) || 0;
  }

  const rows = [];

  let use = 0;
  let levels = 0;
  switch (object.type) {
  case "unit":
    if (getInt("isbldg")) {
      use = 4;
    } else if (object.id.match(/^[A-Z]/)) {
      use = 2;
    } else {
      use = 1;
    }
    break;
  case "item":
    use = 8;
    break;
  case "ability":
    levels = getInt("levels");
    if (object.data.code) {
      rows.push(
        <tr key="code">
          <PopupCell>{rawNames ? "code" : "Ability Code"}</PopupCell>
          <ObjectValue value={object.data.code} meta={{type: "abilCode"}} data={data} rawNames={rawNames}/>
        </tr>
      );
    }
    break;
  case "doodad":
    levels = getInt("numVar");
    break;
  case "upgrade":
    levels = getInt("maxlevel");
    break;
  }

  const metaNames = new Map();
  meta.forEach(row => metaNames.set(row, getName(row)));
  meta.filter(row => metaNames.get(row)).sort((a, b) => {
    if ((a.flags & 16) !== (b.flags & 16)) return (a.flags & 16) - (b.flags & 16);
    if (a.category !== b.category) return data.categories[a.category].localeCompare(data.categories[b.category]);
    return metaNames.get(a).localeCompare(metaNames.get(b));
  }).forEach(row => {
    if ((row.flags & use) !== use) {
      return;
    }
    if (row.specific && row.specific.indexOf(object.type === "ability" ? object.data.code : object.id) < 0) {
      return;
    }

    if ((row.flags & 16) && !levels) {
      return;
    }

    let field = row.field;
    if (row.data) {
      field += String.fromCharCode(65 + row.data - 1);
    }
    if (row.flags & 16) {
      for (let i = 0; i < levels; ++i) {
        let fname = field;
        if (row.index < 0) {
          if (row.flags & 32) {
            if (i > 0) fname += i;
          } else {
            fname += (i + 1);
          }
        }
        const value = (row.index < 0 ? getString(fname, -1) : getString(field, i));
        rows.push(
          <tr key={`${field}${i}`}>
            <PopupCell>{rawNames ? fname : `Level ${i + 1} - ${data.categories[row.category]} - ${metaNames.get(row)}`}</PopupCell>
            <ObjectValue value={value} meta={row} data={data} rawNames={rawNames}/>
          </tr>
        );
      }
    } else {
      const value = getString(field, row.index);
      rows.push(
        <tr key={`${field}${row.index}`}>
          <PopupCell>{rawNames ? field : `${data.categories[row.category]} - ${metaNames.get(row)}`}</PopupCell>
          <ObjectValue value={value} meta={row} data={data} rawNames={rawNames}/>
        </tr>
      );
    }
  });

  return (
    <tbody>
      {rows}
    </tbody>
  );
};

const ObjectDataBody = props => (
  <RawNames.Consumer>
    {rawNames => <ObjectDataBodyEx {...props} rawNames={rawNames}/>}
  </RawNames.Consumer>
);

class DragHandle extends React.Component {
  removeListeners() {
    document.removeEventListener("mousemove", this.onMouseMove, true);
    document.removeEventListener("mouseup", this.onMouseUp, true);
  }
  componentWillUnmount() {
    this.removeListeners();
  }

  state = {};

  onMouseDown = (e) => {
    this.setState({dragPos: e.clientX - this.props.pos});
    document.addEventListener("mousemove", this.onMouseMove, true);
    document.addEventListener("mouseup", this.onMouseUp, true);
    e.preventDefault();
    e.stopPropagation();
  }
  onMouseMove = (e) => {
    const { dragPos } = this.state;
    if ((e.buttons & 1) && dragPos != null) {
      this.props.onChange(e.clientX - dragPos);
      e.preventDefault();
      e.stopPropagation();
    } else {
      this.onMouseUp(e);
    }
  }
  onMouseUp = (e) => {
    this.setState({dragPos: null});
    this.removeListeners();
    e.preventDefault();
    e.stopPropagation();
  }

  render() {
    const {pos} = this.props;
    return <div className="drag-handle" style={{left: pos}} onMouseDown={this.onMouseDown}/>;
  }
}

export class ObjectData extends React.Component {
  state = {sliderPos: 300};

  static contextType = IdCtx;

  setSliderPos = sliderPos => {
    sliderPos = Math.max(50, sliderPos);
    if (this.table_) {
      sliderPos = Math.min(this.table_.offsetWidth - 50, sliderPos);
    }
    this.setState({sliderPos});
    return sliderPos;
  }

  setRef = e => this.table_ = e;

  render() {
    const {data, className, ...props} = this.props;
    const {sliderPos} = this.state;
    const id = this.context;
    const object = data.objects.find(obj => obj.id === id);
    if (!object) {
      return (
        <Panel className={classNames(className, "ObjectData")} {...props}>
          <div><span className="no-results">{id ? "Object not found." : "Select an object from the list."}</span></div>
        </Panel>
      );
    }

    return (
      <Panel className={classNames(className, "ObjectData")} {...props}>
        <DragHandle pos={sliderPos} onChange={this.setSliderPos}/>
        <Title title={object.name}/>
        <div className="nonscrollable">
          <table>
            <thead>
              <tr>
                <th width={sliderPos}>Name</th>
                <th>Value</th>
              </tr>
            </thead>
          </table>
        </div>
        <div className="scrollable">
          <table ref={this.setRef}>
            <thead>
              <tr>
                <th width={sliderPos}>Name</th>
                <th>Value</th>
              </tr>
            </thead>
            <ObjectDataBody object={object} data={data}/>
          </table>
        </div>
      </Panel>
    );
  }
}
