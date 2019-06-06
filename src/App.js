import React from 'react';
import { BrowserRouter as Router, Route, Link, Redirect } from 'react-router-dom';
import { Navbar, Modal, Button, Glyphicon, Panel } from 'react-bootstrap';
import { LinkContainer } from 'react-router-bootstrap';
import { withAsync } from './utils';
import AppCache from './data/cache';
import Title from './data/title';
import Options from './data/options';
import { notifyMessage } from 'notify';
import './App.scss';

import DataMenu from './DataMenu';
import DataView from './DataView';
import HomePage from './HomePage';

const Spinner = () => <div className="Spinner"/>;

const WithData = withAsync({
  data: ({id}, cache) => cache.data(id)
}, ({data, children}) => (
  <AppCache.DataContext.Provider value={data}>
    {children}
  </AppCache.DataContext.Provider>
), undefined, () => <Redirect to="/"/>);
WithData.contextType = AppCache.Context;

function isDropFile(e) {
  if (e.dataTransfer.items) {
    for (let i = 0; i < e.dataTransfer.items.length; ++i) {
      if (e.dataTransfer.items[i].kind === "file") {
        return true;
      }
    }
  } if (e.dataTransfer.files.length) {
    return true;
  }
  return false;
}
function getDropFile(e) {
  if (e.dataTransfer.items) {
    for (let i = 0; i < e.dataTransfer.items.length; ++i) {
      if (e.dataTransfer.items[i].kind === "file") {
        return e.dataTransfer.items[i].getAsFile();
      }
    }
  } if (e.dataTransfer.files.length) {
    return e.dataTransfer.files[0];
  }
}

class App extends React.PureComponent {
  static contextType = AppCache.Context;

  get build() {
    return this.props.match.params.build;
  }

  state = {
    loading: 0,
    dropping: 0,
  }

  onLoading = loading => {
    this.setState({loading});
  }

  componentDidMount() {
    this.context.subscribe(this.onLoading);
    document.addEventListener("drop", this.onDrop, true);
    document.addEventListener("dragover", this.onDragOver, true);
    document.addEventListener("dragenter", this.onDragEnter, true);
    document.addEventListener("dragleave", this.onDragLeave, true);
  }
  componentWillUnmount() {
    this.context.unsubscribe(this.onLoading);

    document.removeEventListener("drop", this.onDrop, true);
    document.removeEventListener("dragover", this.onDragOver, true);
    document.removeEventListener("dragenter", this.onDragEnter, true);
    document.removeEventListener("dragleave", this.onDragLeave, true);
  }

  setDropping(inc) {
    this.setState(({dropping}) => ({dropping: Math.max(dropping + inc, 0)}));
  }

  onDrop = e => {
    const file = getDropFile(e);
    if (file) {
      e.preventDefault();
      this.context.loadMap(file);
    }
    this.setState({dropping: 0});
  }
  onDragEnter = e => {
    e.preventDefault();
    this.setDropping(1);
  }
  onDragOver = e => {
    if (isDropFile(e)) {
      e.preventDefault();
    }
  }
  onDragLeave = e => {
    this.setDropping(-1);
  }

  render() {
    //const { versions, loading } = this.state;
    const build = this.build;
    if (build && !this.context.hasData(build)) {
      notifyMessage(`No data for ${build}`, "warning");
      return <Redirect to="/"/>;
    }
    return (
      <React.Fragment>
        {this.state.dropping > 0 && <div className="DropFrame"/>}
        <Navbar className="app-navbar" fluid>
          <Navbar.Header>
            <Navbar.Brand>
              <Link to="/"><span className="AppIcon"/>WC3 Data</Link>
            </Navbar.Brand>
            <Navbar.Toggle/>
          </Navbar.Header>
          {!!build && <Navbar.Collapse>
            <WithData id={build}>
              <DataMenu/>
            </WithData>
          </Navbar.Collapse>}
        </Navbar>
        {build ? (
          <WithData id={build}>
            <DataView/>
          </WithData>
        ) : (
          <HomePage/>
        )}
        {this.state.loading > 0 && <Spinner/>}
      </React.Fragment>
    );
  }
}

