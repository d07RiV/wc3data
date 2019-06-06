import React from 'react';
import ReactDOM from 'react-dom';
import pathHash, { makeUid } from 'data/hash';
import FileModelView from 'files/FileModel';
import AppCache from 'data/cache';
import keycode from 'keycode';

export default class ObjectModel extends React.Component {
  state = {show: false};
  static contextType = AppCache.DataContext;

  show = e => {
    this.setState({show: true});
    e.preventDefault();
  }

  componentWillUnmount() {
    document.removeEventListener("keydown", this.onKeyDown, true);
  }

  setPopup = node => {
    this.popup = node;
    if (node) {
      document.addEventListener("keydown", this.onKeyDown, true);
    } else {
      document.removeEventListener("keydown", this.onKeyDown, true);
    }
  }

  onKeyDown = e => {
    switch (e.which) {
    case keycode.codes.esc:
      this.setState({show: false});
      break;
    default:
    }
  }

  onMouseDown = e => {
    if (e.target === this.popup) {
      this.setState({show: false});
    }
  }

  render() {
    let path = this.props.path;
    const data = this.context;
    if (path === "" || path === "_" || path === "-") {
      return path;
    }
    const m = path.match(/^(.*)(\.\w+)$/);
    if (!m) {
      path += ".mdx";
    } else if (m[2] !== ".mdx") {
      path = m[1] + ".mdx";
    }
    const hash = pathHash(path);
    const uid = makeUid(hash);

    return (
      <React.Fragment>
        {this.state.show && ReactDOM.createPortal(
          <div className="ObjectModel" onMouseDown={this.onMouseDown} ref={this.setPopup}>
            <div>
              <FileModelView path={path}/>
            </div>
          </div>,
          document.body
        )}
        <a href={`/${data.id}/files/${uid}`} onClick={this.show}>{path}</a>
      </React.Fragment>
    );
  }
}
