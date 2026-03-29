```
 ▐▛███▜▌   Claude Code v2.1.87
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/devel/src/github.com/orimanabu/ctags

❯ このプロジェクトはctagsです。`ctags --oneshot --sort -o - --output-format json` を実行するWASMバイナリを生成できますか

⏺ Explore(Explore ctags project structure)
  ⎿  Done (43 tool uses · 56.8k tokens · 1m 36s)
  (ctrl+o to expand)

⏺ まず既存のWASM関連ファイルを確認します。

  Read 2 files (ctrl+o to expand)

⏺ Bash(git log --oneline -20 && echo "---" && git diff master..HEAD --name-only 2>/dev/null | head -50)
  ⎿  214b6af21 main: add --oneshot=<filename> option
     1a16310fd main: make the set of system calls allowed in the sandbox tunable
     c7c90141d main: always compile the internal sort routines
     … +41 lines (ctrl+o to expand)

  Searched for 1 pattern, read 2 files (ctrl+o to expand)

⏺ Bash(git show wasm:main/wasm_entry.c 2>/dev/null | head -100)
  ⎿  /*
     *   Copyright (c) 2025
     *
     … +75 lines (ctrl+o to expand)

⏺ Bash(git show wasm:main/wasm_entry.c 2>/dev/null | tail -100)
  ⎿  #include "ctags.h"
     #include "debug.h"
     #include "entry_p.h"
     … +70 lines (ctrl+o to expand)

⏺ Bash(git show wasm:Makefile.am 2>/dev/null | grep -A 30 "wasm")
  ⎿  #   mkdir build-wasm && cd build-wasm
     #   emconfigure ../configure --prefix=... --disable-seccomp --disable-xml \
     #       --disable-json --disable-yaml --disable-pcre2 --disable-iconv
     … +53 lines (ctrl+o to expand)

  Searched for 5 patterns, read 2 files (ctrl+o to expand)

⏺ Bash(git show wasm:Makefile.am | grep -B5 -A40 "^WASM_MODULE")
  ⎿  #   emconfigure ../configure --prefix=... --disable-seccomp --disable-xml \
     #       --disable-json --disable-yaml --disable-pcre2 --disable-iconv
     #   cc -fsigned-char -DPCC_USE_SYSTEM_STRNLEN -o packcc ../misc/packcc/src/packcc.c
     … +43 lines (ctrl+o to expand)

  Searched for 3 patterns, read 2 files (ctrl+o to expand)

⏺ Now I have everything I need. Let me create the solution:

  1. main/wasm_entry.c - adapted from wasm branch with JSON + sort support
  2. Updated Makefile.am with the wasm build target (with Jansson support)
  3. A build script

  Searched for 3 patterns, read 2 files (ctrl+o to expand)

⏺ Now I have everything. Let me create the three files:

⏺ Write(main/wasm_entry.c)
  ⎿  Wrote 152 lines to main/wasm_entry.c
       1 /*
       2 *   Copyright (c) 2025
       3 *
       4 *   This source code is released for free distribution under the terms of the
       5 *   GNU General Public License version 2 or (at your option) any later version.
       6 *
       7 *   WASM entry point for ctags.
       8 *   Provides JavaScript-callable functions for parsing source code from memory.
       9 *   By default, outputs JSON-formatted tags with sorting enabled, equivalent to:
      10 *     ctags --oneshot=<filename> --sort -o - --output-format json
     … +142 lines (ctrl+o to expand)

⏺ Now add the wasm build target to Makefile.am:

⏺ Update(Makefile.am)
  ⎿  Added 35 lines
      351      -name 'Makefile.in' -o -name 'Makefile' -o \
      352      -name 'BUNDLES' \) -exec rm -f {} +
      353
      354 +# Build a WebAssembly module using Emscripten.
      355 +#
      356 +# Prerequisites:
      357 +#   - Emscripten SDK (emcc/emconfigure/emmake)
      358 +#   - Jansson compiled for WASM (see misc/build-wasm.sh for a helper script)
      359 +#
      360 +# Quick start (run from a separate build directory):
      361 +#   mkdir build-wasm && cd build-wasm
      362 +#   JANSSON_CFLAGS="-I/path/to/jansson/src" \
      363 +#   JANSSON_LIBS="/path/to/jansson/src/.libs/libjansson.a" \
      364 +#   emconfigure ../configure --disable-seccomp --disable-xml \
      365 +#       --disable-yaml --disable-pcre2 --disable-iconv
      366 +#   cc -fsigned-char -DPCC_USE_SYSTEM_STRNLEN -o packcc ../misc/packcc/src/packcc.c
      367 +#   emmake make wasm
      368 +#
      369 +WASM_MODULE   = ctags-wasm.js
      370 +WASM_CPPFLAGS = $(libctags_a_CPPFLAGS)
      371 +WASM_LDFLAGS  = \
      372 +  -s WASM=1 \
      373 +  -s MODULARIZE=1 \
      374 +  -s EXPORT_NAME=CTagsModule \
      375 +  -s EXPORTED_FUNCTIONS='["_ctags_init","_ctags_parse_buffer","_ctags_set_output_format","_malloc","_free"]' \
      376 +  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","allocateUTF8","UTF8ToString"]' \
      377 +  -s ALLOW_MEMORY_GROWTH=1
      378 +
      379 +$(WASM_MODULE): libctags.a $(GNULIB_LIBS) $(srcdir)/main/wasm_entry.c
      380 +  $(CC) $(CFLAGS) $(WASM_CPPFLAGS) \
      381 +    $(srcdir)/main/wasm_entry.c \
      382 +    libctags.a $(GNULIB_LIBS) \
      383 +    $(JANSSON_LIBS) \
      384 +    $(WASM_LDFLAGS) -o $@
      385 +
      386 +.PHONY: wasm
      387 +wasm: $(WASM_MODULE)
      388 +
      389  include $(top_srcdir)/makefiles/testing.mak
      390
      391  include $(top_srcdir)/makefiles/help.mak
  ⎿  Found 10 new diagnostic issues in 1 file (ctrl+o to expand)

⏺ The diagnostics are false positives from the host clang — emscripten.h and EMSCRIPTEN_KEEPALIVE only exist when building with emcc. Now let me create a build helper script:

⏺ Write(misc/build-wasm.sh)
  ⎿  Wrote 112 lines to misc/build-wasm.sh
       1 #!/bin/sh
       2 # Build ctags as a WebAssembly module.
       3 #
       4 # Produces ctags-wasm.js and ctags-wasm.wasm in the build directory.
       5 # The module exports:
       6 #   ctags_init()                          - initialize (JSON output + sort)
       7 #   ctags_parse_buffer(name, data, size)  - parse source from a memory buffer
       8 #   ctags_set_output_format(fmt)          - change output format at runtime
       9 #
      10 # Output mimics: ctags --oneshot=<filename> --sort -o - --output-format json
     … +102 lines (ctrl+o to expand)

⏺ Bash(chmod +x /Users/ori/devel/src/github.com/orimanabu/ctags/misc/build-wasm.sh)
  ⎿  Done

⏺ 以上の3ファイルを作成しました。内容の概要：

  ---
  追加・変更したファイル

  main/wasm_entry.c (新規)
  - wasm ブランチのコードをベースに new-wasm ブランチの API に合わせて修正
  - closeTagFile(false, true) — forceUseInternalSort=true でソートをインメモリで実行（sort(1) コマンドを使わない）
  - デフォルトで WRITER_JSON + SO_SORTED に設定 → ctags --oneshot --sort -o - --output-format json と同等の動作
  - エクスポート関数: ctags_init(), ctags_parse_buffer(), ctags_set_output_format()

  Makefile.am (更新)
  - make wasm ターゲットを追加
  - libctags.a と Jansson をリンクして ctags-wasm.js / ctags-wasm.wasm を生成

  misc/build-wasm.sh (新規)
  - Emscripten が PATH にあれば、Jansson のダウンロード・WASM ビルドから ctags のビルドまで自動実行

  ---
  ビルド手順

  # Emscripten SDK を有効化
  source /path/to/emsdk/emsdk_env.sh

  # ビルド実行
  ./misc/build-wasm.sh

  または手動で：

  mkdir build-wasm && cd build-wasm
  JANSSON_CFLAGS="-I/path/to/jansson/src" \
  JANSSON_LIBS="/path/to/jansson/src/.libs/libjansson.a" \
  emconfigure ../configure --disable-seccomp --disable-xml \
      --disable-yaml --disable-pcre2 --disable-iconv
  cc -fsigned-char -DPCC_USE_SYSTEM_STRNLEN -o packcc ../misc/packcc/src/packcc.c
  emmake make wasm

  ---
  JavaScript での使用例

  let output = '';
  const Module = await CTagsModule({
    print: (line) => { output += line + '\n'; },
  });
  Module._ctags_init();

  const src = `int foo(void) { return 0; }`;
  const encoded = new TextEncoder().encode(src);
  Module.ccall('ctags_parse_buffer', null,
    ['string', 'array', 'number'],
    ['test.c', encoded, encoded.length]
  );
  // output: JSON lines sorted by tag name

✻ Brewed for 7m 8s
```

