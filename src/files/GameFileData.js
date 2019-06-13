import React from 'react';
import classNames from 'classnames';
import pathHash, { parseUid, equalUid } from 'data/hash';
import { Glyphicon } from 'react-bootstrap';
import { downloadBlob, withAsync } from 'utils';
import SlkFile from 'mdx/parsers/slk/file';
import encoding from 'text-encoding';
import Title from 'data/title';

import FileSlkView from './FileSlk';
import FileHexView from './FileHex';
import FileTextView from './FileText';
import FileImageView from './FileImage';
import FileModelView from './FileModel';
import FileJassView from './FileJass';

const GameFileImage = ({image, name}) => (
  <div className="FileData">
    <ul className="tab-line">
      <li className="tab-button active">Image</li>
    </ul>
    <div className="tab-pane image">
      <FileImageView image={image} name={name}/>
    </div>
  </div>
);

class GameFileInner extends React.Component {
  constructor(props) {
    super(props);
    const {name, data} = props;

    const state = {panel: "hex"};

    const ext = name.substr(name.lastIndexOf(".")).toLowerCase();
    if (ext === ".mdx") {
      this.model = true;
      state.panel = "model";
    } else if (ext === ".j") {
      const text = new encoding.TextDecoder().decode(data);
      this.jass = text.split(/\r\n?|\n/);
      state.panel = "jass";
    } else if (ext === ".txt" || ext === ".slk") {
      const text = new encoding.TextDecoder().decode(data);
      const lines = text.split(/\r\n?|\n/);
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
      if (ext === ".slk") {
        try {
          this.slk = new SlkFile(text);
          state.panel = "slk";
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
    case "hex": return <FileHexView data={this.props.data}/>;
    case "text": return <FileTextView lines={this.text} heights={this.textHeights}/>;
    case "jass": return <FileJassView lines={this.jass}/>;
    case "model": return <FileModelView id={this.props.id}/>;
    case "slk": return <FileSlkView data={this.slk}/>;
    default: return null;
    }
  }

  onDownload = () => {
    const blob = new Blob([this.props.data], {type: "application/octet-stream"});
    downloadBlob(blob, this.props.name);
  }

  render() {
    return (
      <div className="FileData">
        <Title title={this.props.name}/>
        <ul className="tab-line">
          <li key="dl" className="tab-xbutton" onClick={this.onDownload}>Download <Glyphicon glyph="download-alt"/></li>
          {this.makePanel("hex", "Hex")}
          {this.text != null && this.makePanel("text", "Text")}
          {this.jass != null && this.makePanel("jass", "JASS")}
          {this.slk != null && this.makePanel("slk", "SLK")}
          {!!this.model && this.makePanel("model", "Model")}
        </ul>
        <div className={classNames("tab-pane", this.state.panel)}>
          {this.renderPane()}
        </div>
      </div>
    );
  }
}

const GameFileLoader = withAsync({
  data: ({data}) => data,
}, GameFileInner, undefined, undefined);

class GameFileDataFinder extends React.PureComponent {
  constructor(props) {
    super(props);
    const {id, data, listFile} = this.props;

    const names = listFile.split("\n").filter(n => n.length > 0);
    this.name = names.find(path => {
      const unk = path.match(/^Unknown\\([0-9A-F]{16})/);
      const key = unk ? parseUid(unk[1]) : pathHash(path, true);
      return equalUid(key, id);
    });
    if (this.name != null) {
      this.name = this.name.split(/[\\/]/).pop();
      const ext = this.name.substr(this.name.lastIndexOf(".")).toLowerCase();

      if ([".blp", ".dds", ".gif", ".jpg", ".jpeg", ".png", ".tga"].indexOf(ext) >= 0) {
        this.image = data.image(id);
      } else {
        this.data = fetch(data.cache.binary(id))
          .then(response => response.ok ? response.arrayBuffer() : Promise.reject())
          .then(ab => new Uint8Array(ab));
      }
    }
  }
  render() {
    const { data, id } = this.props;
    if (this.image) {
      return <GameFileImage image={this.image} name={this.name}/>;
    } else if (this.data) {
      return <GameFileLoader data={this.data} name={this.name} id={id}/>;
    } else {
      return <span className="message">File not found</span>;
    }
  }
}

export const GameFileData = withAsync({
  listFile: ({data}) => data.listFile(),
}, GameFileDataFinder, undefined, undefined);
