// Minimal static file server for the ctags WASM demo.
//
// Usage (run from the build directory that contains ctags-wasm.js / ctags-wasm.wasm):
//   node /path/to/ctags/misc/wasm-demo.js [port]
//
// Then open: http://localhost:8766/
//
// The server must send the correct MIME type for .wasm files
// (application/wasm) so browsers can compile the module efficiently.
// The Cross-Origin-Opener-Policy / Cross-Origin-Embedder-Policy headers
// are also set to satisfy SharedArrayBuffer requirements if needed in the future.

'use strict';

const http = require('http');
const fs   = require('fs');
const path = require('path');

const PORT    = parseInt(process.argv[2], 10) || 8766;
const DOCROOT = process.cwd();

const MIME = {
  '.html' : 'text/html; charset=utf-8',
  '.js'   : 'application/javascript; charset=utf-8',
  '.wasm' : 'application/wasm',
  '.css'  : 'text/css; charset=utf-8',
  '.map'  : 'application/json',
};

// Files served from the demo HTML source directory (e.g. misc/wasm-demo.html).
// The build directory is the primary root; fall back to the source directory
// so the HTML/JS demo files are found even before "emmake make wasm" copies them.
const DEMO_SRC = path.join(__dirname);

function serve(req, res) {
  let urlPath = req.url.split('?')[0];
  if (urlPath === '/') urlPath = '/wasm-demo.html';

  // Try build dir first, then source dir
  const candidates = [
    path.join(DOCROOT, urlPath),
    path.join(DEMO_SRC, path.basename(urlPath)),
  ];

  for (const filePath of candidates) {
    if (!filePath.startsWith(DOCROOT) && !filePath.startsWith(DEMO_SRC)) {
      // Prevent path traversal
      continue;
    }
    if (!fs.existsSync(filePath) || !fs.statSync(filePath).isFile()) continue;

    const ext  = path.extname(filePath).toLowerCase();
    const mime = MIME[ext] || 'application/octet-stream';

    res.writeHead(200, {
      'Content-Type'                : mime,
      'Cross-Origin-Opener-Policy'  : 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cache-Control'               : 'no-cache',
    });
    fs.createReadStream(filePath).pipe(res);
    return;
  }

  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end(`404 Not Found: ${urlPath}\n`);
}

const server = http.createServer(serve);
server.listen(PORT, '127.0.0.1', () => {
  console.log(`ctags WASM demo server running at:`);
  console.log(`  http://localhost:${PORT}/`);
  console.log('');
  console.log('Serving files from:');
  console.log(`  build dir : ${DOCROOT}`);
  console.log(`  demo src  : ${DEMO_SRC}`);
  console.log('');
  console.log('Press Ctrl+C to stop.');
});
