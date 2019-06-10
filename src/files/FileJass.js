import React from 'react';
import { AutoSizer } from 'react-virtualized';
import { SearchBox } from 'utils';
import TextView from 'text/TextView';
import AppCache from 'data/cache';
import { withAsync } from 'utils';
import parseKeywords from 'jass/keywords';

const tokenRe = /([a-z_]\w*)|(0x[0-9a-z]+|\d+\.?|\d*\.\d+)|('[^'\\]*(?:\\.[^'\\]*)*')|("[^"\\]*(?:\\.[^"\\]*)*")|(\/\/.*)/gi;
const RowHeight = 16;

class FileJassInner extends React.PureComponent {
  onSearch = (text, dir) => this._list ? this._list.search(text, dir) : null;

  constructor(props) {
    super(props);
    this.keywords = parseKeywords(props.meta);
  }

  tokenFunc = (text, [all, id, num, chr, str, com]) => {
    if (id) {
      const kw = this.keywords[id];
      if (kw) {
        return <span className={kw}>{text}</span>;
      } else {
        return text;
      }
    } else if (num) {
      return <span className="jnum">{text}</span>;
    } else if (chr) {
      return <span className="jchr">{text}</span>;
    } else if (str) {
      return <span className="jstr">{text}</span>;
    } else if (com) {
      return <span className="jcom">{text}</span>;
    } else {
      return text;
    }
  };
  
  render() {
    const {lines} = this.props;
    const heights = lines.map(line => line.split("\n").length * RowHeight);
    return (
      <React.Fragment>
        <SearchBox onSearch={this.onSearch}/>
        <AutoSizer>
          {({width, height}) => (
            <TextView ref={e => this._list = e}
                    className="JassColors"
                    width={width}
                    height={height}
                    lines={lines}
                    heights={heights}
                    highlightExpr={tokenRe}
                    highlightFunc={this.tokenFunc}/>
          )}
        </AutoSizer>
      </React.Fragment>
    );
  }
}

const FileJassView = withAsync({
  meta: (props, context) => context.meta(),
}, FileJassInner, undefined, undefined);
FileJassView.contextType = AppCache.Context;

export default FileJassView;
