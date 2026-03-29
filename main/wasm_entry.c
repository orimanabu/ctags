/*
*   Copyright (c) 2025
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License version 2 or (at your option) any later version.
*
*   WASM entry point for ctags.
*   Provides JavaScript-callable functions for parsing source code from memory.
*   By default, outputs JSON-formatted tags with sorting enabled, equivalent to:
*     ctags --oneshot=<filename> --sort -o - --output-format json
*
*   Usage from JavaScript:
*
*     let output = '';
*     const Module = await CTagsModule({
*       print:    (line) => { output += line + '\n'; },
*       printErr: (line) => { ... },
*     });
*     Module._ctags_init();
*
*     const src = `int foo(void) { return 0; }`;
*     const encoded = new TextEncoder().encode(src);
*     Module.ccall('ctags_parse_buffer', null,
*       ['string', 'array', 'number'],
*       ['test.c', encoded, encoded.length]
*     );
*     console.log(output);
*/

#include "general.h"  /* must always come first */

#include <string.h>
#include <stdlib.h>

#include <emscripten.h>

#include "ctags.h"
#include "debug.h"
#include "entry_p.h"
#include "error_p.h"
#include "field_p.h"
#include "main_p.h"
#include "mio.h"
#define OPTION_WRITE   /* allow writing to Option (same pattern as options.c) */
#include "options_p.h"
#include "lregex_p.h"
#include "parse_p.h"
#include "routines_p.h"
#include "trashbox_p.h"
#include "writer_p.h"
#include "xtag_p.h"
#include "interactive_p.h"

static bool ctags_initialized = false;

/*
 * ctags_init: Initialize ctags subsystems.
 * Configures JSON output (when built with HAVE_JANSSON) and in-memory sorting,
 * matching: ctags --oneshot=<file> --sort -o - --output-format json
 * Must be called once before ctags_parse_buffer().
 */
EMSCRIPTEN_KEEPALIVE
void ctags_init (void)
{
	if (ctags_initialized)
		return;

	initDefaultTrashBox ();
	setErrorPrinter (stderrDefaultErrorPrinter, NULL);
#ifdef HAVE_JANSSON
	setTagWriter (WRITER_JSON, NULL);
#else
	setTagWriter (WRITER_U_CTAGS, NULL);
#endif
	setCurrentDirectory ();
	checkRegex ();
	initFieldObjects ();
	initXtagObjects ();
	initializeParsing ();
	initOptions ();
	initRegexOptscript ();

	/*
	 * INTERACTIVE_MODE         : makes isDestinationStdout() return true
	 *                            so openTagFile() routes output to stdout path.
	 * INTERACTIVE_WITH_SANDBOX : makes openTagFile() use an in-memory MIO
	 *                            instead of a temp file (required for browser WASM).
	 */
	Option.interactive = INTERACTIVE_MODE | INTERACTIVE_WITH_SANDBOX;

	/* SO_SORTED: enable sorting. closeTagFile() is called with
	 * forceUseInternalSort=true so no external sort(1) command is needed. */
	Option.sorted = SO_SORTED;

	ctags_initialized = true;
}

/*
 * ctags_set_output_format: Switch output format.
 * Supported values: "ctags" (default u-ctags), "xref", "etags"
 * "json" requires building with HAVE_JANSSON (--enable-json).
 */
EMSCRIPTEN_KEEPALIVE
void ctags_set_output_format (const char *format)
{
	if (strcmp (format, "xref") == 0)
		setTagWriter (WRITER_XREF, NULL);
	else if (strcmp (format, "etags") == 0)
		setTagWriter (WRITER_ETAGS, NULL);
#ifdef HAVE_JANSSON
	else if (strcmp (format, "json") == 0)
		setTagWriter (WRITER_JSON, NULL);
#endif
	else
		setTagWriter (WRITER_U_CTAGS, NULL);
}

/*
 * ctags_parse_buffer: Parse source code from a memory buffer.
 *
 * filename : Virtual filename used for language detection (e.g. "test.c").
 * data     : Pointer to source code bytes.
 * size     : Number of bytes in data.
 *
 * Tags are written line-by-line to stdout.
 * Capture output in JavaScript by overriding Module.print before calling ctags_init():
 *
 *   let tags = '';
 *   const Module = await CTagsModule({ print: (line) => { tags += line + '\n'; } });
 *   Module._ctags_init();
 *   Module.ccall('ctags_parse_buffer', null,
 *     ['string', 'array', 'number'],
 *     ['test.c', encoded, encoded.length]);
 */
EMSCRIPTEN_KEEPALIVE
void ctags_parse_buffer (const char *filename, const unsigned char *data, int size)
{
	openTagFile ();

	/* Build an in-memory MIO from the source buffer, the same way
	 * interactiveOneshot() does. */
	MIO *input = mio_new_memory (NULL, 0, eRealloc, eFreeNoNullCheck);
	mio_write (input, data, 1, size);
	mio_seek (input, 0, SEEK_SET);

	parseFileWithMio (filename, input, NULL);
	mio_unref (input);

	/* forceUseInternalSort=true: sort in memory without invoking sort(1),
	 * which is unavailable in a WASM/browser environment. */
	closeTagFile (false, true);
}
