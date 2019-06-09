import React from 'react';
import { Link } from 'react-router-dom';
import { Panel, FormGroup, FormControl, ControlLabel, HelpBlock, Glyphicon } from 'react-bootstrap';
import AppCache from 'data/cache';
import { setNotifyComponent } from './notify';

import './HomePage.scss';

const MapLink = ({id, name, desc}) => {
  name = name && name.replace(/\|(c[0-9a-fA-F]{6,8}|r)/g, "");
  desc = desc && desc.replace(/\|(c[0-9a-fA-F]{6,8}|r)/g, "");
  if (desc) {
    return <Link to={`/${id}`}>{name} <span className="desc">{desc}</span></Link>;
  } else {
    return <Link to={`/${id}`}>{name}</Link>;
  }
};

class MapList extends React.Component {
  state = {collapsed: true}
  toggle = () => {
    this.setState(({collapsed}) => ({collapsed: !collapsed}));
  }
  render() {
    const {level=0, name, items, paths, descs, Comp = "div"} = this.props;
    const {collapsed} = this.state;
    const Header = (level ? "h4" : "h3");
    const subLists = {};
    const freeItems = [];
    Object.keys(items).forEach(id => {
      let path = paths[id];
      if (!path || path.indexOf("maps/") !== 0) {
        return;
      }
      path = path.substr(5).split("~");
      if (path.length > level + 1) {
        subLists[path[level]] = (subLists[path[level]] || {});
        subLists[path[level]][id] = items[id];
      } else {
        freeItems.push(id);
      }
    });
    freeItems.sort((a, b) => items[a].localeCompare(items[b]));
    return (
      <Comp className={collapsed ? "collapsed map-list" : "map-list"}>
        <Header onClick={this.toggle}><Glyphicon glyph={collapsed ? "chevron-right" : "chevron-down"}/> {name}</Header>
        <ul>
          {Object.keys(subLists).sort().map(sub => <MapList Comp="li" key={sub} level={level+1} name={sub} items={subLists[sub]} paths={paths} descs={descs}/>)}
          {freeItems.map(id => <li key={id}><MapLink id={id} name={items[id]} desc={paths[id].match(/campaign/i) ? descs[id] : null}/></li>)}
        </ul>
      </Comp>
    );
  }
}

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
        <AppCache.MapsContext.Consumer>
          {maps => (
            <MapList name="Standard Maps" items={maps} paths={cache.custom || {}} descs={cache.customDesc || {}}/>
          )}
        </AppCache.MapsContext.Consumer>
        <h3>Custom Maps</h3>
        <AppCache.MapsContext.Consumer>
          {maps => (
            <ul>
              {Object.entries(maps).filter(([id]) => !cache.custom[id] || !cache.custom[id].match(/^maps\//))
                     .sort((a, b) => a[1].localeCompare(a[2])).map(([id, name]) => {
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
