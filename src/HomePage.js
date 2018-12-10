import React from 'react';
import { Link } from 'react-router-dom';
import { Panel, FormGroup, FormControl, ControlLabel, HelpBlock } from 'react-bootstrap';
import AppCache from 'data/cache';
import { setNotifyComponent } from './notify';

import './HomePage.scss';

export default class HomePage extends React.Component {
  static contextType = AppCache.Context;
  state = {message: null, messageType: "info"}

  setNotification(message, messageType) {
    this.setState({message, messageType});
  }

  componentDidMount() {
    setNotifyComponent(this);
  }
  componentWillUnmount() {
    setNotifyComponent(null);
  }

  parseFile = e => {
    const files = e.target.files;
    if (files.length > 0) {
      this.context.loadMap(files[0]);
    }
  }

  render() {
    const cache = this.context;
    return (
      <div className="HomePage">
        {this.state.message != null && (
          <Panel bsStyle={this.state.messageType}>
            <Panel.Heading><Panel.Title>{this.state.message}</Panel.Title></Panel.Heading>
          </Panel>
        )}
        <h3>Warcraft III Data</h3>
        <ul>
          {Object.entries(cache.versions).sort((a, b) => parseInt(b[0], 10) - parseInt(a[0], 10)).map(([id, name]) => (
            <li key={id}><Link to={`/${id}`}>Patch {name}</Link></li>
          ))}
        </ul>
        <h3>Custom Maps</h3>
        <AppCache.MapsContext.Consumer>
          {maps => (
            <ul>
              {Object.entries(maps).sort((a, b) => a[1].localeCompare(a[2])).map(([id, name]) => {
                let unload = null;
                if (cache.isLocal(id)) {
                  unload = <span className="unload" onClick={() => cache.unloadMap(id)}>(unload)</span>;
                }
                return <li key={id}><Link to={`/${id}`}>{name}</Link>{unload}</li>;
              })}
            </ul>
          )}
        </AppCache.MapsContext.Consumer>
        <form>
          <FormGroup controlId="loadFile">
            <ControlLabel className="linkLike">Load a custom Warcraft III map</ControlLabel>
            <FormControl style={{display: "none"}} type="file" onChange={this.parseFile} accept=".w3x,.w3m,.mpq" multiple={false} value=""/>
            <HelpBlock>
              <p>Or simply drop the map file onto the page</p>
              <p>The map will be parsed in your browser, and the data will be stored in browser cache. Nothing is sent to the server.</p>
            </HelpBlock>
          </FormGroup>
        </form>
      </div>
    );
  }
}
