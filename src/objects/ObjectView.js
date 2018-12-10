import React from 'react';
import { Route } from 'react-router-dom';
import { RawNames, RawNamesSwitch, BuildCtx, TypeCtx, IdCtx } from './ObjectCtx';

import Panel from 'react-flex-panel';
import { withAsync } from 'utils';
import AppCache from 'data/cache';
import Options from 'data/options';

import { ObjectList } from './ObjectList';
import { ObjectData } from './ObjectData';

import './ObjectView.scss';

class ObjectViewComponent extends React.Component {
  static contextType = Options.Context;

  setRawNames = () => this.context.update("rawNames", !this.context.rawNames);

  render() {
    const {match: {params: {build, type, id}}, data} = this.props;
    return (
      <RawNames.Provider value={!!this.context.rawNames}>
        <RawNamesSwitch.Provider value={this.setRawNames}>
          <BuildCtx.Provider value={build}>
            <TypeCtx.Provider value={type}>
              <IdCtx.Provider value={id}>
                <div className="ObjectView">
                  <Panel cols>
                    <ObjectList size={300} minSize={100} resizable className="LeftPanel" data={data} type={type} id={id} key={type}/>
                    <ObjectData minSize={100} className="RightPanel" data={data}/>
                  </Panel>
                </div>
              </IdCtx.Provider>
            </TypeCtx.Provider>
          </BuildCtx.Provider>
        </RawNamesSwitch.Provider>
      </RawNames.Provider>
    );
  }
}

const ObjectViewInner = withAsync({
  data: ({match}, cache) => cache.objects()
}, ObjectViewComponent, undefined, undefined);

ObjectViewInner.contextType = AppCache.DataContext;

export const ObjectView = () => <Route path={`/:build/:type/:id?`} component={ObjectViewInner}/>;
