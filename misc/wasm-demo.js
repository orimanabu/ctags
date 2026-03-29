// Static file server for the ctags WASM demo.
//
// Usage:
//   node /path/to/ctags/misc/wasm-demo.js [port] [build-dir]
//
//   port      : TCP port to listen on           (default: 8766)
//   build-dir : directory containing ctags-wasm.js / ctags-wasm.wasm
//               (default: <ctags-source>/build-wasm)
//
// The server searches for requested files in the following directories, in order:
//   1. build-dir        – WASM module output (ctags-wasm.js, ctags-wasm.wasm)
//   2. misc/            – demo HTML/JS source  (wasm-demo.html, wasm-demo.js)
//   3. current dir      – fallback for any extra files
//
// Only the basename of each URL path is used, so subdirectory traversal is
// impossible and every requested resource is resolved against the roots above.
//
// The server sets the correct MIME type for .wasm (application/wasm) and adds
// COOP/COEP headers required by browsers for efficient WASM compilation.

'use strict';

const http = require('http');
const fs   = require('fs');
const path = require('path');

const PORT = parseInt(process.argv[2], 10) || 8766;

// __dirname is the misc/ directory when this script lives in misc/.
// After "make wasm" copies this file to the build dir, __dirname is the build dir.
const SCRIPT_DIR = __dirname;
const CTAGS_SRC  = path.resolve(path.join(SCRIPT_DIR, '..'));

// Build dir: explicit arg > env var > default (<ctags>/build-wasm)
const BUILD_DIR = process.argv[3]
               || process.env.CTAGS_WASM_BUILD_DIR
               || path.join(CTAGS_SRC, 'build-wasm');

// Demo HTML/JS source dir: prefer misc/ next to wasm-demo.js; also check build dir
const DEMO_SRC = path.join(CTAGS_SRC, 'misc');

// Search order: build dir first (WASM .js/.wasm), then misc/ (HTML/JS), then cwd
const ROOTS = [
  path.resolve(BUILD_DIR),
  path.resolve(DEMO_SRC),
  path.resolve(process.cwd()),
];

const MIME = {
  '.html' : 'text/html; charset=utf-8',
  '.js'   : 'application/javascript; charset=utf-8',
  '.wasm' : 'application/wasm',
  '.css'  : 'text/css; charset=utf-8',
  '.map'  : 'application/json',
};

function serve(req, res) {
  let urlPath = req.url.split('?')[0];
  if (urlPath === '/') urlPath = '/wasm-demo.html';

  // Use only the basename – prevents any path traversal attempt
  const name = path.basename(urlPath);
  if (!name) {
    res.writeHead(400); res.end('bad request\n'); return;
  }

  for (const root of ROOTS) {
    const filePath = path.join(root, name);
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
  console.log(`ctags WASM demo  →  http://localhost:${PORT}/`);
  console.log('');
  console.log('Searching for files in:');
  for (const r of ROOTS) console.log(`  ${r}`);
  console.log('');
  console.log('Press Ctrl+C to stop.');
});
