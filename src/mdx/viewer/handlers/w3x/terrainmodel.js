import MdxParser from '../../../parsers/mdlx/model';

/**
 * A static terrain model.
 */
export default class TerrainModel {
  /**
   * @param {WebGLRenderingContext} gl
   * @param {ArrayBuffer} arrayBuffer
   * @param {Array<number>} locations
   * @param {Array<number>} textures
   */
  constructor(gl, arrayBuffer, locations, textures, loader) {
    let parser = new MdxParser(arrayBuffer);
    let geoset = parser.geosets[0];
    let vertices = geoset.vertices;
    let normals = geoset.normals;
    let uvs = geoset.uvSets[0];
    let uvs2 = geoset.uvSets[1];
    let faces = geoset.faces;
    let normalsOffset = vertices.byteLength;
    let uvsOffset = normalsOffset + normals.byteLength;
    let uvs2Offset = uvsOffset + uvs.byteLength;
    let totalLength = uvs2Offset + (uvs2 ? uvs2.byteLength : 0);
    if (!uvs2) {
      uvs2Offset = uvsOffset;
    }
    let instances = locations.length / 3;

    if (!textures) {
      this.textures = parser.textures.map(tex => tex.path ? loader.load(tex.path) : null);
      textures = new Uint8Array(instances);
      if (uvs2 && parser.textures.length >= 2) {
        for (let i = 0; i < instances; ++i) {
          textures[i] = 2;
        }
      }
    }


    let vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, totalLength, gl.STATIC_DRAW);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, vertices);
    gl.bufferSubData(gl.ARRAY_BUFFER, normalsOffset, normals);
    gl.bufferSubData(gl.ARRAY_BUFFER, uvsOffset, uvs);
    if (uvs2) gl.bufferSubData(gl.ARRAY_BUFFER, uvs2Offset, uvs2);

    let faceBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, faceBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, faces, gl.STATIC_DRAW);

    let texturesOffset = locations.length * 4;
    let locationAndTextureBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, locationAndTextureBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, texturesOffset + instances, gl.STATIC_DRAW);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Float32Array(locations));
    gl.bufferSubData(gl.ARRAY_BUFFER, texturesOffset, new Uint8Array(textures));

    /** @member {WebGLBuffer} */
    this.vertexBuffer = vertexBuffer;
    /** @member {WebGLBuffer} */
    this.faceBuffer = faceBuffer;
    /** @member {number} */
    this.normalsOffset = normalsOffset;
    /** @member {number} */
    this.uvsOffset = uvsOffset;
    /** @member {number} */
    this.uvs2Offset = uvs2Offset;
    /** @member {number} */
    this.elements = faces.length;
    /** @member {WebGLBuffer} */
    this.locationAndTextureBuffer = locationAndTextureBuffer;
    /** @member {number} */
    this.texturesOffset = texturesOffset;
    /** @member {number} */
    this.instances = instances;
  }

  /**
   * @param {WebGLRenderingContext} gl
   * @param {ANGLEInstancedArrays} instancedArrays
   * @param {Object} attribs
   */
  render(gl, instancedArrays, attribs, tex) {
    if (this.textures) {
      for (let i = 0; i < this.textures.length && i < 2; ++i) {
        gl.activeTexture(gl.TEXTURE1 + i);
        if (this.textures[i]) {
          if (!this.textures[i].webglResource) {
            return;
          }
          gl.bindTexture(gl.TEXTURE_2D, this.textures[i].webglResource);
        } else {
          gl.bindTexture(gl.TEXTURE_2D, tex[i].webglResource);
        }
      }
    }

    // Locations and textures.
    gl.bindBuffer(gl.ARRAY_BUFFER, this.locationAndTextureBuffer);
    gl.vertexAttribPointer(attribs.a_instancePosition, 3, gl.FLOAT, false, 12, 0);
    gl.vertexAttribPointer(attribs.a_instanceTexture, 1, gl.UNSIGNED_BYTE, false, 1, this.texturesOffset);

    // Vertices.
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
    gl.vertexAttribPointer(attribs.a_position, 3, gl.FLOAT, false, 12, 0);
    gl.vertexAttribPointer(attribs.a_normal, 3, gl.FLOAT, false, 12, this.normalsOffset);
    gl.vertexAttribPointer(attribs.a_uv1, 2, gl.FLOAT, false, 8, this.uvsOffset);
    gl.vertexAttribPointer(attribs.a_uv2, 2, gl.FLOAT, false, 8, this.uvs2Offset);

    // Faces.
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.faceBuffer);

    // Draw.
    instancedArrays.drawElementsInstancedANGLE(gl.TRIANGLES, this.elements, gl.UNSIGNED_SHORT, 0, this.instances);
  }
}