const AppLoader = withAsync({
  ready: (props, cache) => cache.ready,
}, App, undefined, undefined);
AppLoader.contextType = AppCache.Context;

class MapDialog extends React.Component {
  renderStep(index, name) {
    const { progress, status } = this.props;
    if (progress >= index || (status != null && status !== false)) {
      return <li className="success"><Glyphicon glyph="ok"/>{name}</li>;
    } else if (status != null) {
      return <li className="failure"><Glyphicon glyph="remove"/>{name}</li>;      
    } else if (progress === index - 1) {
      return <li className="working"><Glyphicon glyph="ok"/>{name}...</li>;
    } else {
      return <li className="pending"><Glyphicon glyph="ok"/>{name}</li>;
    }
  }
  render() {
    const { name, status, error, onHide } = this.props;
    return (
      <Modal dialogClassName="MapDialog" show={name != null} onHide={onHide}>
        <Modal.Header closeButton>
          <Modal.Title>Processing {name}</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          <ul className="load-progress">
            {this.renderStep(0, "Downloading meta data")}
            {this.renderStep(1, "Loading objects")}
            {this.renderStep(2, "Writing objects")}
            {this.renderStep(3, "Identifying files")}
            {this.renderStep(4, "Copying files")}
            {this.renderStep(5, "Finishing")}
          </ul>
          {error != null && (
            <Panel bsStyle="danger">
              <Panel.Heading><Panel.Title>{error}</Panel.Title></Panel.Heading>
            </Panel>
          )}
        </Modal.Body>
        <Modal.Footer>
          {status == null && <Button bsStyle="warning" onClick={onHide}>Cancel</Button>}
          {status != null && status !== false && (
            <LinkContainer to={`/${status}`}>
              <Button bsStyle="info" onClick={onHide}>Open</Button>
            </LinkContainer>
          )}
          {status != null && <Button bsStyle="info" onClick={onHide}>Ok</Button>}
        </Modal.Footer>
      </Modal>
    );
  }
}

class Root extends React.Component {
  cache = new AppCache(this);
  state = {
    mapLoadName: null,
    mapLoadProgress: -1,
    mapLoadStatus: null,
    mapLoadError: null,
  }

  navigateTo(url) {
    if (this.router) {
      this.router.history.push(url);
    }
  }

  beginMapLoad(name) {
    this.setState({mapLoadName: name, mapLoadProgress: -1, mapLoadStatus: null, mapLoadError: null});
  }
  onMapProgress(stage) {
    this.setState({mapLoadProgress: stage});
  }
  finishMapLoad(id) {
    this.setState({mapLoadStatus: id});
  }
  failMapLoad(error) {
    this.setState({mapLoadStatus: false, mapLoadError: error});
  }

  onCloseMapDialog = () => {
    this.cache.abortMap();
    this.setState({
      mapLoadName: null,
      mapLoadProgress: -1,
      mapLoadStatus: null,
      mapLoadError: null,
    });
  }

  render() {
    const { mapLoadName, mapLoadProgress, mapLoadStatus, mapLoadError } = this.state;
    return (
      <Router basename="/" ref={e => this.router = e}>
        <Title.Root>
          <Options>
            <AppCache.Context.Provider value={this.cache}>
              <AppCache.MapsContext.Provider value={this.cache.maps}>
                <div className="App">
                  <MapDialog name={mapLoadName} status={mapLoadStatus} progress={mapLoadProgress} error={mapLoadError} onHide={this.onCloseMapDialog}/>
                  <Route path="/:build?" component={AppLoader}/>
                </div>
              </AppCache.MapsContext.Provider>
            </AppCache.Context.Provider>
          </Options>
        </Title.Root>
      </Router>
    )
  }
}

export default Root;
