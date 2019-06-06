import React from 'react';
import classNames from 'classnames';
import pathHash, { makeUid, parseUid, equalUid } from 'data/hash';
import { Glyphicon } from 'react-bootstrap';
import { downloadBlob } from 'utils';

import FileHexView from './FileHex';
import FileTextView from './FileText';
import FileImageView from './FileImage';
import FileAudioView from './FileAudio';
import FileModelView from './FileModel';

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
        const text = new TextDecoder().decode(this.binary).split(/\r\n?|\n/);
        this.text = [];
        const maxLength = 2048;
        text.forEach(line => {
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
    case "text": return <FileTextView lines={this.text} heights={this.textHeights}/>;
    case "audio": return <FileAudioView audio={this.audio}/>;
    case "model": return <FileModelView id={this.props.id}/>;
    case "image": return <FileImageView data={this.binary} image={this.image}/>;
    default: return null;
    }
  }

  getName() {
    let { data, id } = this.props;
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
        return parts[parts.length - 1];
      }
    }
    return makeUid(id);
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
        <ul className="tab-line">
          <li key="dl" className="tab-xbutton" onClick={this.onDownload}>Download <Glyphicon glyph="download-alt"/></li>
          {this.makePanel("hex", "Hex")}
          {this.text != null && this.makePanel("text", "Text")}
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
