import React from 'react';
import { Switch, Route } from 'react-router-dom';
import Title from 'data/title';
import AppCache from 'data/cache';

import { ObjectView } from 'objects/ObjectView';
import { FileView } from 'files/FileView';
import MapHome from './MapHome';
import objectTypes from 'objects/types';
import JassView from './jass/JassView';
import MapView from './MapView';

const DataView = () => (
  <AppCache.DataContext.Consumer>
    {data => {
      let views = (
        <Switch>
          <Route path={`/:build/(${Object.keys(objectTypes).join("|")})`} component={ObjectView}/>
          <Route path="/:build/script" component={JassView}/>
          <Route path="/:build/map" component={MapView}/>
          <Route path="/:build/files" component={FileView}/>
          <Route path="/:build" exact component={MapHome}/>
        </Switch>
      );
      if (data.isMap) {
        return (
          <Title title={data.name}>
            {views};
          </Title>
        )
      } else {
        return views;
      }
    }}
  </AppCache.DataContext.Consumer>
);

export default DataView;
