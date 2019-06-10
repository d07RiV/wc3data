/**
 * A static terrain model.
 */
export default class SplatModel {
  /**
   * @param {WebGLRenderingContext} gl
   * @param {ArrayBuffer} arrayBuffer
   * @param {Array<number>} locations
   * @param {Array<number>} textures
   */
  constructor(gl, texture, locations, corners, centerOffset) {
    this.texture = texture;
    this.batches = [];

    const MaxVertices = 65000;

    let vertices = [];
    let uvs = [];
    let indices = [];
    let instances = locations.length / 5;
    for (let idx = 0; idx < instances; ++idx) {
      let [x0, y0, x1, y1, opacity] = locations.slice(idx * 5, idx * 5 + 5);

      let ix0 = Math.floor((x0 - centerOffset[0]) / 128.0);
      let ix1 = Math.ceil((x1 - centerOffset[0]) / 128.0);
      let iy0 = Math.floor((y0 - centerOffset[1]) / 128.0);
      let iy1 = Math.ceil((y1 - centerOffset[1]) / 128.0);

      let newVerts = (iy1 - iy0 + 1) * (ix1 - ix0 + 1);
      if (newVerts > MaxVertices) {
        continue;
      }

      let start = vertices.length / 2, step = ix1 - ix0 + 1;
      if (start + newVerts > MaxVertices) {
        this.addBatch(gl, vertices, uvs, indices);
        vertices.length = 0;
        uvs.length = 0;
        indices.length = 0;
        start = 0;
      }

      for (let iy = iy0; iy <= iy1; ++iy) {
        let y = iy * 128.0 + centerOffset[1];
        for (let ix = ix0; ix <= ix1; ++ix) {
          let x = ix * 128.0 + centerOffset[0];
          vertices.push(x, y);
          uvs.push((x - x0) / (x1 - x0), 1.0 - (y - y0) / (y1 - y0), opacity);
        }
      }
      for (let i = 0; i < iy1 - iy0; ++i) {
        for (let j = 0; j < ix1 - ix0; ++j) {
          let i0 = start + i * step + j;
          indices.push(i0, i0 + 1, i0 + step, i0 + 1, i0 + step + 1, i0 + step);
        }
      }
    }

    if (indices.length) {
      this.addBatch(gl, vertices, uvs, indices);
    }
  }

  addBatch(gl, vertices, uvs, indices) {
    let uvsOffset = vertices.length * 4;

    let vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, uvsOffset + uvs.length * 4, gl.STATIC_DRAW);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, new Float32Array(vertices));
    gl.bufferSubData(gl.ARRAY_BUFFER, uvsOffset, new Float32Array(uvs));

    let faceBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, faceBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

    this.batches.push({
      uvsOffset,
      vertexBuffer,
      faceBuffer,
      elements: indices.length,
    });
  }

  /**
   * @param {WebGLRenderingContext} gl
   * @param {Object} attribs
   */
  render(gl, attribs) {
    // Texture
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, this.texture.webglResource);

    for (let b of this.batches) {
      // Vertices
      gl.bindBuffer(gl.ARRAY_BUFFER, b.vertexBuffer);
      gl.vertexAttribPointer(attribs.a_position, 2, gl.FLOAT, false, 8, 0);
      gl.vertexAttribPointer(attribs.a_uv, 3, gl.FLOAT, false, 12, b.uvsOffset);

      // Faces.
      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, b.faceBuffer);

      // Draw.
      gl.drawElements(gl.TRIANGLES, b.elements, gl.UNSIGNED_SHORT, 0);
    }
  }
}
