/* See LICENSE for licence details. */
void (*ctrl_func[CTRL_CHARS])(struct terminal * term, void *arg) = {
	[BS]  = bs,
	[HT]  = tab,
	[LF]  = nl,
	[VT]  = nl,
	[FF]  = nl,
	[CR]  = cr,
	[ESC] = enter_esc,
};

void (*esc_func[ESC_CHARS])(struct terminal * term, void *arg) = {
	['7'] = save_state,
	['8'] = restore_state,
	['D'] = nl,
	['E'] = crnl,
	['H'] = set_tabstop,
	['M'] = reverse_nl,
	['P'] = enter_dcs,
	['Z'] = identify,
	['['] = enter_csi,
	[']'] = enter_osc,
	['c'] = ris,
};

void (*csi_func[ESC_CHARS])(struct terminal * term, void *arg) = {
	['@'] = insert_blank,
	['A'] = curs_up,
	['B'] = curs_down,
	['C'] = curs_forward,
	['D'] = curs_back,
	['E'] = curs_nl,
	['F'] = curs_pl,
	['G'] = curs_col,
	['H'] = curs_pos,
	['J'] = erase_display,
	['K'] = erase_line,
	['L'] = insert_line,
	['M'] = delete_line,
	['P'] = delete_char,
	['X'] = erase_char,
	['a'] = curs_forward,
	['c'] = identify,
	['d'] = curs_line,
	['e'] = curs_down,
	['f'] = curs_pos,
	['g'] = clear_tabstop,
	['h'] = set_mode,
	['l'] = reset_mode,
	['m'] = set_attr,
	['n'] = status_report,
	['r'] = set_margin,
	['s'] = save_state,
	['u'] = restore_state,
	['`'] = curs_col,
};

void control_character(struct terminal *term, uint8_t ch)
{
	static const char *ctrl_char[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS ", "HT ", "LF ", "VT ", "FF ", "CR ", "SO ", "SI ",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM ", "SUB", "ESC", "FS ", "GS ", "RS ", "US ",
	};

	if (DEBUG)
		fprintf(stderr, "ctl: %s\n", ctrl_char[ch]);

	if (ctrl_func[ch])
		ctrl_func[ch](term, NULL);
}

void esc_sequence(struct terminal *term, uint8_t ch)
{
	if (DEBUG)
		fprintf(stderr, "esc: ESC %s\n", term->esc.buf);

	if (strlen(term->esc.buf) == 1 && esc_func[ch])
		esc_func[ch](term, NULL);

	if (ch != '[' && ch != ']')
		reset_esc(term);
}

void csi_sequence(struct terminal *term, uint8_t ch)
{
	struct parm_t parm;

	if (DEBUG)
		fprintf(stderr, "csi: CSI %s\n", term->esc.buf);

	reset_parm(&parm);
	parse_arg(term->esc.buf + 1, &parm, ';', isdigit); /* skip '[' */
	*(term->esc.bp - 1) = '\0'; /* omit final character */

	if (csi_func[ch])
		csi_func[ch](term, &parm);

	reset_esc(term);
}

void osc_sequence(struct terminal *term, uint8_t ch)
{
	int i, osc_mode;
	struct parm_t parm;

	if (DEBUG)
		fprintf(stderr, "osc: OSC %s\n", term->esc.buf);

	reset_parm(&parm);
	parse_arg(term->esc.buf + 1, &parm, ';', is_osc_parm); /* skip ']' */
	if (*(term->esc.bp - 1) == BACKSLASH) /* ST: ESC BACKSLASH */
		*(term->esc.bp - 2) = '\0';
	*(term->esc.bp - 1) = '\0'; /* omit final character */

	if (DEBUG)
		for (i = 0; i < parm.argc; i++)
			fprintf(stderr, "\targv[%d]: %s\n", i, parm.argv[i]);

	if (parm.argc > 0) {
		osc_mode = atoi(parm.argv[0]);
		if (DEBUG)
			fprintf(stderr, "osc_mode:%d\n", osc_mode);

		if (osc_mode == 4)
			set_palette(term, &parm);
		else if (osc_mode == 104)
			reset_palette(term, &parm);
		else if (osc_mode == 8900)
			glyph_width_report(term, &parm);
	}

	reset_esc(term);
}

