const http = require('http');
const fs = require('fs');
const path = require('path');
const dir = '/Users/ori/devel/src/github.com/orimanabu/ctags/build-wasm';
const mime = { '.html':'text/html', '.js':'application/javascript', '.wasm':'application/wasm' };
http.createServer((req, res) => {
  const file = path.join(dir, req.url === '/' ? 'ctags-demo.html' : req.url);
  const ext = path.extname(file);
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); res.end('not found'); return; }
    res.writeHead(200, {'Content-Type': mime[ext] || 'text/plain'});
    res.end(data);
  });
}).listen(8766, () => console.log('http://localhost:8766/ctags-demo.html'));
