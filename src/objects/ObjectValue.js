import React from 'react';
import { Link } from 'react-router-dom';
import { BuildCtx, ObjectIcon, TileSets, DestructableCategory, DoodadCategory, TechList } from './ObjectCtx';
import { Popover, OverlayTrigger } from 'react-bootstrap';
import ObjectTooltip from './Tooltip';
import classNames from 'classnames';
import AppCache from 'data/cache';
import tagString from 'data/tagString';
import ObjectModel from './ObjectModel';

const ObjectLink = ({object}) => (
  <BuildCtx.Consumer>
    {build => <ObjectTooltip id={object.id}><Link to={`/${build}/${object.type}/${object.id}`}><ObjectIcon object={object}/>{object.name}</Link></ObjectTooltip>}
  </BuildCtx.Consumer>
);

const ObjectLinkRaw = ({object}) => (
  <BuildCtx.Consumer>
    {build => <ObjectTooltip id={object.id}><Link to={`/${build}/${object.type}/${object.id}`}>{object.id}</Link></ObjectTooltip>}
  </BuildCtx.Consumer>
);

class StringPopupCell extends React.PureComponent {
  state = {popup: null};

  onMouseOver = e => {
    const rect = e.target.getBoundingClientRect();
    const height = Math.max(document.documentElement.clientHeight, window.innerHeight || 0);
    if (rect.top + rect.bottom > height) {
      this.setState({popup: "top"});
    } else {
      this.setState({popup: "bottom"});
    }
  }
  onMouseOut = () => {
    this.setState({popup: null});
  }

  render() {
    const {children, tooltip} = this.props;
    const {popup} = this.state;

    return popup ? (
      <td className={classNames("popupStringCell", popup)} onMouseEnter={this.onMouseOver} onMouseLeave={this.onMouseOut}>
        {tooltip}
        {children}
      </td>
    ) : (
      <td onMouseEnter={this.onMouseOver} onMouseLeave={this.onMouseOut}>{children}</td>
    );
  }
}

export class PopupCell extends React.PureComponent {
  state = {popup: false};

  onMouseOver = e => {
    if (e.target.scrollWidth >= e.target.offsetWidth) {
      if (e.target.closest(".ObjectModel")) {
        return;
      }
      this.setState({popup: true});
    }
  }
  onMouseOut = () => {
    this.setState({popup: false});
  }

  render() {
    const {children} = this.props;
    const {popup} = this.state;

    return popup ? (
      <td className="popupCell" onMouseEnter={this.onMouseOver} onMouseLeave={this.onMouseOut}>
        <div>{children}</div>#
      </td>
    ) : (
      <td onMouseEnter={this.onMouseOver} onMouseLeave={this.onMouseOut}>{children}</td>
    );
  }
}

const StringPopup = ({value, data}) => {
  const result = tagString(value, data);
  return <div>{result}</div>;
};

function formatString(str, data) {
  if (str.match(/<.*>|\|[crn]/i)) {
    return (
      <StringPopupCell tooltip={<StringPopup value={str} data={data}/>}>
        {str}
      </StringPopupCell>
    );
  } else {
    return <PopupCell>{str}</PopupCell>;
  }
}

class StringIconPopup extends React.Component {
  state = {visible: false};
  onLoad = () => this.setState({visible: true});

  render() {
    const {path, cache, className, ...props} = this.props;
    const image = cache.image(path);
    if (image == null) {
      return null;
    }
    return (
      <Popover id="icon-preview" {...props} className={classNames(className, {loading: !this.state.visible})}>
        <img src={image} onLoad={this.onLoad} alt="icon"/>
      </Popover>
    );
  }
}

const StringIcon = ({path}) => (
  <AppCache.DataContext.Consumer>
    {cache => {
      const icon = cache.iconByName(path);
      return (
        <OverlayTrigger placement="top" overlay={<StringIconPopup path={path} cache={cache}/>}>
          <span>
            {icon != null && <span className="Icon" style={icon}/>}
            {path}
          </span>
        </OverlayTrigger>
      );
    }}
  </AppCache.DataContext.Consumer>
);

const ObjectSubValue = ({value, meta, data}) => {
  if (meta.type.indexOf("Flags") >= 0) {
    debugger;
  }
  let type = meta.type;
  if (type === "lightningList") {
    type = "lightningEffect";
  }
  if (data.types[type]) {
    if (!value || value === "_") return "None";
    return data.types[type][value] || "value";
  }
  switch (type) {
  case "bool": return parseInt(value, 10) ? "True" : "False";
  case "string": return value === "_" ? "" : value;
  case "char": return value;
  case "int": return parseInt(value, 10) || 0;
  case "real":
  case "unreal": return (parseFloat(value) || 0).toFixed(meta.stringExt || 2);
  case "destructableCategory": return DestructableCategory[value] || value;
  case "tilesetList": return TileSets[value] || value;
  case "doodadCategory": return DoodadCategory[value] || value;

  case "techList":
    if (TechList[value]) return TechList[value];
    // fall through
  case "buffCode": case "buffList":
  case "effectCode": case "effectList":
  case "unitCode": case "unitList":
  case "abilCode": case "abilityList": case "heroAbilityList":
  case "itemCode": case "itemList":
  case "upgradeCode": case "upgradeList":
    if (value === "_") return "";
    const obj = data.objects.find(obj => obj.id === value);
    return obj ? <ObjectLink object={obj}/> : value;
  case "icon":
    return <StringIcon path={value}/>;
  case "model":
    return <ObjectModel path={value}/>;
  default:
    return value !== "_" && value || "None";
  }
};

const ObjectSubValueRaw = ({value, meta, data}) => {
  let type = meta.type;
  switch (type) {
  case "int": return parseInt(value, 10) || 0;
  case "real":
  case "unreal": return (parseFloat(value) || 0).toFixed(meta.stringExt || 2);
  case "techList":
  case "buffCode": case "buffList":
  case "effectCode": case "effectList":
  case "unitCode": case "unitList":
  case "abilCode": case "abilityList": case "heroAbilityList":
  case "itemCode": case "itemList":
  case "upgradeCode": case "upgradeList":
    const obj = data.objects.find(obj => obj.id === value);
    return obj ? <ObjectLinkRaw object={obj}/> : value;
  default:
    return value;
  }
};

export const ObjectValue = ({value, meta, data, rawNames}) => {
  const SubValue = rawNames ? ObjectSubValueRaw : ObjectSubValue;
  if (meta.type.indexOf("List") >= 0) {
    if (value === "_") {
      return <PopupCell>{rawNames ? value : ""}</PopupCell>;
    }
    if (value === "") return <PopupCell/>;
    const list = [];
    value.split(",").forEach((part, index) => {
      if (list.length) list.push(rawNames ? "," : ", ");
      list.push(<SubValue value={part} meta={meta} data={data} key={index}/>);
    });
    return <PopupCell>{list}</PopupCell>;
  } else if (meta.type.indexOf("Flags") >= 0 && !rawNames) {
    const list = [];
    const iValue = parseInt(value, 10);
    for (let flag = 0; flag < 31 && (1 << flag) <= iValue; ++flag) {
      if ((1 << flag) & iValue) {
        if (list.length) list.push(rawNames ? "," : ", ");
        list.push(<SubValue value={flag} meta={meta} data={data} key={flag}/>);
      }
    }
    return <PopupCell>{list}</PopupCell>;
  } else if (meta.type === "string") {
    if (!rawNames && value === "_") value = "";
    return formatString(value, data);
  } else {
    return <PopupCell><SubValue value={value} meta={meta} data={data}/></PopupCell>;
  }
};