```
❯ wasmブランチのwasm-demo.{html,js}を参考に、ブラウザで起動するwasmバイナリを使ったデモを作ってください

⏺ Bash(git show wasm:misc/wasm-demo.html && echo "===JS===" && git show wasm:misc/wasm-demo.js)
  ⎿  <!DOCTYPE html>
     <html lang="en">
     <head>
     … +94 lines (ctrl+o to expand)

⏺ wasm ブランチのデモを参考に、JSON出力に対応した新しいデモを作成します。まず ctags の JSON 出力形式を確認します。

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

⏺ JSON 出力フォーマット（name, path, pattern, kind, scope, line 等）を把握しました。デモを作成します。

⏺ Write(misc/wasm-demo.html)
  ⎿  Wrote 673 lines to misc/wasm-demo.html
       1 <!DOCTYPE html>
       2 <html lang="en">
       3 <head>
       4   <meta charset="UTF-8">
       5   <meta name="viewport" content="width=device-width, initial-scale=1.0">
       6   <title>ctags WASM Demo</title>
       7   <style>
       8     * { box-sizing: border-box; margin: 0; padding: 0; }
       9
      10     body {
     … +663 lines (ctrl+o to expand)

⏺ Write(misc/wasm-demo.js)
  ⎿  Wrote 79 lines to misc/wasm-demo.js
       1 // Minimal static file server for the ctags WASM demo.
       2 //
       3 // Usage (run from the build directory that contains ctags-wasm.js / ctags-wasm.wasm):
       4 //   node /path/to/ctags/misc/wasm-demo.js [port]
       5 //
       6 // Then open: http://localhost:8766/
       7 //
       8 // The server must send the correct MIME type for .wasm files
       9 // (application/wasm) so browsers can compile the module efficiently.
      10 // The Cross-Origin-Opener-Policy / Cross-Origin-Embedder-Policy headers
     … +69 lines (ctrl+o to expand)

⏺ Makefile.am のコピー先も wasm-demo.html と wasm-demo.js を含むよう更新します。

  Read 1 file (ctrl+o to expand)

⏺ Update(Makefile.am)
  ⎿  Added 2 lines
      385
      386  .PHONY: wasm
      387  wasm: $(WASM_MODULE)
      388 +  cp $(srcdir)/misc/wasm-demo.html .
      389 +  cp $(srcdir)/misc/wasm-demo.js .
      390
      391  include $(top_srcdir)/makefiles/testing.mak
      392

⏺ EXTRA_DIST にデモファイルを追加します。

⏺ Update(Makefile.am)
  ⎿  Error editing file

  Searched for 1 pattern (ctrl+o to expand)

⏺ Update(Makefile.am)
  ⎿  Added 3 lines
      73           $(PEG_INPUT) $(OPTLIB2C_INPUT) $(OPTLIB2C_PCRE2_INPUT) $(TXT2CSTR_INPUT) \
      74           misc/universal-ctags.svg \
      75           misc/review \
      76 +         misc/build-wasm.sh \
      77 +         misc/wasm-demo.html \
      78 +         misc/wasm-demo.js \
      79           docs Tmain Units m4/gnulib-cache.m4
      80
      81  if HAVE_PEGOF

⏺ 作成したファイルを確認します。

  Read 1 file (ctrl+o to expand)

⏺ Bash(git diff --stat HEAD)
  ⎿   Makefile.am | 5 +++++
      1 file changed, 5 insertions(+)

⏺ Bash(git status)
  ⎿  On branch new-wasm
     Changes not staged for commit:
       (use "git add <file>..." to update what will be committed)
     … +11 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

⏺ Bash(git status --short main/wasm_entry.c misc/build-wasm.sh)
  ⎿  (No output)

  Searched for 1 pattern (ctrl+o to expand)

⏺ Bash(git check-ignore -v main/wasm_entry.c misc/build-wasm.sh)
  ⎿  Error: Exit code 1

⏺ Bash(git ls-files main/wasm_entry.c misc/build-wasm.sh)
  ⎿  main/wasm_entry.c
     misc/build-wasm.sh

⏺ これらのファイルはすでにトラックされています（前回のセッションで作成済み）。git diff で確認します。

⏺ Bash(git log --oneline -3 && echo "---" && git show --stat HEAD)
  ⎿  aa49581b2 Make wasm binary of `ctags --oneshot --sort -o - --output-format json`
     214b6af21 main: add --oneshot=<filename> option
     1a16310fd main: make the set of system calls allowed in the sandbox tunable
     … +12 lines (ctrl+o to expand)

⏺ 前回のセッションで main/wasm_entry.c と misc/build-wasm.sh はすでにコミット済みでした。今回追加したのは以下の差分です。

⏺ Bash(git diff HEAD Makefile.am)
  ⎿  diff --git a/Makefile.am b/Makefile.am
     index 0d2bc2bbf..44390efe1 100644
     --- a/Makefile.am
     … +19 lines (ctrl+o to expand)

⏺ 以上です。作成・変更したものをまとめます。

  ---
  追加・変更したファイル

  misc/wasm-demo.html (新規)

  wasm ブランチのデモを元に、JSON 出力対応で全面改修:

  ┌───────────────────┬────────────────────────────────────────────────────────────┐
  │       機能        │                            内容                            │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ 出力形式セレクタ  │ json / u-ctags / xref / etags を切り替え                   │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ JSON テーブル表示 │ name, kind, line, scope, typeref, pattern を列に分けて表示 │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ ptag 除外         │ {"_type":"ptag",...} はテーブルから除外                    │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ raw 出力          │ JSON シンタックスハイライト付きで折りたたみ表示            │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ サンプル          │ C / Python / JavaScript / Go / Rust の 5 種                │
  ├───────────────────┼────────────────────────────────────────────────────────────┤
  │ キーバインド      │ Ctrl+Enter で実行                                          │
  └───────────────────┴────────────────────────────────────────────────────────────┘

  misc/wasm-demo.js (新規)

  Node.js 静的ファイルサーバー。application/wasm の MIME タイプと COOP/COEP ヘッダを正しく設定し、ビルドディレクトリと misc/ の両方からファイルを配信。

  Makefile.am (更新)

  make wasm 実行後に wasm-demo.html/wasm-demo.js をビルドディレクトリにコピーするよう追加。

  ---
  動作確認手順

  # 1. WASM ビルド（初回）
  ./misc/build-wasm.sh          # → build-wasm/ctags-wasm.{js,wasm}

  # 2. デモサーバー起動
  node misc/wasm-demo.js        # build-wasm/ ディレクトリから実行

  # → http://localhost:8766/ をブラウザで開く

✻ Baked for 3m 10s
```
