import GenericObject from './genericobject';
import {vec3} from 'gl-matrix';

const zero = vec3.create();

/**
 * An MDX camera.
 */
export default class Camera extends GenericObject {
  /**
   * @param {ModelViewer.viewer.handlers.mdx.Model} model
   * @param {ModelViewer.parsers.mdlx.Camera} camera
   * @param {number} index
   */
  constructor(model, camera, index) {
    super(model, camera, index);

    this.name = camera.name;
    this.position = camera.position;
    this.fieldOfView = camera.fieldOfView;
    this.farClippingPlane = camera.farClippingPlane;
    this.nearClippingPlane = camera.nearClippingPlane;
    this.targetPosition = camera.targetPosition;
  }

  /**
   * @param {vec3} out
   * @param {ModelInstance} instance
   * @return {number}
   */
  getPositionTranslation(out, instance) {
    const res = this.getVector3Value(out, 'KCTR', instance, zero);
    vec3.add(out, out, this.position);
    return res;
  }

  /**
   * @param {vec3} out
   * @param {ModelInstance} instance
   * @return {number}
   */
  getTargetTranslation(out, instance) {
    const res = this.getVector3Value(out, 'KTTR', instance, zero);
    vec3.add(out, out, this.targetPosition);
    return res;
  }

  /**
   * @param {Float32Array} out
   * @param {ModelInstance} instance
   * @return {number}
   */
  getRotation(out, instance) {
    return this.getFloatValue(out, 'KCRL', instance, 0);
  }
}
