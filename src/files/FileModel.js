import React from 'react';
import ModelViewer from 'mdx';
import { vec3, mat4 } from 'gl-matrix';
import { AutoSizer } from 'react-virtualized';
import AppCache from 'data/cache';
import pathHash from 'data/hash';

const v3pos = vec3.create(), v3dir = vec3.create(), v3up = vec3.create(), v3sub = vec3.create();
const m4rot = mat4.create();
const f32rot = new Float32Array(1);

const TeamColors = [
  "Red",
  "Blue",
  "Teal",
  "Purple",
  "Yellow",
  "Orange",
  "Green",
  "Pink",
  "Gray",
  "Light Blue",
  "Dark Green",
  "Brown",
  "Maroon",
  "Navy",
  "Turquoise",
  "Violet",
  "Wheat",
  "Peach",
  "Mint",
  "Lavender",
  "Coal",
  "Snow",
  "Emerald",
  "Peanut",
  "Black",
];

export default class FileModelView extends React.PureComponent {
  state = {
    objects: [],
    errors: [],
    current: 0,
    teamColor: 0
  }

  static contextType = AppCache.DataContext;

  yaw = -Math.PI / 2;
  pitch = -Math.PI / 4;
  distance = 400;
  center = vec3.create();
  minDistance = 8;
  maxDistance = 3000;

  freeCamera() {
    if (this.scene) {
      this.scene.camera.setRotationAroundAngles(this.yaw, this.pitch, this.center, this.distance);
    }
  }

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

