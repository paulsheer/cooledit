/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef _XIM_H
#define _XIM_H

/* #if (XtSpecificationRelease >= 6) */
/* #undef USE_XIM */
#define USE_XIM
/* #endif */

void init_xlocale (void);
long create_input_context_ (const char *msg, XIMStyle input_style, XIC *input_context, Window winid);
void destroy_input_context_ (XIC *ic);

typedef void (*set_input_context_cb_t) (XIMStyle input_style);
typedef void (*destroy_input_context_cb_t) (void);
void xim_set_input_cb (set_input_context_cb_t create_cb, destroy_input_context_cb_t destroy_cb);


#endif	/* _XIM_H */
