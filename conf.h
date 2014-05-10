/* See LICENSE for licence details. */
/* framubuffer device */
const char *fb_path = "/dev/fb0";
//const char *fb_path = "/dev/tty"; /* for BSD */

/* shell */
const char *shell_cmd = "/bin/bash";
// const char *shell_cmd = "/bin/csh"; /* for BSD */

/* TERM value */
const char *term_name = "yaft-256color"; /* default TERM */

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	ACTIVE_CURSOR_COLOR = 2,
	PASSIVE_CURSOR_COLOR = 1,
};

/* misc */
enum {
	DEBUG = false,             /* write dump of input to stdout, debug message to stderr */
	TABSTOP = 8,               /* hardware tabstop */
	LAZY_DRAW = false,         /* reduce drawing when input data size is larger than BUFSIZE */
	ENCODING = EUCJP,         /* character encoding UTF-8 or EUC-JP */
	MAX_CHARS = JIS_CHARS,     /* in UTF-8 set UCS2_CHARS, in EUCJP set JIS_CHARS */
	/* for EUC-JP (JIS code point) */
	SUBSTITUTE_HALF = 0x3F,    /* used for missing glyph(single width): QUESTION MARK (0x3F) */
	SUBSTITUTE_WIDE = 0x222E,  /* used for missing glyph(double width): GETA MARK (0x222E) */
	REPLACEMENT_CHAR = 0x3F,   /* used for malformed EUC-JP sequence:  QUESTION MARK (0x3F) */
};
