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
        </ul>
        <div className={classNames("tab-pane", this.state.panel)}>
          {this.renderPane()}
        </div>
      </div>
    );
  }
}
