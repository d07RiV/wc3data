const proxy = require('http-proxy-middleware');
module.exports = function(app) {
  app.use(proxy('/api', {
    target: 'http://www.wc3local.com',
    changeOrigin: true,
    headers: {
      "Connection": "keep-alive",
    },
  }));
};
