import shaders from '../../shaders';

export default {
  vsGround: `
    uniform mat4 u_mvp;
    uniform sampler2D u_heightMap;
    uniform sampler2D u_heightMap0;
    uniform vec2 u_size;
    uniform vec2 u_offset;
    uniform float u_tilesetHeight;
    uniform float u_tilesetCount;
    uniform vec2 u_pixel;

    attribute vec2 a_position;
    ${shaders.instanceId}
    attribute vec4 a_textures;
    attribute vec4 a_variations;

    varying vec2 v_uv[4];
    varying vec2 v_suv;
    varying vec3 v_normal;

    vec4 getCell(float tileset, float variation) {
      float x = variation * 0.03125;
      float y = tileset * u_tilesetHeight;

      return vec4(x, y, x + 0.03125, y + u_tilesetHeight);
    }
    
    vec2 getUV(vec2 position, float tileset, float variation) {
      vec2 uv = vec2(position.x, 1.0 - position.y);
      vec4 cell = getCell(tileset, variation);
      vec2 cellSize = vec2(1.0 / 32.0, 1.0 / u_tilesetCount);
      vec2 pixelSize = vec2(2.0 / 2048.0, 2.0 / (64.0 * u_tilesetCount));

      return clamp(cell.xy + uv * cellSize, cell.xy + pixelSize, cell.zw - pixelSize);
    }

    const float normalDist = 1.0;

    void main() {
      if (a_textures[0] > 0.5) {
        v_uv[0] = getUV(a_position, a_textures[0], a_variations[0]);
        v_uv[1] = getUV(a_position, a_textures[1], a_variations[1]);
        v_uv[2] = getUV(a_position, a_textures[2], a_variations[2]);
        v_uv[3] = getUV(a_position, a_textures[3], a_variations[3]);

        vec2 halfPixel = vec2(0.5, 0.5) * u_pixel;
        vec2 corner = vec2(mod(a_InstanceID, u_size.x), floor(a_InstanceID / u_size.x));
        vec2 base = floor(corner + a_position);
        float height = texture2D(u_heightMap, base * u_pixel + halfPixel).a;

        float hL = texture2D(u_heightMap0, vec2(base - vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
        float hR = texture2D(u_heightMap0, vec2(base + vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
        float hD = texture2D(u_heightMap0, vec2(base - vec2(0.0, normalDist)) * u_pixel + halfPixel).a;
        float hU = texture2D(u_heightMap0, vec2(base + vec2(0.0, normalDist)) * u_pixel + halfPixel).a;

        v_normal = normalize(vec3(hL - hR, hD - hU, normalDist * 2.0));

        v_suv = base / u_size;
        gl_Position = u_mvp * vec4(base * 128.0 + u_offset, height * 128.0, 1.0);
      } else {
        v_uv[0] = vec2(0.0);
        v_uv[1] = vec2(0.0);
        v_uv[2] = vec2(0.0);
        v_uv[3] = vec2(0.0);
        v_suv = vec2(0.0);

        v_normal = vec3(0.0);

        gl_Position = vec4(0.0);
      }
    }
  `,
  fsGround: `
    uniform sampler2D u_tilesets;
    uniform sampler2D u_shadowMap;

    varying vec2 v_uv[4];
    varying vec2 v_suv;
    varying vec3 v_normal;

    const vec3 lightDirection = normalize(vec3(-0.3, -0.3, 0.25));

    vec4 blend(vec4 color, vec2 uv) {
      vec4 texel = texture2D(u_tilesets, uv).rgba;

      return mix(color, texel, texel.a);
    }

    void main() {
      vec4 color = texture2D(u_tilesets, v_uv[0]).rgba;
      color = blend(color, v_uv[1]);
      color = blend(color, v_uv[2]);
      color = blend(color, v_uv[3]);

      float shadow = texture2D(u_shadowMap, v_suv).a;
      color.xyz *= clamp(dot(v_normal, lightDirection) + 0.45, 0.0, 1.0);
      color.xyz *= 1.0 - shadow;

      gl_FragColor = color;
    }
  `,
  vsWater: `
    uniform mat4 u_mvp;
    uniform sampler2D u_heightMap;
    uniform sampler2D u_waterHeightMap;
    uniform vec2 u_size;
    uniform vec2 u_pixel;
    uniform vec2 u_offset;
    uniform float u_offsetHeight;
    uniform float u_tileIndex;
    uniform vec4 u_minDeepColor;
    uniform vec4 u_maxDeepColor;
    uniform vec4 u_minShallowColor;
    uniform vec4 u_maxShallowColor;

    attribute vec2 a_position;
    ${shaders.instanceId}
    attribute float a_isWater;

    varying vec2 v_uv;
    varying vec2 v_position;
    varying vec4 v_color;

    const float minDepth = 10.0 / 128.0;
    const float deepLevel = 64.0 / 128.0;
    const float maxDepth = 72.0 / 128.0;

    void main() {
      if (a_isWater > 0.5) {
        vec2 corner = vec2(mod(a_InstanceID, u_size.x), floor(a_InstanceID / u_size.x));
        vec2 texPosition = (mod(corner, 2.0) + a_position) * 0.5;
        v_uv = (vec2(mod(u_tileIndex, 16.0), floor(u_tileIndex / 16.0)) + texPosition) / vec2(16.0, 3.0);

        vec2 halfPixel = vec2(0.5, 0.5) * u_pixel;
        vec2 base = corner + a_position;
        float height = texture2D(u_heightMap, base * u_pixel + halfPixel).a;
        float waterHeight = texture2D(u_waterHeightMap, base * u_pixel + halfPixel).a + u_offsetHeight;
        float value = clamp(waterHeight - height, 0.0, 1.0);

        if (value <= deepLevel) {
          value = max(0.0, value - minDepth) / (deepLevel - minDepth);
          v_color = mix(u_minShallowColor, u_maxShallowColor, value) / 255.0;
        } else {
          value = clamp(value - deepLevel, 0.0, maxDepth - deepLevel) / (maxDepth - deepLevel);
          v_color = mix(u_minDeepColor, u_maxDeepColor, value) / 255.0;
        }

        v_position = base * 128.0 + u_offset;
        gl_Position = u_mvp * vec4(v_position, waterHeight * 128.0, 1.0);
      } else {
        v_uv = vec2(0.0);
        v_color = vec4(0.0);

        gl_Position = vec4(0.0);
      }
    }
  `,
  fsWater: `
    uniform sampler2D u_waterMap;

    uniform vec4 u_mapBounds;

    varying vec2 v_position;
    varying vec2 v_uv;
    varying vec4 v_color;

    void main() {
      vec2 d2 = min(v_position - u_mapBounds.xy, u_mapBounds.zw - v_position);
      float d1 = clamp(min(d2.x, d2.y) / 64.0 + 1.0, 0.0, 1.0) * 0.8 + 0.2;
      gl_FragColor = texture2D(u_waterMap, v_uv).rgba * vec4(v_color.rgb * d1, v_color.a);
    }
  `,
  vsCliffs: `
    uniform mat4 u_mvp;
    uniform sampler2D u_heightMap;
    uniform vec2 u_pixel;
    uniform vec2 u_size;
    uniform vec2 u_centerOffset;

    attribute vec3 a_position;
    attribute vec3 a_normal;
    attribute vec2 a_uv;
    attribute vec3 a_instancePosition;
    attribute float a_instanceTexture;

    varying vec3 v_normal1;
    varying vec3 v_normal2;
    varying float v_height;
    varying vec2 v_uv;
    varying vec2 v_suv;
    varying float v_texture;

    const float normalDist = 1.0;

    void main() {
      // Half of a pixel in the cliff height map.
      vec2 halfPixel = u_pixel * 0.5;

      vec3 position = vec3(a_position.y, -a_position.x, a_position.z);

      // The bottom left corner of the map tile this vertex is on.
      vec2 corner = ((a_instancePosition.xy - u_centerOffset.xy + position.xy) / 128.0);

      float height = texture2D(u_heightMap, corner * u_pixel + halfPixel).a;

      float hL = texture2D(u_heightMap, vec2(corner - vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
      float hR = texture2D(u_heightMap, vec2(corner + vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
      float hD = texture2D(u_heightMap, vec2(corner - vec2(0.0, normalDist)) * u_pixel + halfPixel).a;
      float hU = texture2D(u_heightMap, vec2(corner + vec2(0.0, normalDist)) * u_pixel + halfPixel).a;

      v_normal1 = normalize(vec3(hL - hR, hD - hU, normalDist * 2.0));
      v_normal2 = vec3(a_normal.y, -a_normal.x, a_normal.z);
      v_height = position.z / 128.0;

      v_uv = a_uv;
      v_suv = corner / u_size;
      v_texture = a_instanceTexture;

      gl_Position = u_mvp * vec4(position + vec3(a_instancePosition.xy, a_instancePosition.z + height * 128.0), 1.0);
    }
  `,
  fsCliffs: `
    #extension GL_OES_standard_derivatives : enable
    uniform sampler2D u_texture1;
    uniform sampler2D u_texture2;
    uniform sampler2D u_shadowMap;

    varying vec3 v_normal1;
    varying vec3 v_normal2;
    varying float v_height;
    varying vec2 v_uv;
    varying vec2 v_suv;
    varying float v_texture;

    const vec3 lightDirection = normalize(vec3(-0.3, -0.3, 0.25));

    vec4 sample(int texture, vec2 uv) {
      if (texture == 0) {
        return texture2D(u_texture1, uv);
      } else {
        return texture2D(u_texture2, uv);
      }
    }

    void main() {
      vec4 color = sample(int(v_texture+0.01), v_uv).rgba;

      float height = 2.0 * (fract(v_height) - 0.5);
      height = height * height;
      vec3 normal = normalize(mix(v_normal2, v_normal1, height));

      float shadow = texture2D(u_shadowMap, v_suv).a;
      color.xyz *= clamp(dot(normal, lightDirection) + 0.45, 0.1, 1.0);
      color.xyz *= 1.0 - shadow;

      gl_FragColor = color;
    }
  `,
  vsUberSplat: `
    uniform mat4 u_mvp;
    uniform sampler2D u_heightMap;
    uniform vec2 u_pixel;
    uniform vec2 u_size;
    uniform vec2 u_shadowPixel;
    uniform vec2 u_centerOffset;

    attribute vec3 a_position;
    attribute vec2 a_uv;

    varying vec2 v_uv;
    varying vec2 v_suv;
    varying vec3 v_normal;

    const float normalDist = 0.25;

    void main() {
      vec2 halfPixel = u_pixel * 0.5;

      vec2 base = (a_position.xy - u_centerOffset) / 128.0;
      float height = texture2D(u_heightMap, base * u_pixel + halfPixel).a;

      float hL = texture2D(u_heightMap, vec2(base - vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
      float hR = texture2D(u_heightMap, vec2(base + vec2(normalDist, 0.0)) * u_pixel + halfPixel).a;
      float hD = texture2D(u_heightMap, vec2(base - vec2(0.0, normalDist)) * u_pixel + halfPixel).a;
      float hU = texture2D(u_heightMap, vec2(base + vec2(0.0, normalDist)) * u_pixel + halfPixel).a;

      v_normal = normalize(vec3(hL - hR, hD - hU, normalDist * 2.0));
      v_uv = a_uv;
      v_suv = base / u_size;

      gl_Position = u_mvp * vec4(a_position.xy, height * 128.0 + a_position.z, 1.0);
    }
  `,
  fsUberSplat: `
    uniform sampler2D u_texture;
    uniform sampler2D u_shadowMap;
    uniform vec4 u_color;

    varying vec2 v_uv;
    varying vec2 v_suv;
    varying vec3 v_normal;

    const vec3 lightDirection = normalize(vec3(-0.3, -0.3, 0.25));

    void main() {
      vec4 color = texture2D(u_texture, clamp(v_uv, 0.0, 1.0)).rgba * u_color;

      float shadow = texture2D(u_shadowMap, v_suv).a;
      color.xyz *= clamp(dot(v_normal, lightDirection) + 0.45, 0.0, 1.0);
      color.xyz *= 1.0 - shadow;

      gl_FragColor = color;
    }
  `,
  vsSimpleModel: `
    uniform mat4 u_mvp;

    attribute vec3 a_position;
    attribute vec3 a_normal;
    attribute vec2 a_uv;
    attribute vec3 a_instancePosition;
    attribute vec2 a_instanceRotation;
    attribute float a_instanceScale;

    varying vec3 v_normal;
    varying vec2 v_uv;

    ${shaders.quat_transform}

    void main() {
      v_normal = quat_transform(a_instanceRotation, a_normal);
      v_uv = a_uv;

      gl_Position = u_mvp * vec4(quat_transform(a_instanceRotation, a_position) * a_instanceScale + a_instancePosition, 1.0);
    }
  `,
  fsSimpleModel: `
    uniform sampler2D u_texture;
    uniform bool u_alphaTest;
    uniform bool u_unshaded;

    varying vec3 v_normal;
    varying vec2 v_uv;

    const vec3 lightDirection = normalize(vec3(-0.3, -0.3, 0.25));

    void main() {
      vec4 color = texture2D(u_texture, v_uv).rgba;

      // 1bit Alpha
      if (u_alphaTest && color.a < 0.75) {
        discard;
      }

      if (!u_unshaded) {
        color *= clamp(dot(v_normal, lightDirection) + 0.45, 0.1, 1.0);
      }

      gl_FragColor = color;
    }
  `,
};
