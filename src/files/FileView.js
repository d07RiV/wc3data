import React from 'react';
import { Route } from 'react-router-dom';
import { IdCtx } from './FileCtx';

import Panel from 'react-flex-panel';
import AppCache from 'data/cache';

import { parseUid } from 'data/hash';

import { FileList } from './FileList';
import { FileData } from './FileData';
import { GameFileData } from './GameFileData';

import './FileView.scss';

class FileViewComponent extends React.Component {
  static contextType = AppCache.DataContext;

  render() {
    const data = this.context;
    const uid = this.props.match.params.id;
    const key = uid == null ? null : parseUid(uid);
    return (
      <IdCtx.Provider value={key}>
        <div className="FileView">
          <Panel cols>
            <FileList size={300} minSize={100} resizable className="LeftPanel" data={data} id={key}/>
            <Panel className="RightPanel" minSize={100}>
              {key == null ? (
                <span className="message">Select a file to view its contents</span>
              ) : (data.hasFile(key) ? (
                <FileData data={data} id={key} key={uid}/>
              ) : (data.archive ? (
                <span className="message">File not found</span>
              ) : (
                <GameFileData data={data} id={key} key={uid}/>
              )))}
            </Panel>
          </Panel>
        </div>
      </IdCtx.Provider>
    );
  }
}

export const FileView = () => <Route path={`/:build/files/:id?`} component={FileViewComponent}/>;
