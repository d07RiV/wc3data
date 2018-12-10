import Worker from './parser.worker.js';

const parseMapPromise = (worker, meta, map, progress) => new Promise((resolve, reject) => {
  worker.addEventListener("message", e => {
    if (e.data.progress) {
      if (progress) progress(e.data.progress);
    } else if (e.data.result) {
      worker.terminate();
      resolve(e.data.result);
    } else {
      worker.terminate();
      reject(e.data.error);
    }
  });
  worker.postMessage({meta, map});
});

export default class MapParser {
  worker = new Worker();

  onProgress = () => undefined;

  async parse(meta, map) {
    meta = await meta;
    this.onProgress(0);
    return await parseMapPromise(this.worker, meta, map, this.onProgress);
  }

  terminate() {
    this.worker.terminate();
  }
}
