import React from 'react';
import { Switch, Route } from 'react-router-dom';

import { ObjectView } from 'objects/ObjectView';
import { FileView } from 'files/FileView';
import MapHome from './MapHome';
import objectTypes from 'objects/types';
import JassView from './jass/JassView';

const DataView = () => (
  <Switch>
    <Route path={`/:build/(${Object.keys(objectTypes).join("|")})`} component={ObjectView}/>
    <Route path="/:build/script" component={JassView}/>
    <Route path="/:build/files" component={FileView}/>
    <Route path="/:build" exact component={MapHome}/>
  </Switch>
);

export default DataView;