void dcs_sequence(struct terminal *term, uint8_t ch)
{
	struct parm_t parm;

	if (DEBUG)
		fprintf(stderr, "dcs: DCS %s\n", term->esc.buf);

	reset_parm(&parm);
	parse_arg(term->esc.buf, &parm, ';', isdigit);
	*(term->esc.bp - 1) = '\0'; /* omit final character */

	if (term->esc.state == STATE_DCS && ch == '{') /* end of DECDLD header */
		enter_dscs(term, &parm);
	else if (term->esc.state == STATE_DSCS && ('0' <= ch && ch <= '~')) /* end of Dscs of DECDLD */
		enter_sixel(term, &parm);
	else
		reset_esc(term);
}

void utf8_charset(struct terminal *term, uint8_t ch)
{
	if (0x80 <= ch && ch <= 0xBF) {
		/* check illegal UTF-8 sequence
			* ? byte sequence: first byte must be between 0xC2 ~ 0xFD
			* 2 byte sequence: first byte must be between 0xC2 ~ 0xDF
 			* 3 byte sequence: second byte following 0xE0 must be between 0xA0 ~ 0xBF
 			* 4 byte sequence: second byte following 0xF0 must be between 0x90 ~ 0xBF
 			* 5 byte sequence: second byte following 0xF8 must be between 0x88 ~ 0xBF
 			* 6 byte sequence: second byte following 0xFC must be between 0x84 ~ 0xBF
		*/
		if ((term->charset.following_byte == 0)
			|| (term->charset.following_byte == 1 && term->charset.count == 0 && term->charset.code <= 1) 
			|| (term->charset.following_byte == 2 && term->charset.count == 0 && term->charset.code == 0 && ch < 0xA0)
			|| (term->charset.following_byte == 3 && term->charset.count == 0 && term->charset.code == 0 && ch < 0x90)
			|| (term->charset.following_byte == 4 && term->charset.count == 0 && term->charset.code == 0 && ch < 0x88)
			|| (term->charset.following_byte == 5 && term->charset.count == 0 && term->charset.code == 0 && ch < 0x84))
			term->charset.is_valid = false;

		term->charset.code <<= 6;
		term->charset.code += ch & 0x3F;
		term->charset.count++;
	}
	else if (0xC0 <= ch && ch <= 0xDF) {
		term->charset.code = ch & 0x1F;
		term->charset.following_byte = 1;
		term->charset.count = 0;
		return;
	}
	else if (0xE0 <= ch && ch <= 0xEF) {
		term->charset.code = ch & 0x0F;
		term->charset.following_byte = 2;
		term->charset.count = 0;
		return;
	}
	else if (0xF0 <= ch && ch <= 0xF7) {
		term->charset.code = ch & 0x07;
		term->charset.following_byte = 3;
		term->charset.count = 0;
		return;
	}
	else if (0xF8 <= ch && ch <= 0xFB) {
		term->charset.code = ch & 0x03;
		term->charset.following_byte = 4;
		term->charset.count = 0;
		return;
	}
	else if (0xFC <= ch && ch <= 0xFD) {
		term->charset.code = ch & 0x01;
		term->charset.following_byte = 5;
		term->charset.count = 0;
		return;
	}
	else { /* 0xFE - 0xFF: not used in UTF-8 */
		addch(term, REPLACEMENT_CHAR);
		reset_charset(term);
		return;
	}

	if (term->charset.count >= term->charset.following_byte) {
		/*	illegal code point (ref: http://www.unicode.org/reports/tr27/tr27-4.html)
			0xD800   ~ 0xDFFF : surrogate pair
			0xFDD0   ~ 0xFDEF : noncharacter
			0xnFFFE  ~ 0xnFFFF: noncharacter (n: 0x00 ~ 0x10)
			0x110000 ~		: invalid (unicode U+0000 ~ U+10FFFF)
		*/
		if (!term->charset.is_valid
			|| (0xD800 <= term->charset.code && term->charset.code <= 0xDFFF)
			|| (0xFDD0 <= term->charset.code && term->charset.code <= 0xFDEF)
			|| ((term->charset.code & 0xFFFF) == 0xFFFE || (term->charset.code & 0xFFFF) == 0xFFFF)
			|| (term->charset.code > 0x10FFFF))
			addch(term, REPLACEMENT_CHAR);
		else
			addch(term, term->charset.code);

		reset_charset(term);
	}
}

