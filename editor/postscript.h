
/*  Some configuration constants, meassured in points (1/72 inch) */
extern int postscript_option_page_top;	/* A4 = 297mm x 210mm = 841pt x 595pt */
extern int postscript_option_page_right_edge;

/*  Set to 1 if your printer doesn't have iso encoding builtin. */
extern int isoencoding_not_builtin;

extern int postscript_option_landscape;
extern int postscript_option_footline_in_landscape;
extern int postscript_option_line_numbers;
extern int postscript_option_font_size;
extern int postscript_option_left_margin;
extern int postscript_option_right_margin;
extern int postscript_option_top_margin;
extern int postscript_option_bottom_margin;
extern unsigned char *postscript_option_font;
extern unsigned char *postscript_option_header_font;
extern int postscript_option_header_font_size;
extern unsigned char *postscript_option_line_number_font;
extern int postscript_option_line_number_size;
extern int postscript_option_col_separation;
extern int postscript_option_header_height;
extern int postscript_option_show_header;
extern int postscript_option_wrap_lines;
extern int postscript_option_columns;
extern int postscript_option_chars_per_line;
extern int postscript_option_plain_text;
extern unsigned char *postscript_option_title;
extern unsigned char *postscript_option_file;
extern unsigned char *postscript_option_pipe;
extern unsigned char *option_continuation_string;
extern int postscript_option_tab_size;

extern void (*postscript_dialog_cannot_open) (unsigned char *);
extern int (*postscript_dialog_exists) (unsigned char *);
extern unsigned char *(*postscript_get_next_line) (unsigned char *);

void postscript_print (void);

