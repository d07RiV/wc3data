import React from 'react';
import encoding from 'text-encoding';
import { Modal, Button, Radio, FormGroup, ControlLabel, FormControl, Checkbox } from 'react-bootstrap';
import objectTypes from './types';
import { listObjectData, TileSets, DestructableCategory, DoodadCategory, TechList } from './ObjectCtx';
import { downloadBlob } from 'utils';
import Options from 'data/options';

class DataCompiler {
  constructor(type, json, names) {
    this.type = type;
    this.json = json;
    this.names = names;

    if (this.json) {
      this.outJs = {};
    } else {
      this.outLines = [];
    }
  }

  onObject(id) {
    if (this.json) {
      this.curJs = {};
      this.outJs[id] = this.curJs;
    } else {
      this.outLines.push(`[${id}]`);
    }
  }

  onValue(key, value) {
    if (this.json) {
      this.curJs[key] = value;
    } else {
      this.outLines.push(`${key}=${value}`);
    }
  }

  translateSub(value, meta, data) {
    let type = meta.type;
    if (type === "lightningList") {
      type = "lightningEffect";
    }
    if (data.types[type]) {
      if (!value || value === "_") return "None";
      return data.types[type][value] || "value";
    }
    switch (type) {
    case "bool": return parseInt(value, 10) ? "True" : "False";
    case "string": return value === "_" ? "" : value;
    case "char": return value;
    case "int": return parseInt(value, 10) || 0;
    case "real":
    case "unreal": return (parseFloat(value) || 0).toFixed(meta.stringExt || 2);
    case "destructableCategory": return DestructableCategory[value] || value;
    case "tilesetList": return TileSets[value] || value;
    case "doodadCategory": return DoodadCategory[value] || value;
  
    case "techList":
      if (TechList[value]) return TechList[value];
      // fall through
    case "buffCode": case "buffList":
    case "effectCode": case "effectList":
    case "unitCode": case "unitList":
    case "abilCode": case "abilityList": case "heroAbilityList":
    case "itemCode": case "itemList":
    case "upgradeCode": case "upgradeList":
      if (value === "_") return "";
      const obj = data.objects.find(obj => obj.id === value);
      return obj ? obj.name : value;
    default:
      return value !== "_" && value || "None";
    }
  }

  translate(value, meta, data) {
    if (meta.type.indexOf("List") >= 0) {
      if (value === "_" || value === "") {
        return "";
      }
      return value.split(",").map(part => this.translateSub(part, meta, data)).join(", ");
    } else if (meta.type.indexOf("Flags") >= 0) {
      const iValue = parseInt(value, 10);
      return [...Array(31)].map((v, i) => i).filter(i => ((1 << i) & iValue) !== 0).map(flag => this.translateSub(flag, meta, data)).join(", ");
    } else if (meta.type === "string") {
      if (value === "_") {
        return "";
      }
      return value;
    } else {
      return this.translateSub(value, meta, data);
    }
  }

  processObject(obj, data) {
    this.onObject(obj.id);
    if (this.names < 2) {
      listObjectData(obj, data, this.names === 1, (key, name, value, meta) => {
        if (this.names === 0) {
          value = this.translate(value, meta, data);
        }
        this.onValue(name, value);
      });
    } else {
      Object.keys(obj.data).sort().forEach(key => this.onValue(key, obj.data[key]));
    }
  }

  process(data) {
    data.objects.forEach(obj => {
      if (this.type !== "all" && this.type !== obj.type) {
        return;
      }
      this.processObject(obj, data);
    });
  }
  processSingle(data) {
    const obj = this.type === "all" ? data.objects[0] : data.objects.find(obj => obj.type === this.type);
    if (obj) {
      this.processObject(obj, data);
    }
  }

  get output() {
    if (this.json === 0) {
      return this.outLines.join("\r\n");
    } else if (this.json === 1) {
      return JSON.stringify(this.outJs).replace(/\n/g, "\r\n");
    } else {
      return JSON.stringify(this.outJs, null, 2).replace(/\n/g, "\r\n");
    }
  }

  get preview() {
    let out;
    if (this.json === 0) {
      out = this.outLines.join("\n");
    } else if (this.json === 1) {
      out = JSON.stringify(this.outJs);
    } else if (this.json === 2) {
      out = JSON.stringify(this.outJs, null, 2);
    }
    return out.slice(0, 1000);
  }
}

export default class DataDownload extends React.PureComponent {
  static contextType = Options.Context;
  static defaultState = {type: "all", json: 0, names: 0};
  state = {}

  get options() {
    return this.context.dataDownload || DataDownload.defaultState;
  }
  setOption = (name, value) => {
    const opt = {...this.options};
    opt[name] = value;
    this.context.update("dataDownload", opt);
  }

  componentDidUpdate() {
    if (this.props.show !== this.state.show) {
      this.setState({show: this.props.show}, () => this.setOption("type", this.props.show));
    }
  }

  setType = e => this.setOption("type", e.target.value);

  onDownload = () => {
    const { type, json, names } = this.options;

    const comp = new DataCompiler(type, json, names);
    comp.process(this.props.data);

    const encoded = new encoding.TextEncoder().encode(comp.output);
    const blob = new Blob([encoded], {type: "text/plain"});
    downloadBlob(blob, type + ".txt");

    this.props.onHide();
  }

  render() {
    const { show, onHide } = this.props;
    const { type, json, names } = this.options;

    const comp = new DataCompiler(type, json, names);
    comp.processSingle(this.props.data);

    return (
      <Modal show={!!show} onHide={onHide}>
        <Modal.Header closeButton>
          <Modal.Title>Download data</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          <form>
            <FormGroup controlId="formType">
              <ControlLabel>Object type</ControlLabel>
              <FormControl componentClass="select" value={type} onChange={this.setType}>
                {Object.keys(objectTypes).map(t => <option key={t} value={t}>{objectTypes[t]}</option>)}
                <option value="all">All (Combined)</option>
              </FormControl>
            </FormGroup>
            <FormGroup>
              <Radio name="json" inline checked={json === 0} onChange={() => this.setOption("json", 0)}>
                Text
              </Radio>{' '}
              <Radio name="json" inline checked={json === 1} onChange={() => this.setOption("json", 1)}>
                JSON
              </Radio>
              <Radio name="json" inline checked={json === 2} onChange={() => this.setOption("json", 2)}>
                Indented JSON
              </Radio>
            </FormGroup>
            <FormGroup>
              <Radio name="names" inline checked={names === 0} onChange={() => this.setOption("names", 0)}>
                Editor names
              </Radio>{' '}
              <Radio name="names" inline checked={names === 1} onChange={() => this.setOption("names", 1)}>
                Code names
              </Radio>
              <Radio name="names" inline checked={names === 2} onChange={() => this.setOption("names", 2)}>
                Raw data
              </Radio>
            </FormGroup>
          </form>
          <div className="dataPreview">
            {comp.preview}
          </div>
        </Modal.Body>
        <Modal.Footer>
          <Button bsStyle="info" onClick={this.onDownload}>Download</Button>
          <Button onClick={onHide}>Cancel</Button>
        </Modal.Footer>
      </Modal>
    );
  }
}
