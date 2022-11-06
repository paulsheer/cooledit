/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* debug.h
   Copyright (C) 1996-2022 Paul Sheer
 */

struct debug_options {
    int show_output, stop_at_main, show_on_stdout;
};

extern struct debug_options debug_options;

void debug_callback (void);
void debug_init (void);
void debug_shut (void);
void debug_menus (Window parent, Window main_window, int x, int y);
void debug_key_command (int command);

