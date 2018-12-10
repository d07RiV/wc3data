import React from 'react';
import { AutoSizer, List } from 'react-virtualized';

export default class FileHexView extends React.PureComponent {
  onSearch = (text, dir) => {
    if (this._list) {
      return this._list.search(text, dir);
    } else {
      return null;
    }
  }

  get digits() {
    const length = this.props.data.length;
    let numDigits = 4;
    while ((1 << (numDigits * 4)) <= length) {
      ++numDigits;
    }
    return numDigits;
  }

  rowRenderer = ({index, ...options}) => {
    const {key, style} = options;
    const offset = index * 16;
    const digits = this.digits;
    const row = [...this.props.data.subarray(offset, offset + 16)];
    const dot = ".".charCodeAt(0);
    return (
      <div className="HexRow" key={key} style={style}>
        <span className="offset">{offset.toString(16).padStart(digits, '0')}</span>
        <span className="hex">
          {row.map((byte, i) => <span key={i}>{byte.toString(16).padStart(2, '0')}</span>)}
          {row.length < 16 && [...Array(16 - row.length)].map((nil, i) => <span key={i} className="padding">{"  "}</span>)}
        </span>
        <span className="ascii">
          {String.fromCharCode(...row.map(byte => byte >= 32 && byte <= 126 ? byte : dot))}
        </span>
      </div>
    );
  }

  render() {
    const { data } = this.props;
    return (
      <AutoSizer>
        {({width, height}) => (
          <List className="HexLines"
                width={width}
                height={height}
                rowCount={Math.ceil(data.length / 16)}
                rowHeight={19}
                rowRenderer={this.rowRenderer}
            />
        )}
      </AutoSizer>
    );
  }
}
