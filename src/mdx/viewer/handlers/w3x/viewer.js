import {vec3, quat} from 'gl-matrix';
import {VEC3_UNIT_Z} from '../../../common/gl-matrix-addon';
import unique from '../../../common/arrayunique';
import War3Map from '../../../parsers/w3x/map';
import War3MapW3i from '../../../parsers/w3x/w3i/file';
import War3MapW3e from '../../../parsers/w3x/w3e/file';
import War3MapDoo from '../../../parsers/w3x/doo/file';
import War3MapUnitsDoo from '../../../parsers/w3x/unitsdoo/file';
import MappedData from '../../../utils/mappeddata';
import ModelViewer from '../../viewer';
import geoHandler from '../geo/handler';
import mdxHandler from '../mdx/handler';
import shaders from './shaders';
import getCliffVariation from './variations';
import TerrainModel from './terrainmodel';
import SplatModel from './splatmodel';
// import SimpleModel from './simplemodel';
import standOnRepeat from './standsequence';

let normalHeap1 = vec3.create();
let normalHeap2 = vec3.create();

/**
 *
 */
export default class War3MapViewer extends ModelViewer {
  /**
   * @param {HTMLCanvasElement} canvas
   * @param {function} wc3PathSolver
   */
  constructor(canvas, wc3PathSolver, mapData) {
    super(canvas);

    this.batchSize = 256;

    this.on('error', (target, error, reason) => console.error(target, error, reason));

    this.addHandler(geoHandler);
    this.addHandler(mdxHandler);

    /** @member {function} */
    this.wc3PathSolver = wc3PathSolver;

    this.groundShader = this.loadShader('Ground', shaders.vsGround, shaders.fsGround);
    this.waterShader = this.loadShader('Water', shaders.vsWater, shaders.fsWater);
    this.cliffShader = this.loadShader('Cliffs', shaders.vsCliffs, shaders.fsCliffs);
    this.simpleModelShader = this.loadShader('SimpleModel', shaders.vsSimpleModel, shaders.fsSimpleModel);
    this.uberSplatShader = this.loadShader('UberSplats', shaders.vsUberSplat, shaders.fsUberSplat);

    this.scene = this.addScene();
    this.camera = this.scene.camera;
    this.units = [];

    this.waterIndex = 0;
    this.waterIncreasePerFrame = 0;

    this.anyReady = false;

    this.terrainCliffsAndWaterLoaded = false;
    this.terrainData = new MappedData();
    this.cliffTypesData = new MappedData();
    this.waterData = new MappedData();
    this.terrainReady = false;
    this.cliffsReady = false;
    this.shadowsReady = false;

    const objects = mapData.objects();

    Promise.all([
      objects,
      ...['TerrainArt\\Terrain.slk', 'TerrainArt\\CliffTypes.slk', 'TerrainArt\\Water.slk']
        .map((path) => this.loadGeneric(wc3PathSolver(path)[0], 'text', undefined, path).whenLoaded())
    ]).then(([obj, terrain, cliffTypes, water]) => {
        this.terrainObjects = obj.objects;
        this.terrainCliffsAndWaterLoaded = true;
        this.terrainData.load(terrain.data);
        this.cliffTypesData.load(cliffTypes.data);
        this.waterData.load(water.data);
        this.emit('terrainloaded');
      });

    this.doodadsAndDestructiblesLoaded = false;
    this.doodadsData = new MappedData();
    this.doodadMetaData = new MappedData();
    this.destructableMetaData = new MappedData();
    this.doodads = [];
    this.terrainDoodads = [];
    this.doodadsReady = false;
    this.uberSplats = [];
    this.uberSplatsReady = false;

    objects.then(obj => {
      this.doodadsAndDestructiblesLoaded = true;
      this.doodadsData = obj.objects;
      this.emit('doodadsloaded');
    });

    this.unitsAndItemsLoaded = false;
    this.unitsData = new MappedData();
    this.unitMetaData = new MappedData();
    this.uberSplatData = new MappedData();
    this.units = [];
    this.unitsReady = false;

    Promise.all([objects, this.loadGeneric(wc3PathSolver('Splats\\UberSplatData.slk')[0], 'text', undefined, 'Splats\\UberSplatData.slk').whenLoaded()])
      .then(([obj, uberSplatData]) => {
        this.unitsAndItemsLoaded = true;
        this.unitsData = obj.objects;
        this.uberSplatData.load(uberSplatData.data);
        this.emit('unitsloaded');
      });
  
    let gl = this.gl;
    let shadowMap = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, shadowMap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R8, 4, 4, 0, gl.RED, gl.UNSIGNED_BYTE, new Uint8Array(16));
    this.shadowMap = shadowMap;
  }

  /**
   *
   */
  renderGround() {
    if (this.terrainReady) {
      let gl = this.gl;
      let webgl = this.webgl;
      let instancedArrays = webgl.extensions.instancedArrays;
      let shader = this.groundShader;
      let uniforms = shader.uniforms;
      let attribs = shader.attribs;
      let {columns, rows, centerOffset, vertexBuffer, faceBuffer, heightMap, cliffHeightMap, instanceBuffer,
        instanceCount, textureBuffer, variationBuffer, heightMapSize} = this.terrainRenderData;
      let tilesetTextures = this.tilesetTextures;
      let instanceAttrib = attribs.a_InstanceID;
      let positionAttrib = attribs.a_position;
      let texturesAttrib = attribs.a_textures;
      let variationsAttrib = attribs.a_variations;

      gl.disable(gl.BLEND);

      webgl.useShaderProgram(shader);

      gl.uniformMatrix4fv(uniforms.u_mvp, false, this.camera.worldProjectionMatrix);
      gl.uniform2fv(uniforms.u_offset, centerOffset);
      gl.uniform2f(uniforms.u_size, columns - 1, rows - 1);
      gl.uniform2fv(uniforms.u_pixel, heightMapSize);
      gl.uniform1i(uniforms.u_heightMap, 0);
      gl.uniform1i(uniforms.u_tilesets, 1);
      gl.uniform1i(uniforms.u_shadowMap, 2);
      gl.uniform1i(uniforms.u_heightMap0, 3);
      gl.uniform1f(uniforms.u_tilesetHeight, 1 / (tilesetTextures.length + 1));
      gl.uniform1f(uniforms.u_tilesetCount, tilesetTextures.length + 1);

      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, heightMap);

      gl.activeTexture(gl.TEXTURE1);
      gl.bindTexture(gl.TEXTURE_2D, this.tilesetsTexture);

      gl.activeTexture(gl.TEXTURE2);
      gl.bindTexture(gl.TEXTURE_2D, this.shadowMap);

      gl.activeTexture(gl.TEXTURE3);
      gl.bindTexture(gl.TEXTURE_2D, cliffHeightMap);

      gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
      gl.vertexAttribPointer(positionAttrib, 2, gl.FLOAT, false, 8, 0);

      gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
      gl.vertexAttribPointer(instanceAttrib, 1, gl.FLOAT, false, 4, 0);
      instancedArrays.vertexAttribDivisorANGLE(instanceAttrib, 1);

      gl.bindBuffer(gl.ARRAY_BUFFER, textureBuffer);
      gl.vertexAttribPointer(texturesAttrib, 4, gl.UNSIGNED_BYTE, false, 4, 0);
      instancedArrays.vertexAttribDivisorANGLE(texturesAttrib, 1);

      gl.bindBuffer(gl.ARRAY_BUFFER, variationBuffer);
      gl.vertexAttribPointer(variationsAttrib, 4, gl.UNSIGNED_BYTE, false, 4, 0);
      instancedArrays.vertexAttribDivisorANGLE(variationsAttrib, 1);

      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, faceBuffer);

      instancedArrays.drawElementsInstancedANGLE(gl.TRIANGLES, 6, gl.UNSIGNED_BYTE, 0, instanceCount);

      instancedArrays.vertexAttribDivisorANGLE(texturesAttrib, 0);
      instancedArrays.vertexAttribDivisorANGLE(variationsAttrib, 0);
      instancedArrays.vertexAttribDivisorANGLE(instanceAttrib, 0);
    }
  }

  /**
   *
   */
  renderWater() {
    if (this.terrainReady) {
      let gl = this.gl;
      let webgl = this.webgl;
      let instancedArrays = webgl.extensions.instancedArrays;
      let shader = this.waterShader;
      let uniforms = shader.uniforms;
      let attribs = shader.attribs;
      let mapBounds = this.mapBounds;
      let {columns, rows, centerOffset, vertexBuffer, faceBuffer, heightMap, instanceBuffer, instanceCount, waterHeightMap, waterBuffer, heightMapSize} = this.terrainRenderData;
      let instanceAttrib = attribs.a_InstanceID;
      let positionAttrib = attribs.a_position;
      let isWaterAttrib = attribs.a_isWater;

      gl.enable(gl.BLEND);
      gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);

      webgl.useShaderProgram(shader);

      gl.uniformMatrix4fv(uniforms.u_mvp, false, this.camera.worldProjectionMatrix);
      gl.uniform2fv(uniforms.u_offset, centerOffset);
      gl.uniform2f(uniforms.u_size, columns - 1, rows - 1);
      gl.uniform2fv(uniforms.u_pixel, heightMapSize);
      gl.uniform1i(uniforms.u_heightMap, 0);
      gl.uniform1i(uniforms.u_waterHeightMap, 1);
      gl.uniform1i(uniforms.u_waterMap, 2);
      gl.uniform1f(uniforms.u_offsetHeight, this.waterHeightOffset);
      gl.uniform1f(uniforms.u_tileIndex, this.waterIndex | 0);
      gl.uniform4fv(uniforms.u_maxDeepColor, this.maxDeepColor);
      gl.uniform4fv(uniforms.u_minDeepColor, this.minDeepColor);
      gl.uniform4fv(uniforms.u_maxShallowColor, this.maxShallowColor);
      gl.uniform4f(uniforms.u_mapBounds, mapBounds[0] * 128.0 + centerOffset[0],
        mapBounds[2] * 128.0 + centerOffset[1],
        (columns - mapBounds[1] - 1) * 128.0 + centerOffset[0],
        (rows - mapBounds[3] - 1) * 128.0 + centerOffset[1]);

      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, heightMap);

      gl.activeTexture(gl.TEXTURE1);
      gl.bindTexture(gl.TEXTURE_2D, waterHeightMap);

      gl.activeTexture(gl.TEXTURE2);
      gl.bindTexture(gl.TEXTURE_2D, this.waterTexture);

      gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
      gl.vertexAttribPointer(positionAttrib, 2, gl.FLOAT, false, 8, 0);

      gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
      gl.vertexAttribPointer(instanceAttrib, 1, gl.FLOAT, false, 4, 0);
      instancedArrays.vertexAttribDivisorANGLE(instanceAttrib, 1);

      gl.bindBuffer(gl.ARRAY_BUFFER, waterBuffer);
      gl.vertexAttribPointer(isWaterAttrib, 1, gl.UNSIGNED_BYTE, false, 1, 0);
      instancedArrays.vertexAttribDivisorANGLE(isWaterAttrib, 1);

      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, faceBuffer);
      instancedArrays.drawElementsInstancedANGLE(gl.TRIANGLES, 6, gl.UNSIGNED_BYTE, 0, instanceCount);

      instancedArrays.vertexAttribDivisorANGLE(isWaterAttrib, 0);
      instancedArrays.vertexAttribDivisorANGLE(instanceAttrib, 0);
    }
  }

  /**
   *
   */
  renderCliffs() {
    if (this.cliffsReady) {
      let gl = this.gl;
      let instancedArrays = gl.extensions.instancedArrays;
      let webgl = this.webgl;
      let shader = this.cliffShader;
      let attribs = shader.attribs;
      let uniforms = shader.uniforms;
      let {columns, rows, centerOffset, cliffHeightMap, heightMapSize} = this.terrainRenderData;

      gl.disable(gl.BLEND);

      webgl.useShaderProgram(shader);

      gl.uniformMatrix4fv(uniforms.u_mvp, false, this.camera.worldProjectionMatrix);
      gl.uniform1i(uniforms.u_heightMap, 0);
      gl.uniform2f(uniforms.u_size, columns - 1, rows - 1);
      gl.uniform2fv(uniforms.u_pixel, heightMapSize);
      gl.uniform2fv(uniforms.u_centerOffset, centerOffset);
      gl.uniform1i(uniforms.u_texture1, 1);
      gl.uniform1i(uniforms.u_texture2, 2);
      gl.uniform1i(uniforms.u_shadowMap, 3);

      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, cliffHeightMap);

      gl.activeTexture(gl.TEXTURE1);
      gl.bindTexture(gl.TEXTURE_2D, this.cliffTextures[0].webglResource);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);

      if (this.cliffTextures.length > 1) {
        gl.activeTexture(gl.TEXTURE2);
        gl.bindTexture(gl.TEXTURE_2D, this.cliffTextures[1].webglResource);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      }

      gl.activeTexture(gl.TEXTURE3);
      gl.bindTexture(gl.TEXTURE_2D, this.shadowMap);

      // Set instanced attributes.
      instancedArrays.vertexAttribDivisorANGLE(attribs.a_instancePosition, 1);
      instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceTexture, 1);

      // Render the cliffs.
      for (let cliff of this.cliffModels) {
        cliff.render(gl, instancedArrays, attribs, this.cliffTextures);
      }

      // Clear instanced attributes.
      instancedArrays.vertexAttribDivisorANGLE(attribs.a_instancePosition, 0);
      instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceTexture, 0);
    }
  }

  /**
   *
   */
  renderUberSplats() {
    if (this.terrainReady && this.uberSplatsReady) {
      let gl = this.gl;
      let webgl = this.webgl;
      let shader = this.uberSplatShader;
      let attribs = shader.attribs;
      let uniforms = shader.uniforms;
      let {columns, rows, centerOffset, heightMap, heightMapSize} = this.terrainRenderData;

      gl.enable(gl.BLEND);
      gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
      gl.blendEquation(gl.FUNC_ADD);

      webgl.useShaderProgram(shader);

      gl.uniformMatrix4fv(uniforms.u_mvp, false, this.camera.worldProjectionMatrix);
      gl.uniform1i(uniforms.u_heightMap, 0);
      gl.uniform2f(uniforms.u_size, columns - 1, rows - 1);
      gl.uniform2fv(uniforms.u_pixel, heightMapSize);
      gl.uniform2fv(uniforms.u_centerOffset, centerOffset);
      gl.uniform1i(uniforms.u_texture, 1);
      gl.uniform1i(uniforms.u_shadowMap, 2);

      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, heightMap);

      gl.activeTexture(gl.TEXTURE2);
      gl.bindTexture(gl.TEXTURE_2D, this.shadowMap);

      // Render the cliffs.
      for (let splat of this.uberSplatModels) {
        splat.render(gl, uniforms, attribs);
      }
    }
  }

  // /**
  //  * 
  //  */
  // renderDoodads(opaque) {
  //   if (this.doodadsReady) {
  //     let gl = this.gl;
  //     let instancedArrays = gl.extensions.instancedArrays;
  //     let webgl = this.webgl;
  //     let shader = this.simpleModelShader;
  //     let attribs = shader.attribs;
  //     let uniforms = shader.uniforms;

  //     webgl.useShaderProgram(shader);

  //     gl.uniformMatrix4fv(uniforms.u_mvp, false, this.camera.worldProjectionMatrix);
  //     gl.uniform1i(uniforms.u_texture, 0);

  //     gl.activeTexture(gl.TEXTURE0);

  //     // Enable instancing.
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instancePosition, 1);
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceRotation, 1);
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceScale, 1);

  //     // Render the dooadads.
  //     for (let doodad of this.doodads) {
  //       if (opaque) {
  //         doodad.renderOpaque(gl, instancedArrays, uniforms, attribs);
  //       } else {
  //         doodad.renderTranslucent(gl, instancedArrays, uniforms, attribs);
  //       }
  //     }

  //     // Render the terrain doodads.
  //     for (let doodad of this.terrainDoodads) {
  //       if (opaque) {
  //         doodad.renderOpaque(gl, instancedArrays, uniforms, attribs);
  //       } else {
  //         doodad.renderTranslucent(gl, instancedArrays, uniforms, attribs);
  //       }
  //     }

  //     // Disable instancing.
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instancePosition, 0);
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceRotation, 0);
  //     instancedArrays.vertexAttribDivisorANGLE(attribs.a_instanceScale, 0);
  //   }
  // }

  /**
   * Render the map.
   */
  render() {
    if (this.anyReady) {
      this.gl.viewport(...this.camera.rect);

      this.renderGround();
      this.renderCliffs();
      // this.renderDoodads(true);
      super.renderOpaque();
      // this.renderDoodads(false);
      this.renderUberSplats();
      this.renderWater();
      super.renderTranslucent();
    }
  }

  /**
   * Update the map.
   */
  update() {
    if (this.anyReady) {
      if (this.waterTextures) {
        this.waterIndex += this.waterIncreasePerFrame;
        if (this.waterIndex >= this.waterTextures.length) {
          this.waterIndex = 0;
        }
      }

      super.update();
    }
  }

  /**
   * @param {*} buffer 
   */
  async loadMap(buffer) {
    // Readonly mode to optimize memory usage.
    this.mapMpq = new War3Map(buffer, true);

    let wc3PathSolver = this.wc3PathSolver;

    let w3i = new War3MapW3i(this.mapMpq.get('war3map.w3i').arrayBuffer());
    this.w3i = w3i;
    this.tileset = w3i.tileset;
    this.editorVersion = w3i.editorVersion;
    this.mapBounds = w3i.cameraBoundsComplements;
    this.mapFlags = w3i.flags;

    this.emit('maploaded');

    this.mapPathSolver = (path) => {
      // MPQ paths have backwards slashes...always? Don't know.
      //let mpqPath = path.replace(/\//g, '\\');

      // If the file is in the map, return it.
      // Otherwise, if it's in the tileset MPQ, return it from there.
      //let file = this.mapMpq.get(mpqPath);
      //if (file) {
      //  return [file.arrayBuffer(), path.substr(path.lastIndexOf('.')), false];
      //}

      // Try to get the file from the game MPQs.
      return wc3PathSolver(path, this.tileset);
    };

    let w3e = new War3MapW3e(this.mapMpq.get('war3map.w3e').arrayBuffer());

    this.corners = w3e.corners;
    this.centerOffset = w3e.centerOffset;
    this.mapSize = w3e.mapSize;

    this.calculateRamps();

    const source = this.mapMpq.get('war3map.doo');
    if (!source) {
      return;
    }
    this.doodads = new War3MapDoo(source.arrayBuffer());

    this.emit('tilesetloaded');

    if (this.terrainCliffsAndWaterLoaded) {
      this.loadTerrainCliffsAndWater(w3e);
    } else {
      this.once('terrainloaded', () => this.loadTerrainCliffsAndWater(w3e));
    }

    let doodadsLoaded, unitsLoaded;

    this.shadows = [];
    this.shadowTextures = {};

    if (this.doodadsAndDestructiblesLoaded) {
      this.loadDoodadsAndDestructibles();
      doodadsLoaded = Promise.resolve();
    } else {
      doodadsLoaded = new Promise((resolve, reject) => {
        this.once('doodadsloaded', () => {
          this.loadDoodadsAndDestructibles()
          resolve();
        });
      });
    }

    if (this.unitsAndItemsLoaded) {
      this.loadUnitsAndItems();
      unitsLoaded = Promise.resolve();
    } else {
      unitsLoaded = new Promise((resolve, reject) => {
        this.once('unitsloaded', () => {
          this.loadUnitsAndItems();
          resolve();
        });
      });
    }

    Promise.all([doodadsLoaded, unitsLoaded]).then(() => {
      this.initShadows();
    });
  }

  /**
   * 
   * @param {*} modifications 
   */
  loadDoodadsAndDestructibles() {
    let scene = this.scene;
    let doo = this.doodads;

    // Collect the doodad and destructible data.
    for (let doodad of doo.doodads) {
      let row = this.doodadsData.find(row => row.id === doodad.id);
      if (!row) continue;
      let type = row.type;
      row = row.data;
      let file = row.file;
      let numVar = parseInt(row.numvar || "0");

      if (file.endsWith('.mdl')) {
        file = file.slice(0, -4);
      }

      let fileVar = file;

      file += '.mdx';

      if (numVar > 1) {
        fileVar += Math.min(doodad.variation, numVar - 1);
      }

      fileVar += '.mdx';

      let color = [255, 255, 255];

      if (type === 'destructible') {
        color[0] = parseInt(row.colorr);
        color[1] = parseInt(row.colorg);
        color[2] = parseInt(row.colorb);
      }

      if (type === 'destructible' && row.shadow && row.shadow !== "_") {
        this.addShadow(row.shadow, doodad.location[0], doodad.location[1]);
      }

      // First see if the model is local.
      // Doodads refering to local models may have invalid variations, so if the variation doesn't exist, try without a variation.
      let mpqFile = this.mapMpq.get(fileVar) || this.mapMpq.get(file);
      let model;

      if (mpqFile) {
        model = this.load(mpqFile.name);
      } else {
        model = this.load(fileVar);
      }

      model.whenLoaded().then(() => {
        let instance = model.addInstance();
        if (row.texfile) {
          const repl = model.replaceables.indexOf(parseInt(row.texid || "0"));
          if (repl >= 0) {
            let tex = row.texfile;
            if (tex.lastIndexOf('.') <= Math.max(tex.lastIndexOf('/'), tex.lastIndexOf('\\'))) {
              tex += '.blp';
            }
            instance.setTexture(model.textures[repl], this.load(tex));
          }
        }
        instance.move(doodad.location);
        instance.rotateLocal(quat.setAxisAngle(quat.create(), VEC3_UNIT_Z, doodad.angle));
        instance.scale(doodad.scale);
        instance.setScene(scene);
        if (!this.inPlayableArea(doodad.location[0], doodad.location[1])) {
          color[0] = Math.round(color[0] * 51 / 255);
          color[1] = Math.round(color[1] * 51 / 255);
          color[2] = Math.round(color[2] * 51 / 255);
        }
        instance.setVertexColor(color);
  
        standOnRepeat(instance);
      });

    }

    this.doodadsReady = true;
    this.anyReady = true;
  }

  /**
   * 
   * @param {*} modifications 
   */
  loadUnitsAndItems(modifications) {
    const source = this.mapMpq.get('war3mapUnits.doo');
    if (!source) {
      return;
    }
    let unitsDoo = new War3MapUnitsDoo(source.arrayBuffer());
    let scene = this.scene;
    let splats = {};

    // Collect the units and items data.
    for (let unit of unitsDoo.units) {
      let path;

      // Hardcoded?
      let location = unit.location;
      let scale = unit.scale;
      let teamColor = unit.player;
      let animProps = "";
      // fix team colors for older versions
      if (this.editorVersion < 0x17A8 && teamColor >= 12) {
        teamColor += 12;
      }
      let row = null;
      let color = [255, 255, 255];

      if (unit.id === 'sloc') {
        path = 'Objects\\StartLocation\\StartLocation.mdx';
      } else {
        row = this.unitsData.find(row => row.id === unit.id);
        if (!row) continue;
        const type = row.type;
        row = row.data;

        path = row.file;

        if (path.endsWith('.mdl')) {
          path = path.slice(0, -4);
        }

        scale = vec3.clone(unit.scale);
        vec3.scale(scale, scale, row.modelscale);

        let rowTeamColor = row.teamcolor && parseInt(row.teamcolor);
        if (row.teamcolor && rowTeamColor >= 0) {
          teamColor = rowTeamColor;
        }

        if (type === 'unit') {
          color[0] = parseInt(row.red);
          color[1] = parseInt(row.green);
          color[2] = parseInt(row.blue);
        } else if (type === 'item') {
          color[0] = parseInt(row.colorr);
          color[1] = parseInt(row.colorg);
          color[2] = parseInt(row.colorb);
        }
  
        let rowMoveHeight = row.moveheight && parseFloat(row.moveheight);
        if (rowMoveHeight) {
          location = vec3.clone(unit.location);
          location[2] += rowMoveHeight;
        }

        path += '.mdx';

        let splat = row.ubersplat && this.uberSplatData.getRow(row.ubersplat);
        if (splat) {
          let texture = `${splat.Dir}\\${splat.file}.blp`;
          if (!splats[texture]) {
            splats[texture] = {locations: [], opacity: 1};
          }
          let x = unit.location[0], y = unit.location[1], s = splat.Scale;
          splats[texture].locations.push(x - s, y - s, x + s, y + s, 1);
        }

        if (row.unitshadow && row.unitshadow !== "_") {
          let texture = `ReplaceableTextures\\Shadows\\${row.unitshadow}.blp`;
          let shadowX = parseFloat(row.shadowx || "0");
          let shadowY = parseFloat(row.shadowy || "0");
          let shadowW = parseFloat(row.shadoww || "0");
          let shadowH = parseFloat(row.shadowh || "0");
          if (!splats[texture]) {
            splats[texture] = {locations: [], opacity: 0.5};
          }
          let x = unit.location[0] - shadowX, y = unit.location[1] - shadowY;
          splats[texture].locations.push(x, y, x + shadowW, y + shadowH, 3);
        }

        if (row.buildingshadow && row.buildingshadow !== "_") {
          this.addShadow(row.buildingshadow, unit.location[0], unit.location[1]);
        }

        animProps = row.animprops;
      }

      if (path) {
        let model = this.load(path);
        let instance = model.addInstance();

        //let normal = this.groundNormal([], unit.location[0], unit.location[1]);

        instance.move(location);
        instance.rotateLocal(quat.setAxisAngle(quat.create(), VEC3_UNIT_Z, unit.angle));
        instance.scale(scale);
        instance.setTeamColor(teamColor);
        instance.setScene(scene);
        if (!this.inPlayableArea(location[0], location[1])) {
          color[0] = Math.round(color[0] * 51 / 255);
          color[1] = Math.round(color[1] * 51 / 255);
          color[2] = Math.round(color[2] * 51 / 255);
        }
        instance.setVertexColor(color);

        if (row) {
          this.units.push({unit, instance, location, radius: parseFloat(row.scale || "0") * 36});
        }

        standOnRepeat(instance, animProps);
      } else {
        console.log('Unknown unit ID', unit.id, unit)
      }
    }

    this.unitsReady = true;
    this.anyReady = true;

    let splatPromises = Object.entries(splats).map((splat) => {
      let path = splat[0];
      let {locations, opacity} = splat[1];

      return this.load(path).whenLoaded()
        .then((texture) => {
          let splat = new SplatModel(this.gl, texture, locations, this.centerOffset);
          splat.color[3] = opacity;
          return splat;
        });
    });

    Promise.all(splatPromises).then(models => {
      this.uberSplatModels = models;
      this.uberSplatsReady = true;
    });
  }

  /**
   *
   */
  async loadTerrainCliffsAndWater(w3e) {
    let tileset = w3e.tileset;

    this.tilesets = [];
    this.tilesetTextures = [];

    for (let groundTileset of w3e.groundTilesets) {
      let row = this.terrainData.getRow(groundTileset);
      if (!row) {
        continue;
      }

      this.tilesets.push(row);
      this.tilesetTextures.push(this.load(`${row.dir}\\${row.file}.blp`));
    }

    let blights = {
      A: 'Ashen',
      B: 'Barrens',
      C: 'Felwood',
      D: 'Cave',
      F: 'Lordf',
      G: 'Dungeon',
      I: 'Ice',
      J: 'DRuins',
      K: 'Citadel',
      L: 'Lords',
      N: 'North',
      O: 'Outland',
      Q: 'VillageFall',
      V: 'Village',
      W: 'Lordw',
      X: 'Village',
      Y: 'Village',
      Z: 'Ruins',
    };

    this.blightTextureIndex = this.tilesetTextures.length;
    this.tilesetTextures.push(this.load(`TerrainArt\\Blight\\${blights[tileset]}_Blight.blp`));

    this.cliffTilesets = [];
    this.cliffTextures = [];

    for (let cliffTileset of w3e.cliffTilesets) {
      let row = this.cliffTypesData.getRow(cliffTileset);
      if (!row) {
        continue;
      }

      this.cliffTilesets.push(row);
      this.cliffTextures.push(this.load(`${row.texDir}\\${row.texFile}.blp`));
    }

    let waterRow = this.waterData.getRow(`${tileset}Sha`);

    this.waterHeightOffset = waterRow.height;
    this.waterIncreasePerFrame = waterRow.texRate / 60;
    this.waterTextures = [];
    this.maxDeepColor = new Float32Array([waterRow.Dmax_R, waterRow.Dmax_G, waterRow.Dmax_B, waterRow.Dmax_A]);
    this.minDeepColor = new Float32Array([waterRow.Dmin_R, waterRow.Dmin_G, waterRow.Dmin_B, waterRow.Dmin_A]);
    this.maxShallowColor = new Float32Array([waterRow.Smax_R, waterRow.Smax_G, waterRow.Smax_B, waterRow.Smax_A]);
    this.minShallowColor = new Float32Array([waterRow.Smin_R, waterRow.Smin_G, waterRow.Smin_B, waterRow.Smin_A]);

    for (let i = 0, l = waterRow.numTex; i < l; i++) {
      this.waterTextures.push(this.load(`${waterRow.texFile}${i < 10 ? '0' : ''}${i}.blp`));
    }

    await this.whenLoaded([...this.tilesetTextures, ...this.waterTextures]);

    let gl = this.gl;

    this.createTilesetsAndWaterTextures();

    let corners = w3e.corners;
    let [columns, rows] = this.mapSize;
    let centerOffset = this.centerOffset;
    let instanceCount = (columns - 1) * (rows - 1);
    let cliffHeights = new Float32Array(columns * rows);
    let cornerHeights = new Float32Array(columns * rows);
    let waterHeights = new Float32Array(columns * rows);
    let cornerTextures = new Uint8Array(instanceCount * 4);
    let cornerVariations = new Uint8Array(instanceCount * 4);
    let waterFlags = new Uint8Array(instanceCount);
    let instance = 0;
    let cliffs = {};

    this.columns = columns - 1;
    this.rows = rows - 1;

    let cliffPathTex = {};
    for (let doodad of this.doodads.terrainDoodads) {
      let row = this.terrainObjects.find(row => row.id === doodad.id);
      if (!row) continue;
      row = row.data;
      if (!cliffPathTex[row.pathtex]) {
        cliffPathTex[row.pathtex] = this.load(row.pathtex);
      }
    }
    await this.whenLoaded(Object.values(cliffPathTex));
    for (let doodad of this.doodads.terrainDoodads) {
      let row = this.terrainObjects.find(row => row.id === doodad.id);
      if (!row) continue;
      row = row.data;
      let pathTex = cliffPathTex[row.pathtex];
      if (!pathTex || !pathTex.imageData) {
        continue;
      }
      let path = row.file + ".mdx";
      if (!cliffs[path]) {
        cliffs[path] = {locations: []};
      }
      let [x, y] = doodad.location;
      let w = pathTex.originalData.width >> 2, h = pathTex.originalData.height >> 2;
      for (let i = 0; i < h; ++i) {
        for (let j = 0; j < w; ++j) {
          corners[y + i][x + j].rampType = -1;
        }
      }
      cliffs[path].locations.push((x + w/2) * 128.0 + centerOffset[0], (y + h/2) * 128.0 + centerOffset[1], (corners[y][x].layerHeight - 2) * 128.0);
    }
    
    for (let y = 0; y < rows; y++) {
      for (let x = 0; x < columns; x++) {
        let bottomLeft = corners[y][x];
        let index = y * columns + x;

        cliffHeights[index] = bottomLeft.groundHeight;
        cornerHeights[index] = bottomLeft.groundHeight + bottomLeft.layerHeight + (bottomLeft.rampAdjust || 0) - 2;
        waterHeights[index] = bottomLeft.waterHeight;

        bottomLeft.depth = bottomLeft.water ? this.waterHeightOffset + bottomLeft.waterHeight - cornerHeights[index] : 0;

        if (y < rows - 1 && x < columns - 1) {
          // Water can be used with cliffs and normal corners, so store water state regardless.
          waterFlags[instance] = this.isWater(x, y);

          // Is this a cliff, or a normal corner?
          if (bottomLeft.rampType) {
            if (bottomLeft.rampType > 0) {
              let cliffTexture = bottomLeft.cliffTexture;
              if (cliffTexture === 15) {
                cliffTexture = 1;
              }
              let cliffRow = this.cliffTilesets[cliffTexture];
              let dir = cliffRow.rampModelDir;
              let path = `Doodads\\Terrain\\${dir}\\${dir}${bottomLeft.rampName}0.mdx`;

              let base = bottomLeft.layerHeight;
              if (bottomLeft.cliffType === 1) {
                base = Math.min(base, corners[y + 2][x].layerHeight, corners[y][x + 1].layerHeight, corners[y + 2][x + 1].layerHeight);
              } else {
                base = Math.min(base, corners[y + 1][x].layerHeight, corners[y][x + 2].layerHeight, corners[y + 1][x + 2].layerHeight);
              }

              if (!cliffs[path]) {
                cliffs[path] = {locations: [], textures: []};
              }
              cliffs[path].locations.push(x * 128 + centerOffset[0], y * 128 + centerOffset[1], (base - 2) * 128);
              cliffs[path].textures.push(cliffTexture);
            }
          } else if (this.isCliff(x, y)) {
            let bottomLeftLayer = bottomLeft.layerHeight;
            let bottomRightLayer = corners[y][x + 1].layerHeight;
            let topLeftLayer = corners[y + 1][x].layerHeight;
            let topRightLayer = corners[y + 1][x + 1].layerHeight;
            let base = Math.min(bottomLeftLayer, bottomRightLayer, topLeftLayer, topRightLayer);
            let fileName = this.cliffFileName(bottomLeftLayer, bottomRightLayer, topLeftLayer, topRightLayer, base);

            if (fileName !== 'AAAA') {
              let cliffTexture = bottomLeft.cliffTexture;

              // ?
              if (cliffTexture === 15) {
                cliffTexture = 1;
              }

              let cliffRow = this.cliffTilesets[cliffTexture];
              let dir = cliffRow.cliffModelDir;
              let path = `Doodads\\Terrain\\${dir}\\${dir}${fileName}${getCliffVariation(dir, fileName, bottomLeft.cliffVariation)}.mdx`;

              if (!cliffs[path]) {
                cliffs[path] = {locations: [], textures: []};
              }

              cliffs[path].locations.push(x * 128 + centerOffset[0], y * 128 + centerOffset[1], (base - 2) * 128);
              cliffs[path].textures.push(cliffTexture);
            }
          } else {
            let bottomLeftTexture = this.cornerTexture(x, y);
            let bottomRightTexture = this.cornerTexture(x + 1, y);
            let topLeftTexture = this.cornerTexture(x, y + 1);
            let topRightTexture = this.cornerTexture(x + 1, y + 1);
            let textures = unique([bottomLeftTexture, bottomRightTexture, topLeftTexture, topRightTexture]).sort();

            cornerTextures[instance * 4] = textures[0] + 1;
            cornerVariations[instance * 4] = this.getVariation(textures[0], bottomLeft.groundVariation);

            textures.shift();

            for (let i = 0, l = textures.length; i < l; i++) {
              let texture = textures[i];
              let bitset = 0;

              if (bottomRightTexture === texture) {
                bitset |= 0b0001;
              }

              if (bottomLeftTexture === texture) {
                bitset |= 0b0010;
              }

              if (topRightTexture === texture) {
                bitset |= 0b0100;
              }

              if (topLeftTexture === texture) {
                bitset |= 0b1000;
              }

              cornerTextures[instance * 4 + 1 + i] = texture + 1;
              cornerVariations[instance * 4 + 1 + i] = bitset;
            }
          }

          instance += 1;
        }
      }
    }

    let vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 1, 0, 0, 1, 1, 1]), gl.STATIC_DRAW);

    let faceBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, faceBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint8Array([0, 1, 2, 1, 3, 2]), gl.STATIC_DRAW);

    let cliffHeightMap = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, cliffHeightMap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, columns, rows, 0, gl.RED, gl.FLOAT, cliffHeights);

    let heightMap = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, heightMap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, columns, rows, 0, gl.RED, gl.FLOAT, cornerHeights);

    let waterHeightMap = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, waterHeightMap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R32F, columns, rows, 0, gl.RED, gl.FLOAT, waterHeights);

    let instanceBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(instanceCount).map((currentValue, index, array) => index), gl.STATIC_DRAW);

    let textureBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, textureBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, cornerTextures, gl.STATIC_DRAW);

    let variationBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, variationBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, cornerVariations, gl.STATIC_DRAW);

    let waterBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, waterBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, waterFlags, gl.STATIC_DRAW);

    let heightMapSize = new Float32Array([1 / columns, 1 / rows]);

    this.terrainRenderData = {
      rows,
      columns,
      centerOffset,
      vertexBuffer,
      faceBuffer,
      heightMap,
      instanceBuffer,
      instanceCount,
      cornerTextures,
      textureBuffer,
      variationBuffer,
      heightMapSize,
      cliffHeightMap,
      waterHeightMap,
      waterBuffer,
    };

    this.terrainReady = true;
    this.anyReady = true;

    this.createWaves();

    let cliffPromises = Object.entries(cliffs).map((cliff) => {
      let path = cliff[0];
      let {locations, textures} = cliff[1];

      return this.loadGeneric(this.mapPathSolver(path)[0], 'arrayBuffer', undefined, path)
        .whenLoaded()
        .then((resource) => {
          return new TerrainModel(gl, resource.data, locations, textures, this);
        });
    });

    this.cliffModels = await Promise.all(cliffPromises);
    this.cliffModels.sort((a, b) => !a.textures > !b.textures);
    this.cliffsReady = true;
  }

  calculateRamps() {
    let corners = this.corners;
    let [columns, rows] = this.mapSize;

    // There are no C or X ramps because they're not supported in city tileset
    let ramps = [
      "AAHL",
      "AALH",
      "ABHL",
      "AHLA",
      "ALHA",
      "ALHB",
      "BALH",
      "BHLA",
      "HAAL",
      "HBAL",
      "HLAA",
      "HLAB",
      "LAAH",
      "LABH",
      "LHAA",
      "LHBA",
    ];

    // Adjust terrain height inside ramps (set rampAdjust)
    for (let y = 1; y < rows - 1; ++y) {
      for (let x = 1; x < columns - 1; ++x) {
        let o = corners[y][x];
        if (!o.ramp) continue;
        let a = corners[y - 1][x - 1];
        let b = corners[y][x - 1];
        let c = corners[y + 1][x - 1];
        let d = corners[y + 1][x];
        let e = corners[y + 1][x + 1];
        let f = corners[y][x + 1];
        let g = corners[y - 1][x + 1];
        let h = corners[y - 1][x];
        let base = o.layerHeight;
        if ((b.ramp && f.ramp) || (d.ramp && h.ramp)) {
          let adjust = 0;
          if (b.ramp && f.ramp) {
            adjust = Math.max(adjust, (b.layerHeight + f.layerHeight) / 2 - base);
          }
          if (d.ramp && h.ramp) {
            adjust = Math.max(adjust, (d.layerHeight + h.layerHeight) / 2 - base);
          }
          if (a.ramp && e.ramp) {
            adjust = Math.max(adjust, ((a.layerHeight + e.layerHeight) / 2 - base) / 2);
          }
          if (c.ramp && g.ramp) {
            adjust = Math.max(adjust, ((c.layerHeight + g.layerHeight) / 2 - base) / 2);
          }
          o.rampAdjust = adjust;
        }
      }
    }

    // Calculate ramp tiles
    for (let y = 0; y < rows - 1; ++y) {
      for (let x = 0; x < columns - 1; ++x) {
        let a = corners[y][x];
        let b = corners[y + 1][x];
        let c = corners[y][x + 1];
        let d = corners[y + 1][x + 1];
        if (!a.rampType && y < rows - 2) {
          let e = corners[y + 2][x];
          let f = corners[y + 2][x + 1];
          let ae = Math.min(a.layerHeight, e.layerHeight), cf = Math.min(c.layerHeight, f.layerHeight);
          if (b.layerHeight === ae && d.layerHeight === cf) {
            let base = Math.min(ae, cf);
            if (a.ramp === b.ramp && a.ramp === e.ramp && c.ramp === d.ramp && c.ramp === f.ramp && a.ramp !== c.ramp) {
              let name = this.rampFileName(e, f, c, a, base);
              if (ramps.includes(name)) {
                a.rampName = name;
                a.rampType = 1;
                b.rampType = -1;
              }
            }
          }
        }
        if (!a.rampType && x < columns - 2) {
          let e = corners[y][x + 2];
          let f = corners[y + 1][x + 2];
          let ae = Math.min(a.layerHeight, e.layerHeight), bf = Math.min(b.layerHeight, f.layerHeight);
          if (c.layerHeight === ae && d.layerHeight === bf) {
            let base = Math.min(ae, bf);
            if (a.ramp === c.ramp && a.ramp === e.ramp && b.ramp === d.ramp && b.ramp === f.ramp && a.ramp !== b.ramp) {
              let name = this.rampFileName(b, f, e, a, base);
              if (ramps.includes(name)) {
                a.rampName = name;
                a.rampType = 2;
                c.rampType = -1;
              }
            }
          }
        }
      }
    }

    // Calculate cliff tiles
    for (let y = 0; y < rows - 1; ++y) {
      for (let x = 0; x < columns - 1; ++x) {
        let a = corners[y][x];
        let b = corners[y + 1][x];
        let c = corners[y][x + 1];
        let d = corners[y + 1][x + 1];
        if (a.rampAdjust || b.rampAdjust || c.rampAdjust || d.rampAdjust) {
          continue;
        }
        let base = a.layerHeight;
        if (b.layerHeight !== base || c.layerHeight !== base || d.layerHeight !== base) {
          a.cliff = true;
        }
      }
    }

    // Fix ramp cliffTexture
    for (let y = 0; y < rows - 1; ++y) {
      for (let x = 0; x < columns - 1; ++x) {
        let a = corners[y][x], b;
        if (a.rampType && a.rampType > 0) {
          let x1 = x + 1, y1 = y + 1;
          if (a.rampType === 1) {
            b = corners[y + 1][x];
            y1 = y + 2;
          } else {
            b = corners[y][x + 1];
            x1 = x + 2;
          }
          let x0 = Math.max(0, x - 1), y0 = Math.max(0, y - 1);
          x1 = Math.min(x1, columns - 1);
          y1 = Math.min(y1, rows - 1);
          let tex = null;
          for (let ty = y0; ty <= y1 && tex == null; ++ty) {
            for (let tx = x0; tx <= x1; ++tx) {
              let tile = corners[ty][tx];
              if (tile.cliff && !tile.rampType) {
                tex = tile.cliffTexture;
                break;
              }
            }
          }
          if (tex != null) {
            a.cliffTexture = b.cliffTexture = tex;
          }
        }
      }
    }
  }

  /**
   * @param {number} bottomLeftLayer
   * @param {number} bottomRightLayer
   * @param {number} topLeftLayer
   * @param {number} topRightLayer
   * @param {number} base
   * @return {string}
   */
  cliffFileName(bottomLeftLayer, bottomRightLayer, topLeftLayer, topRightLayer, base) {
    return String.fromCharCode(65 + topLeftLayer - base) +
      String.fromCharCode(65 + topRightLayer - base) +
      String.fromCharCode(65 + bottomRightLayer - base) +
      String.fromCharCode(65 + bottomLeftLayer - base);
  }

  rampFileName(a, b, c, d, base) {
    let ah = a.layerHeight - base, bh = b.layerHeight - base, ch = c.layerHeight - base, dh = d.layerHeight - base;
    if (ah > 2 || bh > 2 || ch > 2 || dh > 2) {
      return "";
    }
    let ramp = "LHX", cliff = "ABC";
    return (a.ramp ? ramp : cliff)[ah] + (b.ramp ? ramp : cliff)[bh] + (c.ramp ? ramp : cliff)[ch] + (d.ramp ? ramp : cliff)[dh];
  }

  /**
   * Creates a shared texture that holds all of the tileset textures.
   * Each tileset is flattend to a single row of tiles, such that indices 0-15 are the normal part, and indices 16-31 are the extended part.
   */
  createTilesetsAndWaterTextures() {
    let tilesets = this.tilesetTextures;
    let tilesetsCount = tilesets.length;
    let canvas = document.createElement('canvas');
    let ctx = canvas.getContext('2d');

    canvas.width = 2048;
    canvas.height = 64 * (tilesetsCount + 1); // 1 is added for a black tileset, to remove branches from the fragment shader, at the cost of 512Kb.

    for (let tileset = 0; tileset < tilesetsCount; tileset++) {
      let imageData = tilesets[tileset].imageData;
      if (!imageData) {
        continue;
      }

      for (let variation = 0; variation < 16; variation++) {
        let x = (variation % 4) * 64;
        let y = ((variation / 4) | 0) * 64;

        ctx.putImageData(imageData, variation * 64 - x, (tileset + 1) * 64 - y, x, y, 64, 64);
      }

      if (imageData.width === 512) {
        for (let variation = 0; variation < 16; variation++) {
          let x = 256 + (variation % 4) * 64;
          let y = ((variation / 4) | 0) * 64;

          ctx.putImageData(imageData, 1024 + variation * 64 - x, (tileset + 1) * 64 - y, x, y, 64, 64);
        }
      }
    }

    let gl = this.gl;
    let texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas);

    this.tilesetsTexture = texture;

    canvas.height = 128 * 3; // up to 48 frames.

    let waterTextures = this.waterTextures;

    for (let i = 0, l = waterTextures.length; i < l; i++) {
      let x = i % 16;
      let y = (i / 16) | 0;

      ctx.putImageData(waterTextures[i].imageData, x * 128, y * 128);
    }

    let waterTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, waterTexture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas);

    this.waterTexture = waterTexture;
  }

  /**
   * @param {number} groundTexture
   * @param {number} variation
   * @return {number}
   */
  getVariation(groundTexture, variation) {
    let texture = this.tilesetTextures[groundTexture];

    // Extended?
    if (texture.width > texture.height) {
      if (variation < 16) {
        return 16 + variation;
      } else if (variation === 16) {
        return 15;
      } else {
        return 0;
      }
    } else {
      if (variation === 0) {
        return 0;
      } else {
        return 15;
      }
    }
  }

  /**
   * Is the corner at the given column and row a cliff?
   *
   * @param {number} column
   * @param {number} row
   * @return {boolean}
   */
  isCliff(column, row) {
    return !!this.corners[row][column].cliff;
  }

  /**
   * Is the tile at the given column and row water?
   *
   * @param {number} column
   * @param {number} row
   * @return {boolean}
   */
  isWater(column, row) {
    let corners = this.corners;

    return corners[row][column].water || corners[row][column + 1].water || corners[row + 1][column].water || corners[row + 1][column + 1].water;
  }

  /**
   * Given a cliff index, get its ground texture index.
   * This is an index into the tilset textures.
   *
   * @param {number} whichCliff
   * @return {number}
   */
  cliffGroundIndex(whichCliff) {
    let whichTileset = this.cliffTilesets[whichCliff].groundTile;
    let tilesets = this.tilesets;

    for (let i = 0, l = tilesets.length; i < l; i++) {
      if (tilesets[i].tileID === whichTileset) {
        return i;
      }
    }
  }

  /**
   * Get the ground texture of a corner, whether it's normal ground, a cliff, or a blighted corner.
   *
   * @param {number} column
   * @param {number} row
   * @return {number}
   */
  cornerTexture(column, row) {
    let corners = this.corners;
    let columns = this.columns;
    let rows = this.rows;

    for (let y = -1; y < 1; y++) {
      for (let x = -1; x < 1; x++) {
        if (column + x > 0 && column + x < columns - 1 && row + y > 0 && row + y < rows - 1) {
          let corner = corners[row + y][column + x];
          if (this.isCliff(column + x, row + y) || corner.rampType) {
            let texture = corner.cliffTexture;

            if (texture === 15) {
              texture = 1;
            }

            return this.cliffGroundIndex(texture);
          }
        }
      }
    }

    let corner = corners[row][column];

    // Is this corner blighted?
    if (corner.blight) {
      return this.blightTextureIndex;
    }

    return corner.groundTexture;
  }

  load(src) {
    return super.load(src, this.mapPathSolver);
  }

  /**
   * 
   * @param {*} dataMap 
   * @param {*} metadataMap 
   * @param {*} modificationFile 
   */
  applyModificationFile(dataMap, metadataMap, modificationFile) {
    if (modificationFile) {
      // Modifications to built-in objects
      this.applyModificationTable(dataMap, metadataMap, modificationFile.originalTable);

      // Declarations of user-defined objects
      this.applyModificationTable(dataMap, metadataMap, modificationFile.customTable);
    }
  }

  /**
   * 
   * @param {*} dataMap 
   * @param {*} metadataMap 
   * @param {*} modificationTable 
   */
  applyModificationTable(dataMap, metadataMap, modificationTable) {
    for (let modificationObject of modificationTable.objects) {
      let row = dataMap.getRow(modificationObject.oldId);
      let newId = modificationObject.newId;

      // If this is a custom object, and it's not in the mapped data, copy the standard object.
      if (modificationObject.newId !== '' && !dataMap.getRow(newId)) {
        dataMap.setRow(modificationObject.newId, row = {...row});
      }

      for (let modification of modificationObject.modifications) {
        let metadata = metadataMap.getRow(modification.id);

        if (metadata) {
          row[metadata.field] = modification.value;
        } else {
          console.warn('Unknown modification ID', modification.id);
        }
      }
    }
  }

  /**
   * 
   * @param {Float32Array} out
   * @param {number} x
   * @param {number} y
   * @return {out}
   */
  groundNormal(out, x, y) {
    let centerOffset = this.centerOffset;
    let mapSize = this.mapSize;

    x = (x - centerOffset[0]) / 128;
    y = (y - centerOffset[1]) / 128;

    let cellX = x | 0;
    let cellY = y | 0;

    // See if this coordinate is in the map
    if (cellX >= 0 && cellX < mapSize[0] - 1 && cellY >= 0 && cellY < mapSize[1] - 1) {
      // See http://gamedev.stackexchange.com/a/24574
      let corners = this.corners;
      let bottomLeft = corners[cellY][cellX].groundHeight;
      let bottomRight = corners[cellY][cellX + 1].groundHeight;
      let topLeft = corners[cellY + 1][cellX].groundHeight;
      let topRight = corners[cellY + 1][cellX + 1].groundHeight;
      let sqX = x - cellX;
      let sqY = y - cellY;

      if (sqX + sqY < 1) {
        vec3.set(normalHeap1, 1, 0, bottomRight - bottomLeft);
        vec3.set(normalHeap2, 0, 1, topLeft - bottomLeft);
      } else {
        vec3.set(normalHeap1, -1, 0, topRight - topLeft);
        vec3.set(normalHeap2, 0, 1, topRight - bottomRight);
      }

      vec3.normalize(out, vec3.cross(out, normalHeap1, normalHeap2));
    } else {
      vec3.set(out, 0, 0, 1);
    }

    return out;
  }

  heightAt(x, y) {
    let centerOffset = this.centerOffset;
    let mapSize = this.mapSize;

    x = (x - centerOffset[0]) / 128;
    y = (y - centerOffset[1]) / 128;

    let cellX = x | 0;
    let cellY = y | 0;

    if (cellX >= 0 && cellX < mapSize[0] - 1 && cellY >= 0 && cellY < mapSize[1] - 1) {
      let corners = this.corners;
      const ht = c => c.groundHeight + c.layerHeight - 2;
      let bottomLeft = ht(corners[cellY][cellX]);
      let bottomRight = ht(corners[cellY][cellX + 1]);
      let topLeft = ht(corners[cellY + 1][cellX]);
      let topRight = ht(corners[cellY + 1][cellX + 1]);
      let sqX = x - cellX;
      let sqY = y - cellY;
      let height;

      if (sqX + sqY < 1) {
        height = bottomLeft + (bottomRight - bottomLeft) * sqX + (topLeft - bottomLeft) * sqY;
      } else {
        height = topRight + (bottomRight - topRight) * (1 - sqY) + (topLeft - topRight) * (1 - sqX);
      }

      return height * 128;
    }

    return 0;
  }

  inPlayableArea(x, y) {
    x = (x - this.centerOffset[0]) / 128.0;
    y = (y - this.centerOffset[1]) / 128.0;
    if (x < this.mapBounds[0]) return false;
    if (x >= this.mapSize[0] - this.mapBounds[1] - 1) return false;
    if (y < this.mapBounds[2]) return false;
    if (y >= this.mapSize[1] - this.mapBounds[3] - 1) return false;
    return !this.corners[Math.floor(y)][Math.floor(x)].boundary;
  }

  addShadow(file, x, y) {
    if (!this.shadows[file]) {
      let path = `ReplaceableTextures\\Shadows\\${file}.blp`;
      this.shadows[file] = [];
      this.shadowTextures[file] = this.load(path);
    }
    this.shadows[file].push({x, y});
  }

  async initShadows() {
    await this.whenLoaded(Object.values(this.shadowTextures));

    let gl = this.gl;
    let centerOffset = this.centerOffset;
    let [columns, rows] = this.mapSize;
    columns = (columns - 1) * 4;
    rows = (rows - 1) * 4;

    const shadowSize = columns * rows;
    const shadowData = new Uint8Array(columns * rows);
    const source = this.mapMpq.get('war3map.shd');
    if (source) {
      let buffer = source.arrayBuffer();
      if (buffer.byteLength >= shadowSize) {
        buffer = new Uint8Array(buffer);
        for (let i = 0; i < shadowSize; ++i) {
          shadowData[i] = buffer[i] / 2;
        }
      }
    }

    for (let [file, texture] of Object.entries(this.shadowTextures)) {
      if (!texture.originalData) {
        continue;
      }
      let {data, width, height} = texture.originalData;
      let ox = Math.round(width * 0.3), oy = Math.round(height * 0.7);

      for (let location of this.shadows[file]) {
        let x0 = Math.floor((location.x - centerOffset[0]) / 32.0) - ox;
        let y0 = Math.floor((location.y - centerOffset[1]) / 32.0) + oy;
        for (let y = 0; y < height; ++y) {
          if (y0 - y < 0 || y0 - y >= rows) {
            continue;
          }
          for (let x = 0; x < width; ++x) {
            if (x0 + x < 0 || x0 + x >= columns) {
              continue;
            }
            if (data[(y * width + x) * 4 + 3]) {
              shadowData[(y0 - y) * columns + x0 + x] = 128;
            }
          }
        }
      }
    }

    const outsideArea = 204;
    let x0 = this.mapBounds[0] * 4, x1 = (this.mapSize[0] - this.mapBounds[1] - 1) * 4,
        y0 = this.mapBounds[2] * 4, y1 = (this.mapSize[1] - this.mapBounds[3] - 1) * 4;
    for (let y = y0; y < y1; ++y) {
      for (let x = x0; x < x1; ++x) {
        let c = this.corners[y >> 2][x >> 2];
        if (c.boundary) {
          shadowData[y * columns + x] = outsideArea;
        }
      }
    }
    for (let y = 0; y < rows; ++y) {
      for (let x = 0; x < x0; ++x) {
        shadowData[y * columns + x] = outsideArea;
      }
      for (let x = x1; x < columns; ++x) {
        shadowData[y * columns + x] = outsideArea;
      }
    }
    for (let x = x0; x < x1; ++x) {
      for (let y = 0; y < y0; ++y) {
        shadowData[y * columns + x] = outsideArea;
      }
      for (let y = y1; y < rows; ++y) {
        shadowData[y * columns + x] = outsideArea;
      }
    }

    let shadowMap = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, shadowMap);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R8, columns, rows, 0, gl.RED, gl.UNSIGNED_BYTE, shadowData);
    this.shadowMap = shadowMap;

    this.shadowsReady = true;
  }

  createWaves() {
    let [columns, rows] = this.mapSize;
    let corners = this.corners;
    let wavesDepth = 25 / 128;
    let centerOffset = this.centerOffset;
    let waterRow = this.waterData.getRow(`${this.tileset}Sha`);

    let wavesCliff = (this.mapFlags & 0x0800);
    let wavesRolling = (this.mapFlags & 0x1000);

    let shoreline = `${waterRow.shoreDir}\\${waterRow.shoreSFile}\\${waterRow.shoreSFile}0.mdx`;
    let outsideCorner = `${waterRow.shoreDir}\\${waterRow.shoreOCFile}\\${waterRow.shoreOCFile}0.mdx`;
    let insideCorner = `${waterRow.shoreDir}\\${waterRow.shoreICFile}\\${waterRow.shoreICFile}0.mdx`;

    let rotation = (a, b, c) => {
      if (a) return -3 * Math.PI / 4;
      if (b) return -Math.PI / 4;
      if (c) return Math.PI / 4;
      return 3 * Math.PI / 4;
    };
    let rotation2 = (a, b, c, d) => {
      if (a && b) return -Math.PI / 2;
      if (b && c) return 0;
      if (c && d) return Math.PI / 2;
      if (a && d) return Math.PI;
      return null;
    };

    let location = new Float32Array(3);
    let models = {};

    let addInstance = (path, rotation) => {
      if (!models[path]) {
        models[path] = this.load(path);
      }
      let instance = models[path].addInstance();
      instance.move(location);
      instance.rotateLocal(quat.setAxisAngle(quat.create(), VEC3_UNIT_Z, rotation));
      instance.setScene(this.scene);
      if (!this.inPlayableArea(location[0], location[1])) {
        instance.setVertexColor([51, 51, 51]);
      }
      standOnRepeat(instance);
    };

    for (let y = 0; y < rows - 1; ++y) {
      for (let x = 0; x < columns - 1; ++x) {
        let a = corners[y][x], b = corners[y][x + 1], c = corners[y + 1][x + 1], d = corners[y + 1][x];
        if (a.water || b.water || c.water || c.water) {
          let isCliff = (a.layerHeight !== b.layerHeight || a.layerHeight !== c.layerHeight || a.layerHeight !== d.layerHeight);
          if (isCliff && !wavesCliff) {
            continue;
          }
          if (!isCliff && !wavesRolling) {
            continue;
          }
          let ad = (a.depth > wavesDepth ? 1 : 0);
          let bd = (b.depth > wavesDepth ? 1 : 0);
          let cd = (c.depth > wavesDepth ? 1 : 0);
          let dd = (d.depth > wavesDepth ? 1 : 0);
          let count = (ad + bd + cd + dd);
          location[0] = x * 128.0 + centerOffset[0] + 64.0;
          location[1] = y * 128.0 + centerOffset[1] + 64.0;
          location[2] = ((a.waterHeight + b.waterHeight + c.waterHeight + d.waterHeight) / 4.0 + this.waterHeightOffset) * 128.0 + 1.0;
          if (count === 1) {
            addInstance(insideCorner, rotation(ad, bd, cd, dd));
          } else if (count === 2) {
            let rot = rotation2(ad, bd, cd, dd);
            if (rot != null) {
              addInstance(shoreline, rot);
            }
          } else if (count === 3) {
            addInstance(outsideCorner, rotation(!ad, !bd, !cd, !dd) + Math.PI);
          }
        }
      }
    }
  }

  deselect() {
    if (this.selModels && this.selModels.length) {
      this.selModels.forEach(model => {
        let idx = this.uberSplatModels.indexOf(model);
        if (idx >= 0) {
          this.uberSplatModels.splice(idx, 1);
        }
      });
      this.selModels = [];
    }
    this.selected = [];
  }

  doSelectUnits(units) {
    this.deselect();
    if (!units.length) {
      return;
    }

    let splats = {};

    this.selected = units.filter(({unit}) => {
      let row = this.unitsData.find(row => row.id === unit.id);
      if (!row) return false;
      row = row.data;
      let selScale = parseFloat(row.scale || "0");
      if (!selScale) return false;

      let radius = 36 * selScale, path;
      if (radius < 100) {
        path = 'ReplaceableTextures\\Selection\\SelectionCircleSmall.blp';
      } if (radius < 300) {
        path = 'ReplaceableTextures\\Selection\\SelectionCircleMed.blp';
      } else {
        path = 'ReplaceableTextures\\Selection\\SelectionCircleLarge.blp';
      }
      if (!splats[path]) {
        splats[path] = [];
      }
      let [x, y] = unit.location;
      let z = parseFloat(row.selZ || "0");
      splats[path].push(x - radius, y - radius, x + radius, y + radius, z + 5);

      return true;
    });

    let models = this.selModels = [];
    Object.entries(splats).forEach(([path, locations]) => {
      this.load(path).whenLoaded().then(tex => {
        if (this.selModels !== models) return;
        let model = new SplatModel(this.gl, tex, locations, this.centerOffset);
        model.color = [0, 1, 0, 1];
        this.selModels.push(model);
        this.uberSplatModels.push(model);
      });
    });
  }

  selectUnit(x, y, toggle) {
    let ray = new Float32Array(6);
    this.camera.screenToWorldRay(ray, [x, y]);
    let dir = normalHeap1;
    dir[0] = ray[3] - ray[0];
    dir[1] = ray[4] - ray[1];
    dir[2] = ray[5] - ray[2];
    vec3.normalize(dir, dir);
    let eMid = vec3.create(), eSize = vec3.create(), rDir = vec3.create();

    let entity = null;
    let entDist = 1e+6;
    this.units.forEach(ent => {
      let {instance, location, radius} = ent;
      vec3.set(eMid, 0, 0, radius / 2);
      vec3.set(eSize, radius, radius, radius);

      vec3.add(eMid, eMid, location);
      vec3.sub(eMid, eMid, ray);
      vec3.div(eMid, eMid, eSize);
      vec3.div(rDir, dir, eSize);
      let dlen = vec3.sqrLen(rDir);

      let dp = Math.max(0, vec3.dot(rDir, eMid)) / dlen;
      if (dp > entDist) return;
      vec3.scale(rDir, rDir, dp);
      if (vec3.sqrDist(rDir, eMid) < 1.0) {
        entity = ent;
        entDist = dp;
      }
    });
    let sel = [];
    if (entity) {
      if (toggle) {
        sel = this.selected.slice();
        const idx = sel.indexOf(entity);
        if (idx >= 0) {
          sel.splice(idx, 1);
        } else {
          sel.push(entity);
        }
      } else {
        sel = [entity];
      }
    }
    this.doSelectUnits(sel);
    return sel;
  }

  selectUnits(x0, y0, x1, y1) {
    let sp = new Float32Array(2);
    let sel = this.units.filter(ent => {
      let {location, radius, unit} = ent;
      let row = this.unitsData.find(row => row.id === unit.id);
      if (!row) return false;
      row = row.data;
      let [x, y] = unit.location;
      let z = parseFloat(row.selZ || "0") + this.heightAt(x, y);
      this.camera.worldToScreen(sp, [x, y, z]);
      if (sp[0] >= x0 && sp[0] < x1 && sp[1] >= y0 && sp[1] < y1) {
        return true;
      }
      return false;
    });
    this.doSelectUnits(sel);
    return sel;
  }
}

/*
  */