void eucjp_charset(struct terminal *term, uint8_t ch)
{
	/*
		            first byte    second byte   third byte
		ASCII     : 0x00 ~ 0x7F
		JIS X 0208: 0xA1 ~ 0xFE   0xA1 ~ 0xFE
		JIS X 0201: 0x8E          0xA1 ~ 0xDF
		JIS X 0212: 0x8F          0xA1 ~ 0xFE   0xA1 ~ 0xFE

		ref: http://ash.jp/code/code.htm
	*/
	if (term->charset.following_byte == 0) { /* first byte */
		if (0xA1 <= ch && ch <= 0xFE) { /* JIS X 0208 */
			term->charset.code = ch - EUC_JIS_OFFSET;
			term->charset.following_byte = 1;
			term->charset.count = 0;
		}
		else if (ch == 0x8E) { /* JIS X 0201 */
			term->charset.code = 0;
			term->charset.following_byte = 1;
			term->charset.count = 0;
		}
		else if (ch == 0x8F) { /* JIS X 0212 */
			term->charset.code = 0;
			term->charset.following_byte = 2;
			term->charset.count = 0;
		}
		else {
			addch(term, REPLACEMENT_CHAR);
			reset_charset(term);
			return;
		}
	}
	else { /* second or third byte */
		if ((term->charset.code == 0 && term->charset.following_byte == 1)
			&& (0x80 <= ch && ch < 0xFF)){ /* JIS X 0201 */
			// in fact, "(0xA1 <= ch && ch < 0xDF)" is correct. this for X68000 glyphs
			term->charset.code = ch;
			term->charset.count++;
		}
		else if (0xA1 <= ch && ch <= 0xFE) { /* JIS X 0208 or JIS X 0212 */
			term->charset.code <<= 8;
			term->charset.code += ch - EUC_JIS_OFFSET;
			term->charset.count++;
		}
		else {
			addch(term, REPLACEMENT_CHAR);
			reset_charset(term);
			return;
		}
	}

	if (term->charset.count >= term->charset.following_byte) {
		if (term->charset.following_byte == 2) /* JIS X 0212 not supported */
			addch(term, SUBSTITUTE_WIDE);
		else
			addch(term, term->charset.code);

		reset_charset(term);
	}
}

void parse(struct terminal *term, uint8_t *buf, int size)
{
	/*
		CTRL CHARS      : 0x00 ~ 0x1F
		ASCII(printable): 0x20 ~ 0x7E
		CTRL CHARS(DEL) : 0x7F
		UTF-8           : 0x80 ~ 0xFF
		EUC-JP          : 0xA1 ~ 0xFE
	*/
	uint8_t ch;
	int i;

	for (i = 0; i < size; i++) {
		ch = buf[i];
		if (term->esc.state == STATE_RESET) {
			if (term->encoding == UTF8) {
				// interrupted by illegal byte
				if (term->charset.following_byte > 0 && (ch < 0x80 || ch > 0xBF)) { 
					addch(term, REPLACEMENT_CHAR);
					reset_charset(term);
				}

				if (ch <= 0x1F)
					control_character(term, ch);
				else if (ch <= 0x7F)
					addch(term, ch);
				else
					utf8_charset(term, ch);
			}
			else if (term->encoding == EUCJP) {
				// interrupt by illegal byte
				if (term->charset.following_byte > 0 && (ch < 0x80 ||  ch > 0xFE)) {
				// in fact, "(ch < 0xA1 ||  ch > 0xFE)" is corrent. this for X68000 glyphs
					addch(term, REPLACEMENT_CHAR);
					reset_charset(term);
				}

				if (ch <= 0x1F)
					control_character(term, ch);
				else if (ch <= 0x7F)
					addch(term, ch);
				else
					eucjp_charset(term, ch);
			}
		}
		else if (term->esc.state == STATE_ESC) {
			if (push_esc(term, ch))
				esc_sequence(term, ch);
		}
		else if (term->esc.state == STATE_CSI) {
			if (push_esc(term, ch))
				csi_sequence(term, ch);
		}
		else if (term->esc.state == STATE_OSC) {
			if (push_esc(term, ch))
				osc_sequence(term, ch);
		}
		else if (term->esc.state == STATE_DCS || term->esc.state == STATE_DSCS) {
			if (push_esc(term, ch))
				dcs_sequence(term, ch);
		}
		else if (term->esc.state == STATE_SIXEL) {
			parse_sixel(term, ch);
		}
	}
}
