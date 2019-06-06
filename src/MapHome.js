import React from 'react';
import AppCache from 'data/cache';
import tagString from 'data/tagString';

export default class MapHome extends React.Component {
  static contextType = AppCache.DataContext;

  render() {
    const data = this.context;
    let info = data.file("info.json");
    if (info) {
      info = JSON.parse(info);
    }
    let image = null;
    if (data.archive) {
      image = data.archive.loadImage("war3mapPreview.tga");
      if (!image) {
        image = data.archive.loadImage("war3mapMap.blp");
      }
    }
    return (
      <div className="HomePage">
        <h3>{data.core ? "Warcraft III Patch " : ""}{data.name}</h3>
        {info != null && (
          <ul className="mapInfo">
            <li><b>Name:</b> <span>{tagString(info.name)}</span></li>
            <li><b>Suggested Players:</b> <span>{tagString(info.players)}</span></li>
            <li><b>Description:</b> <span>{tagString(info.description)}</span></li>
            <li><b>Author:</b> <span>{tagString(info.author)}</span></li>
          </ul>
        )}
        {image != null && <img src={image} alt="Map preview"/>}
      </div>
    );
  }
}