      this.setCamera({target: {value: -1}});
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
    this.setCamera({target: {value: -1}});
  }

  removeEvents() {
    document.removeEventListener("mousemove", this.onMouseMove, true);
    document.removeEventListener("mouseup", this.onMouseUp, true);
  }

  setObject = e => {
    let index = parseInt(e.target.value, 10);
    if (isNaN(index)) index = 0;
    this.setState(state => {
      if (index !== state.current && state.objects[index]) {
        this.scene.removeInstance(state.objects[state.current].instance);
        this.scene.addInstance(state.objects[index].instance);
        return {current: index};
      }
    });
  }

  setSequence = e => {
    let seq = parseInt(e.target.value, 10);
    if (isNaN(seq)) seq = -1;
    this.setState(state => {
      const obj = state.objects[state.current];
      if (obj && obj.sequence !== seq) {
        obj.instance.setSequence(seq);
        const objects = state.objects.slice();
        objects[state.current] = {...obj, sequence: seq};
        return {objects};
      }
    });
  }

  setCamera = e => {
    let cam = parseInt(e.target.value, 10);
    if (isNaN(cam)) cam = -1;
    this.setState(state => {
      const obj = state.objects[state.current];
      if (obj && obj.camera !== cam) {
        const objects = state.objects.slice();
        objects[state.current] = {...obj, camera: cam};
        return {objects};
      }
    });
  }

  setTeamColor = e => {
    let color = parseInt(e.target.value, 10);
    if (isNaN(color)) color = 0;
    this.setState({teamColor: color}, () => {
      this.state.objects.forEach(obj => obj.instance.setTeamColor(color));
    });
  }

  animate = ts => {
    this.frame = requestAnimationFrame(this.animate);
    this.scene.camera.viewport([0, 0, this.canvas.width, this.canvas.height]);

    const object = this.state.objects[this.state.current];
    const cam = object && object.model.cameras[object.camera];
    if (cam) {
      cam.getPositionTranslation(v3pos, object.instance);
      cam.getTargetTranslation(v3dir, object.instance);
      cam.getRotation(f32rot, object.instance);
      vec3.sub(v3sub, v3dir, v3pos);
      vec3.normalize(v3sub, v3sub);
      mat4.fromRotation(m4rot, f32rot[0], v3sub);
      vec3.set(v3up, 0, 0, 1);
      vec3.transformMat4(v3up, v3up, m4rot);
      const aspect = this.canvas.width / this.canvas.height;
      const vFov = Math.atan(Math.tan(cam.fieldOfView / 2) / aspect) * 2;
      this.scene.camera.perspective(vFov, aspect, cam.nearClippingPlane, cam.farClippingPlane);
      this.scene.camera.moveToAndFace(v3pos, v3dir, v3up);
    } else {
      this.scene.camera.perspective(Math.PI / 4, this.canvas.width / this.canvas.height, this.minDistance, this.maxDistance);
      this.freeCamera();
    }

    this.viewer.updateAndRender();
  }

  modelLoaded(model, index) {
    if (!model.ok) {
      return;
    }
    this.setState(state => {
      const instance = model.addInstance();
      index = Math.min(index, state.objects.length);
      if (index === state.current) {
        if (state.objects[index]) {
          this.scene.removeInstance(state.objects[index].instance);
        }
        this.scene.addInstance(instance);
        if (model.bounds && model.bounds.radius > 0.05) {
          this.minDistance = Math.min(500, model.bounds.radius * 0.2);
          this.maxDistance = this.minDistance * 1000;
          this.distance = this.minDistance * 10;
        }
      }
      instance.setTeamColor(state.teamColor);
      if (model.sequences.length > 0) {
        instance.setSequence(0);
      }
      instance.setSequenceLoopMode(2);
      const objects = [...state.objects];
      objects.splice(index, 0, {
        model,
        instance,
        sequence: model.sequences.length ? 0 : -1,
        camera: model.cameras.length ? 0 : -1,
      });
      return {objects};
    });
  }

  componentDidMount() {
    if (!this.canvas) {
      return;
    }
    const canvas = this.canvas;
    this.viewer = new ModelViewer.viewer.ModelViewer(canvas);
    this.viewer.gl.clearColor(0.3, 0.3, 0.3, 1);
    this.viewer.on('error', (target, error, reason) => {
      if (error === "FailedToFetch") {
        if (target.isPortrait) {
          return;
        }
        this.setState(({errors}) => ({errors: [...errors, `Failed to load ${target.path}`]}));
      }
    });
    this.scene = this.viewer.addScene();
    this.viewer.addHandler(ModelViewer.viewer.handlers.mdx);

    const data = this.context;

    const hash = (this.props.path ? pathHash(this.props.path) : this.props.id);

    const resolvePath = path => {
      const ext = typeof path === "string" ? path.substr(path.lastIndexOf(".")).toLowerCase() : ".mdx";
      if ([".blp", ".dds", ".gif", ".jpg", ".jpeg", ".png", ".tga"].indexOf(ext) >= 0) {
        return [data.image(path), ".png", true];
      } else {
        const bin = data.binary(path);
        if (bin) {
          return [bin.data.buffer, ext, false];
        } else {
          return [data.cache.binary(path), ext, true];
        }
      }
    };

    this.viewer.load(hash, resolvePath).whenLoaded().then(model => this.modelLoaded(model, 0));
    if (this.props.path) {
      const portrait = this.props.path.replace(/\.mdx$/i, "_portrait.mdx");
      const model = this.viewer.load(portrait, resolvePath);
      model.isPortrait = true;
      model.whenLoaded().then(model => this.modelLoaded(model, 1));
    }

    this.frame = requestAnimationFrame(this.animate);
  }

  render() {
    const { errors, objects, current, teamColor } = this.state;
    const object = objects[current];
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
        {objects.length > 0 && (
          <div className="controls">
            {objects.length > 1 && <label>Model:
              <select value={current} onChange={this.setObject}>
                {objects.map((obj, i) => <option key={i} value={i}>{obj.model.name}</option>)}
              </select>
            </label>}
            {object != null && <label>Animation:
              <select value={object.sequence} onChange={this.setSequence}>
                <option value={-1}>None</option>
                {object.model.sequences.map((seq, i) => <option key={i} value={i}>{seq.name}</option>)}
              </select>
            </label>}
            {object != null && object.model.cameras.length > 0 && <label>Camera:
              <select value={object.camera} onChange={this.setCamera}>
                <option value={-1}>Free</option>
                {object.model.cameras.map((cam, i) => <option key={i} value={i}>{cam.name}</option>)}
              </select>
            </label>}
            <label>Team Color:
              <select value={teamColor} onChange={this.setTeamColor}>
                {TeamColors.map((name, i) => <option key={i} value={i}>{name}</option>)}
              </select>
            </label>
          </div>
        )}
      </div>
    );
  }
}
