export class TextureAtlasWebGL1 {
  constructor(gl, width, height, count) {
    this.gl = gl;
    this.width = width;
    this.height = height;
    this.grid_width = 1;
    while (this.grid_width * this.grid_width < count) {
      this.grid_width *= 2;
    }
    this.grid_height = Math.ceil(count / this.grid_height);
    this.count = count;
    this.texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, this.width * this.grid_width, this.height * this.grid_height,
      0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  }
  bind() {
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    return this;
  }
  setParameters(wrapS, wrapT, magFilter, minFilter) {
    const gl = this.gl;
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, wrapS);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, wrapT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, magFilter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, minFilter);
    return this;
  }
  addImage(index, data) {
    const gl = this.gl;
    const cx = (index % this.grid_width);
    const cy = Math.floor(index / this.grid_width);
    if (ArrayBuffer.isView(data)) {
      gl.texSubImage2D(gl.TEXTURE_2D, 0, this.width * cx, this.height * cy, this.width, this.height, gl.RGBA, gl.UNSIGNED_BYTE, data);
    } else {
      gl.texSubImage2D(gl.TEXTURE_2D, 0, this.width * cx, this.height * cy, gl.RGBA, gl.UNSIGNED_BYTE, data);
    }
    return this;
  }
  finish() {
    return this;
  }
}

export class TextureAtlasWebGL2 {
  constructor(gl, width, height, count) {
    this.gl = gl;
    this.width = width;
    this.height = height;
    this.count = count;
    this.texture = gl.createTexture();
    let mips = Math.floor(Math.log(Math.max(width, height)) * Math.LOG2E) + 1;
    gl.bindTexture(gl.TEXTURE_2D_ARRAY, this.texture);
    gl.texStorage3D(gl.TEXTURE_2D_ARRAY, mips, gl.RGBA, this.width, this.height, count);
  }
  bind() {
    gl.bindTexture(gl.TEXTURE_2D_ARRAY, this.texture);
    return this;
  }
  setParameters(wrapS, wrapT, magFilter, minFilter) {
    const gl = this.gl;
    gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_WRAP_S, wrapS);
    gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_WRAP_T, wrapT);
    gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_MAG_FILTER, magFilter);
    gl.texParameteri(gl.TEXTURE_2D_ARRAY, gl.TEXTURE_MIN_FILTER, minFilter);
    return this;
  }
  addImage(index, data) {
    const gl = this.gl;
    gl.texSubImage3D(gl.TEXTURE_2D_ARRAY, 0, 0, 0, index, this.width, this.height, 1, gl.RGBA, gl.UNSIGNED_BYTE, data);
    return this;
  }
  finish() {
    const gl = this.gl;
    gl.generateMipmap(gl.TEXTURE_2D_ARRAY);
    return this;
  }
}
