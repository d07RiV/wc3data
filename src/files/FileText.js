import React from 'react';
import { AutoSizer } from 'react-virtualized';
import { SearchBox } from 'utils';
import TextView from 'text/TextView';

export default class FileTextView extends React.PureComponent {
  onSearch = (text, dir) => {
    if (this._list) {
      return this._list.search(text, dir);
    } else {
      return null;
    }
  }

  render() {
    const { lines, heights } = this.props;
    return (
      <React.Fragment>
        <SearchBox onSearch={this.onSearch}/>
        <AutoSizer>
          {({width, height}) => (
            <TextView ref={e => this._list = e}
                      width={width}
                      height={height}
                      lines={lines}
                      heights={heights}
                      padding={6}/>
          )}
        </AutoSizer>
      </React.Fragment>
    );
  }
}
