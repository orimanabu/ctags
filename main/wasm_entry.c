/*
*   Copyright (c) 2025
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License version 2 or (at your option) any later version.
*
*   WASM entry point for ctags.
*   Provides JavaScript-callable functions for parsing source code from memory.
*
*   Usage from JavaScript:
*
*     // print callback must be set at module creation time
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
 * Must be called once before ctags_parse_buffer().
 */
EMSCRIPTEN_KEEPALIVE
void ctags_init (void)
{
	if (ctags_initialized)
		return;

	initDefaultTrashBox ();
	setErrorPrinter (stderrDefaultErrorPrinter, NULL);
	setTagWriter (WRITER_U_CTAGS, NULL);
	setCurrentDirectory ();
	checkRegex ();
	initFieldObjects ();
	initXtagObjects ();
	initializeParsing ();
	initOptions ();          /* installs default language maps (.c → C etc.) */
	initRegexOptscript ();   /* required after initOptions */

	/*
	 * INTERACTIVE_MODE     : makes isDestinationStdout() return true
	 *                        so openTagFile() routes output to stdout path.
	 * INTERACTIVE_WITH_SANDBOX: makes openTagFile() use an in-memory MIO
	 *                        instead of a temp file (required for browser WASM).
	 * SO_UNSORTED          : disables external sort which also needs temp files.
	 */
	Option.interactive = INTERACTIVE_MODE | INTERACTIVE_WITH_SANDBOX;
	Option.sorted = SO_UNSORTED;

	ctags_initialized = true;
}

/*
 * ctags_set_output_format: Switch output format.
 * format: "ctags" (default universal ctags format) or "xref"
 *
 * Note: "json" requires HAVE_JANSSON (build with --enable-json).
 *       "etags" is also available via WRITER_ETAGS.
 */
EMSCRIPTEN_KEEPALIVE
void ctags_set_output_format (const char *format)
{
	if (strcmp (format, "xref") == 0)
		setTagWriter (WRITER_XREF, NULL);
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
 * Capture output in JavaScript by overriding Module.print before calling:
 *
 *   let tags = '';
 *   Module.print = (line) => { tags += line + '\n'; };
 *   Module._ctags_parse_buffer(filename, data, size);
 */
EMSCRIPTEN_KEEPALIVE
void ctags_parse_buffer (const char *filename, const unsigned char *data, int size)
{
	openTagFile ();

	/* Build the MIO the same way interactiveOneshot() does:
	 * start with empty memory MIO, write data into it, then seek to start. */
	MIO *input = mio_new_memory (NULL, 0, eRealloc, eFreeNoNullCheck);
	mio_write (input, data, 1, size);
	mio_seek (input, 0, SEEK_SET);

	parseFileWithMio (filename, input, NULL);
	mio_unref (input);

	closeTagFile (false);
}
