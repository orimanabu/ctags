const http = require('http');
const fs = require('fs');
const path = require('path');
const mime = { '.html':'text/html', '.js':'application/javascript', '.wasm':'application/wasm' };
http.createServer((req, res) => {
  const file = path.join(__dirname, req.url === '/' ? 'wasm-demo.html' : req.url);
  const ext = path.extname(file);
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); res.end('not found'); return; }
    res.writeHead(200, {'Content-Type': mime[ext] || 'text/plain'});
    res.end(data);
  });
}).listen(8766, () => console.log('http://localhost:8766/wasm-demo.html'));
