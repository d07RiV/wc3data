import React from 'react';
import { RawNames, IdCtx, listObjectData } from './ObjectCtx';
import Panel from 'react-flex-panel';
import classNames from 'classnames';

import Title from 'data/title';
import { ObjectValue, PopupCell } from './ObjectValue';

const ObjectDataBodyEx = ({object, data, rawNames}) => {
  const rows = [];
  listObjectData(object, data, rawNames, (key, name, value, meta) => {
    rows.push(
      <tr key={key}>
        <PopupCell>{name}</PopupCell>
        <ObjectValue value={value} meta={meta} data={data} rawNames={rawNames}/>
      </tr>
    );
  });

  return (
    <tbody>
      {rows}
    </tbody>
  );
};

const ObjectDataBody = props => (
  <RawNames.Consumer>
    {rawNames => <ObjectDataBodyEx {...props} rawNames={rawNames}/>}
  </RawNames.Consumer>
);

class DragHandle extends React.Component {
  removeListeners() {
    document.removeEventListener("mousemove", this.onMouseMove, true);
    document.removeEventListener("mouseup", this.onMouseUp, true);
  }
  componentWillUnmount() {
    this.removeListeners();
  }

  state = {};

  onMouseDown = (e) => {
    this.setState({dragPos: e.clientX - this.props.pos});
    document.addEventListener("mousemove", this.onMouseMove, true);
    document.addEventListener("mouseup", this.onMouseUp, true);
    e.preventDefault();
    e.stopPropagation();
  }
  onMouseMove = (e) => {
    const { dragPos } = this.state;
    if ((e.buttons & 1) && dragPos != null) {
      this.props.onChange(e.clientX - dragPos);
      e.preventDefault();
      e.stopPropagation();
    } else {
      this.onMouseUp(e);
    }
  }
  onMouseUp = (e) => {
    this.setState({dragPos: null});
    this.removeListeners();
    e.preventDefault();
    e.stopPropagation();
  }

  render() {
    const {pos} = this.props;
    return <div className="drag-handle" style={{left: pos}} onMouseDown={this.onMouseDown}/>;
  }
}

export class ObjectData extends React.Component {
  state = {sliderPos: 300};

  static contextType = IdCtx;

  setSliderPos = sliderPos => {
    sliderPos = Math.max(50, sliderPos);
    if (this.table_) {
      sliderPos = Math.min(this.table_.offsetWidth - 50, sliderPos);
    }
    this.setState({sliderPos});
    return sliderPos;
  }

  setRef = e => this.table_ = e;

  render() {
    const {data, className, ...props} = this.props;
    const {sliderPos} = this.state;
    const id = this.context;
    const object = data.objects.find(obj => obj.id === id);
    if (!object) {
      return (
        <Panel className={classNames(className, "ObjectData")} {...props}>
          <div><span className="no-results">{id ? "Object not found." : "Select an object from the list."}</span></div>
        </Panel>
      );
    }

    return (
      <Panel className={classNames(className, "ObjectData")} {...props}>
        <DragHandle pos={sliderPos} onChange={this.setSliderPos}/>
        <Title title={object.name}/>
        <div className="nonscrollable">
          <table>
            <thead>
              <tr>
                <th width={sliderPos}>Name</th>
                <th>Value</th>
              </tr>
            </thead>
          </table>
        </div>
        <div className="scrollable">
          <table ref={this.setRef}>
            <thead>
              <tr>
                <th width={sliderPos}>Name</th>
                <th>Value</th>
              </tr>
            </thead>
            <ObjectDataBody object={object} data={data}/>
          </table>
        </div>
      </Panel>
    );
  }
}
