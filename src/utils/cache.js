class Cache {
  constructor() {
    this.cache = {};
    this.counter = 0;
    this.subs = [];
  }
  subscribe(func) {
    this.subs.push(func);
    func(this.counter);
  }
  unsubscribe(func) {
    this.subs = this.subs.filter(f => f !== func);
  }
  _start() {
    this.counter += 1;
    this.subs.forEach(func => func(this.counter));
  }
  _stop() {
    this.counter -= 1;
    this.subs.forEach(func => func(this.counter));
  }
  fetch(url, opt) {
    opt = opt || {};
    let curl = url;
    if (opt.info) curl += "$info";
    if (opt.binary) curl += "$binary";
    if (!opt.refresh && this.cache[curl]) return this.cache[curl];
    const get = this.cache[curl] = fetch(url).then(response => {
      if (response.ok) {
        if (opt.binary) return response.arrayBuffer();
        if (!opt.info) {
          let res = response.json();
          if (opt.process) {
            res = res.then(v => opt.process(v));
          }
          return res;
        }
        return response.text().then(text => ({
          compressed: parseInt(response.headers.get("Content-Length"), 10),
          uncompressed: text.length,
          json: JSON.parse(text)
        }));
      } else {
        return response.json().then(err => Promise.reject(err));
      }
    });
    if (opt.global) {
      this._start();
      get.then(() => this._stop(), () => this._stop());
    }
    return get;
  }
  refresh(url, opt) {
    return this.fetch(url, {...opt, refresh: true});
  }
}

export { Cache };
export default Cache;
