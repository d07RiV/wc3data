import React from 'react';
import AppCache from 'data/cache';
import ModelViewer from 'mdx';
import { AutoSizer } from 'react-virtualized';

import { vec3, mat4 } from 'gl-matrix';

const v3pos = vec3.create(), v3dir = vec3.create(), v3up = vec3.create(), v3sub = vec3.create();
const m4rot = mat4.create();
const f32rot = new Float32Array(1);

export default class MapHome extends React.Component {
  static contextType = AppCache.DataContext;
  state = {
    errors: [],
  }

  yaw = -Math.PI / 2;
  pitch = -Math.PI / 4;
  distance = 400;
  center = vec3.create();
  minDistance = 8;
  maxDistance = 3000;
  
  componentWillUnmount() {
    if (this.frame) {
      cancelAnimationFrame(this.frame);
      delete this.frame;
    }
    this.removeEvents();
  }

  
  onMouseDown = e => {
    document.addEventListener("mousemove", this.onMouseMove, true);
    document.addEventListener("mouseup", this.onMouseUp, true);
    this.dragPos = {x: e.clientX, y: e.clientY};
    e.preventDefault();
  }
  onMouseMove = e => {
    if (this.dragPos && this.canvas) {
      const dx = e.clientX - this.dragPos.x,
        dy = e.clientY - this.dragPos.y;
      const dim = (this.canvas.width + this.canvas.height) / 2;
      this.yaw += dx * 2 * Math.PI / dim;
      this.pitch += dy * 2 * Math.PI / dim;
      while (this.yaw > Math.PI) {
        this.yaw -= Math.PI * 2;
      }
      while (this.yaw < -Math.PI) {
        this.yaw += Math.PI * 2;
      }
      this.pitch = Math.min(this.pitch, 0);
      this.pitch = Math.max(this.pitch, -Math.PI);
      this.dragPos = {x: e.clientX, y: e.clientY};
    }
    e.preventDefault();
  }
  onMouseUp = e => {
    delete this.dragPos;
    this.removeEvents();
    e.preventDefault();
  }
  onMouseWheel = e => {
    if (e.deltaY > 0) {
      this.distance = Math.min(this.distance * 1.2, this.maxDistance);
    } else {
      this.distance = Math.max(this.distance / 1.2, this.minDistance);
    }
  }

  removeEvents() {
    document.removeEventListener("mousemove", this.onMouseMove, true);
    document.removeEventListener("mouseup", this.onMouseUp, true);
  }

  
  animate = ts => {
    this.frame = requestAnimationFrame(this.animate);
    this.scene.camera.viewport([0, 0, this.canvas.width, this.canvas.height]);

    this.scene.camera.perspective(Math.PI / 4, this.canvas.width / this.canvas.height, this.minDistance, this.maxDistance);
    this.scene.camera.setRotationAroundAngles(this.yaw, this.pitch, this.center, this.distance);

    this.viewer.updateAndRender();
  }

  
  componentDidMount() {
    if (!this.canvas) {
      return;
    }
    const canvas = this.canvas;

    const data = this.context;
    const resolvePath = (path, tileset) => {
      const ext = typeof path === "string" ? path.substr(path.lastIndexOf(".")).toLowerCase() : ".mdx";
      if ([".blp", ".dds", ".gif", ".jpg", ".jpeg", ".png", ".tga"].indexOf(ext) >= 0) {
        return [data.image(path, tileset), ".png", true];
      } else {
        const bin = data.binary(path);
        if (bin) {
          return [bin.data.buffer, ext, false];
        } else {
          return [data.cache.binary(path, tileset), ext, true];
        }
      }
    };

    this.viewer = new ModelViewer.viewer.handlers.w3x.MapViewer(canvas, resolvePath);
    this.viewer.gl.clearColor(0.3, 0.3, 0.3, 1);
    this.viewer.on('error', (target, error, reason) => {
      if (error === "FailedToFetch") {
        this.setState(({errors}) => ({errors: [...errors, `Failed to load ${target.path}`]}));
      }
    });
    this.scene = this.viewer.scene;

    this.viewer.loadMap(data);

    this.frame = requestAnimationFrame(this.animate);
  }

  render() {
    const { errors } = this.state;
    return (
      <div className="FileModel">
        <AutoSizer>
          {({width, height}) => (
            <canvas
            onMouseDown={this.onMouseDown}
            onWheel={this.onMouseWheel}
            ref={n => this.canvas = n}
            width={Math.max(width, 100)}
            height={Math.max(height, 100)}
            />
          )}
        </AutoSizer>
        <ul className="log">
          {errors.map((err, idx) => <li key={idx} className="error">{err}</li>)}
        </ul>
        <div className="credits">
          Model viewer by <a href="https://github.com/flowtsohg/mdx-m3-viewer" target="_blank">flowtsohg@github</a>
        </div>
      </div>
    );
  }
}
