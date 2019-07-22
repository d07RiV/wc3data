import React from 'react';
import classNames from 'classnames';
import pathHash, { makeUid, parseUid, equalUid } from 'data/hash';
import { Glyphicon } from 'react-bootstrap';
import { downloadBlob } from 'utils';
import SlkFile from 'mdx/parsers/slk/file';
import encoding from 'text-encoding';
import Title from 'data/title';

import FileSlkView from './FileSlk';
import FileHexView from './FileHex';
import FileTextView from './FileText';
import FileImageView from './FileImage';
import FileAudioView from './FileAudio';
import FileModelView from './FileModel';
import FileJassView from './FileJass';

import Formats from 'mdx/parsers/w3x';
import { ObjectInspector, ObjectRootLabel, ObjectLabel, ObjectName, ObjectValue } from 'react-inspector';
import ObjectPreviewEx from './ObjectPreviewEx';

const gameFileTypes = {
  "war3map.doo": Formats.doo.File,
  "war3map.imp": Formats.imp.File,
  "war3map.mmp": Formats.mmp.File,
  "war3map.shd": Formats.shd.File,
  "war3mapUnits.doo": Formats.unitsdoo.File,
  "war3map.w3c": Formats.w3c.File,
  "war3map.w3d": Formats.w3d.File,
  "war3map.w3e": Formats.w3e.File,
  "war3map.w3f": Formats.w3f.File,
  "war3map.w3i": Formats.w3i.File,
  "war3map.w3o": Formats.w3o.File,
  "war3map.w3r": Formats.w3r.File,
  "war3map.w3s": Formats.w3s.File,
  "war3map.w3u": Formats.w3u.File,
  "war3map.wct": Formats.wct.File,
  "war3map.wpm": Formats.wpm.File,
  "war3map.wtg": Formats.wtg.File,
  "war3map.wts": Formats.wts.File,
};
    
const nodeRenderer = ({ depth, name, data, isNonenumerable, expanded }) => {
  if (depth === 0) {
    if (typeof name === 'string') {
      return (
        <span>
          <ObjectName name={name} />
          <span>: </span>
          <ObjectPreviewEx data={data} />
        </span>
      );
    } else {
      return <ObjectPreviewEx data={data} />;
    }
  } else {
    return <ObjectLabel name={name} data={data} isNonenumerable={isNonenumerable} />;
  }
};

export class FileData extends React.Component {
  constructor(props) {
    super(props);
    const {id, data} = this.props;
    const state = {panel: "hex"};
    const file = data && data.archive && data.archive.loadBinary(id);
    if (file) {
      this.binary = file.data;
      this.flags = file.flags;
      if (this.flags & 1) {
        const text = new encoding.TextDecoder().decode(this.binary);
        const lines = text.split(/\r\n?|\n/);
        if (this.isJass()) {
          this.jass = lines;
          state.panel = "jass";
        } else {
          this.text = [];
          const maxLength = 2048;
          lines.forEach(line => {
            if (line.length > maxLength) {
              for (let i = 0; i < line.length; i += maxLength) {
                this.text.push(line.substr(i, maxLength));
              }
            } else {
              this.text.push(line);
            }
          });
          this.textHeights = this.text.map(() => 16);
          state.panel = "text";

          try {
            this.slk = new SlkFile(text);
            state.panel = "slk";
          } catch (e) {
          }
        }
      }
      if (this.flags & 6) {
        const blob = new Blob([this.binary], {type: (this.flags & 4) ? "audio/mpeg" : "audio/wav"});
        this.audio = URL.createObjectURL(blob);
        state.panel = "audio";
      }
      if (this.flags & 8) {
        state.panel = "model";
      }
      this.image = data.archive.loadImage(id);
      if (this.image) {
        state.panel = "image";
      }
      const name = this.getName();
      if (gameFileTypes[name]) {
        try {
          this.data = new gameFileTypes[name](this.binary.buffer);
          state.panel = "data";
        } catch (e) {
        }
      }
    }
    this.state = state;
  }

  makePanel(name, title) {
    return <li key={name} className={classNames("tab-button", {"active": this.state.panel === name})}
      onClick={() => this.setState({panel: name})}>{title}</li>;
  }


  renderPane() {
    const { panel } = this.state;
    switch (panel) {
    case "hex": return <FileHexView data={this.binary}/>;
    case "slk": return <FileSlkView data={this.slk}/>;
    case "text": return <FileTextView lines={this.text} heights={this.textHeights}/>;
    case "jass": return <FileJassView lines={this.jass}/>;
    case "audio": return <FileAudioView audio={this.audio}/>;
    case "model": return <FileModelView id={this.props.id}/>;
    case "image": return <FileImageView data={this.binary} image={this.image}/>;
    case "data": return <ObjectInspector expandLevel={1} data={this.data}/>;
    default: return null;
    }
  }

  getName() {
    let { data, id } = this.props;
    if (this.name) {
      return this.name;
    }
    const listfile = data.file("listfile.txt");
    if (listfile) {
      const names = listfile.split("\n").filter(n => n.length > 0);
      const name = names.find(path => {
        const unk = path.match(/^Unknown\\([0-9A-F]{16})/);
        const key = unk ? parseUid(unk[1]) : pathHash(path);
        return equalUid(key, id);
      });
      if (name != null) {
        const parts = name.split(/[\\/]/);
        return this.name = parts[parts.length - 1];
      }
    }
    return this.name = makeUid(id);
  }

  isJass() {
    const name = this.getName();
    if (name && name.match(/\.j$/i)) {
      return true;
    }
    const id = this.props.id;
    return equalUid(pathHash('war3map.j'), id) || equalUid(pathHash('Scripts/war3map.j'), id);
  }

  onDownload = () => {
    const blob = new Blob([this.binary], {type: "application/octet-stream"});
    downloadBlob(blob, this.getName());
  }

  render() {
    if (!this.binary) {
      return null;
    }

    return (
      <div className="FileData">
        <Title title={this.getName()}/>
        <ul className="tab-line">
          <li key="dl" className="tab-xbutton" onClick={this.onDownload}>Download <Glyphicon glyph="download-alt"/></li>
          {this.makePanel("hex", "Hex")}
          {this.text != null && this.makePanel("text", "Text")}
          {this.jass != null && this.makePanel("jass", "JASS")}
          {this.slk != null && this.makePanel("slk", "SLK")}
          {this.audio != null && this.makePanel("audio", "Audio")}
          {(this.flags & 8) !== 0 && this.makePanel("model", "Model")}
          {this.image != null && this.makePanel("image", "Image")}
          {this.data != null && this.makePanel("data", "Data")}
        </ul>
        <div className={classNames("tab-pane", this.state.panel)}>
          {this.renderPane()}
        </div>
      </div>
    );
  }
}
