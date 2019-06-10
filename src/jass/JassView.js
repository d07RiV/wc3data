import React from 'react';
import keycode from 'keycode';
import { Link } from 'react-router-dom';
import { Navbar, Nav, NavItem, FormGroup, Glyphicon, Popover } from 'react-bootstrap';
import { AutoSizer } from 'react-virtualized';
import AppCache from 'data/cache';
import Options from 'data/options';
import { withAsync, OverlayNav, ScrollSaver, downloadBlob } from 'utils';
import parseKeywords from './keywords';
import encoding from 'text-encoding';

import TextView from 'text/TextView';

import './JassView.scss';

const tokenRe = /([a-z_]\w*)|(0x[0-9a-z]+|\d+\.?|\d*\.\d+)|('[^'\\]*(?:\\.[^'\\]*)*')|("[^"\\]*(?:\\.[^"\\]*)*")|(\/\/.*)/gi;

const RowHeight = 16;

const numOption = (setter, key, e) => {
  const val = parseInt(e.target.value, 10);
  if (!isNaN(val)) {
    setter(key, val);
  }
}

const JassOptions = ({options, setOption, ...props}) => {
  const flag = name => <input type="checkbox" checked={!!options[name]} onChange={e => setOption(name, e.target.checked ? 1 : 0)}/>;
  const number = name => <input type="number" min={1} max={10} step={1} value={options[name]} onChange={e => numOption(setOption, name, e)}/>;
  return (
    <Popover id="jass-options" {...props}>
      <form>
        <label>{flag("indentFlag")} Auto Indent</label>
        {!!options.indentFlag && (
          <label className="indent">Amount {number("indent")}</label>
        )}
        <label>{flag("restoreInt")} Restore numbers</label>
        <label>{flag("restoreId")} Restore object IDs</label>
        <label>{flag("insertLines")} Insert blank lines</label>
        <label>{flag("restoreStrings")} Restore trigger strings</label>
        <label>{flag("renameFunctionsFlag")} Rename functions</label>
        {!!options.renameFunctionsFlag && (
          <label className="indent">Max length {number("renameFunctions")}</label>
        )}
        <label>{flag("renameGlobalsFlag")} Rename globals</label>
        {!!options.renameGlobalsFlag && (
          <label className="indent">Max length {number("renameGlobals")}</label>
        )}
        <label>{flag("renameLocalsFlag")} Rename locals</label>
        {!!options.renameLocalsFlag && (
          <label className="indent">Max length {number("renameLocals")}</label>
        )}
        <label>{flag("inlineFunctions")} Inline one-line functions</label>
        <label>{flag("getObjectName")} Restore getObjectName strings</label>
      </form>
    </Popover>
  );
}

class JassViewer extends React.Component {
  state = {
    searchResults: null,
    searchText: "",
  }

  search(text, dir) {
    this.setState({
      searchText: text,
      searchResults: this._list ? this._list.search(text, dir) : null,
    });
  }
  
  findPrev = () => this.search(this.state.searchText, -1);
  findNext = () => this.search(this.state.searchText, 1);
  onSearchChange = e => this.search(e.target.value, 0);
  onSearchKeyDown = e => {
    switch (e.which) {
    case keycode.codes.enter:
      this.search(this.state.searchText, 1);
      e.preventDefault();
      break;
    case keycode.codes.esc:
      this.search("", 0);
      e.preventDefault();
      break;
    default:
    }
  }
  onKeyDown = e => {
    const { searchText } = this.state;
    switch (e.which) {
    case keycode.codes.f3:
      if (e.shiftKey) {
        this.search(searchText, -1);
      } else {
        this.search(searchText, 1);
      }
      if (this._search) {
        this._search.focus();
        this._search.select();
      }
      e.preventDefault();
      break;
    case keycode.codes.f:
      if (e.ctrlKey && this._search) {
        this._search.focus();
        this._search.select();
        e.preventDefault();
      }
      break;
    default:
    }
  }

  tokenFunc = (text, [all, id, num, chr, str, com]) => {
    const { data, keywords, objects } = this.props;
    if (id) {
      const kw = keywords[id];
      if (kw) {
        return <span className={kw}>{text}</span>;
      } else {
        return text;
      }
    } else if (num) {
      return <span className="jnum">{text}</span>;
    } else if (chr) {
      const m = chr.match(/^'(.*)'$/);
      const obj = m && objects.objects.find(o => o.id === m[1]);
      const elt = <span className="jchr">{text}</span>;
      if (obj) {
        return (
          <Link to={`/${data.id}/${obj.type}/${obj.id}`} title={obj.name}>
            {elt}
          </Link>
        );
      } else {
        return elt;
      }
    } else if (str) {
      return <span className="jstr">{text}</span>;
    } else if (com) {
      return <span className="jcom">{text}</span>;
    } else {
      return text;
    }
  };

  onDownload = () => {
    const text = this.props.lines.join("\n").replace("\n", "\r\n");
    const encoded = new encoding.TextEncoder().encode(text);
    const blob = new Blob([encoded], {type: "text/plain"});
    downloadBlob(blob, "war3map.j");
  }

  setScroll = scrollTop => {
    if (this._list) {
      this._list.setScroll(scrollTop);
    } else {
      this._scrollTop = scrollTop;
    }
  }
  setList = e => {
    this._list = e;
    if (this._scrollTop != null) {
      this._list.setScroll(this._scrollTop);
      delete this._scrollTop;
    }
  }

  render() {
    const { lines, heights, options, setOption } = this.props;
    const { searchResults, searchText } = this.state;
    const results = searchResults && searchResults.count;
    return (
      <div className="JassView" onKeyDown={this.onKeyDown}>
        <Navbar fluid className="JassHeader">
          <Nav>
            <NavItem eventKey="source" onClick={this.onDownload}>
              Download <Glyphicon glyph="download-alt"/>
            </NavItem>
            <OverlayNav trigger="click" rootClose placement="bottom" overlay={<JassOptions options={options} setOption={setOption}/>}>
              <NavItem eventKey="format">
                Formatting
              </NavItem>
            </OverlayNav>
          </Nav>
          <Navbar.Form pullLeft>
            <FormGroup>
              <div className="form-control">
                <input type="text"
                       placeholder="Search"
                       ref={node => this._search = node}
                       value={searchText}
                       onChange={this.onSearchChange}
                       onKeyDown={this.onSearchKeyDown}/>
                {!!searchResults && <span className="search-tag">{(searchResults.pos + 1) + "/" + searchResults.count}</span>}
                <button disabled={!results} onClick={this.findPrev} title="Previous"><Glyphicon glyph="chevron-up"/></button>
                <button disabled={!results} onClick={this.findNext} title="Next"><Glyphicon glyph="chevron-down"/></button>
              </div>
            </FormGroup>
          </Navbar.Form>
        </Navbar>
        <ScrollSaver onRef={e => this._saver = e} callback={this.setScroll}/>
        <div className="JassSource" ref={node => this._node = node}>
          <AutoSizer>
            {({width, height}) => (
              <TextView
                    className="JassLines"
                    ref={this.setList}
                    width={width}
                    height={height}
                    lines={lines}
                    heights={heights}
                    onScroll={this.onScroll}
                    highlightExpr={tokenRe}
                    highlightFunc={this.tokenFunc}/>
            )}
          </AutoSizer>
        </div>
      </div>
    );
  }
}

const JassViewerWithData = withAsync({
  objects: ({data}) => data.objects(),
}, JassViewer, undefined, undefined);

class JassViewParser extends React.PureComponent {
  static contextType = AppCache.DataContext;

  render() {
    const {options, setOption, meta} = this.props;
    const data = this.context;
    const {indentFlag, renameGlobalsFlag, renameFunctionsFlag, renameLocalsFlag, ...realOptions} = options;
    if (!indentFlag) realOptions.indent = 0;
    if (!renameGlobalsFlag) realOptions.renameGlobals = 0;
    if (!renameFunctionsFlag) realOptions.renameFunctions = 0;
    if (!renameLocalsFlag) realOptions.renameLocals = 0;
    const lines = data.jass(realOptions);
    const heights = lines.map(line => line.split("\n").length * RowHeight);
    const keywords = parseKeywords(meta);

    return <JassViewerWithData keywords={keywords} data={data} lines={lines} heights={heights} options={options} setOption={setOption}/>;
  }
}

class JassViewInner extends React.Component {
  static defaultState = {
    indentFlag: true,
    indent: 2,
    restoreInt: 1,
    restoreId: 1,
    insertLines: 1,
    restoreStrings: 1,
    renameGlobalsFlag: true,
    renameGlobals: 5,
    renameFunctionsFlag: true,
    renameFunctions: 5,
    renameLocalsFlag: true,
    renameLocals: 5,
    inlineFunctions: 1,
    getObjectName: 1,
  }

  static contextType = Options.Context;

  get options() {
    return this.context.jassFormatting || JassViewInner.defaultState;
  }

  setOption = (name, value) => {
    const opt = {...this.options};
    opt[name] = value;
    this.context.update("jassFormatting", opt);
  }

  render() {
    return <JassViewParser meta={this.props.meta} options={this.options} setOption={this.setOption}/>;
  }
}

const JassView = withAsync({
  meta: (props, context) => context.meta(),
}, JassViewInner, undefined, undefined);
JassView.contextType = AppCache.Context;

export default JassView;
