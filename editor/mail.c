/* mail.c - runs subject/to dialog, then pipes buffer through mail shell command
   Copyright (C) 1996-2018 Paul Sheer
 */


#include "coolwidget.h"

CWidget * mail_editor = 0;

#if 0

void do_mail (CWidget * edit);

void pipe_mail (char *reply_to, char *from, char *to, char *subject, char *cc, char *bcc)
{
    FILE *p;
    p = (FILE *) popen ("sendmail -t", "w");
    if (!p)
	p = (FILE *) popen ("/usr/sbin/sendmail -t", "w");
    if (!p)
	p = (FILE *) popen ("/usr/lib/sendmail -t", "w");
    if (!p) {
	return;
#if 0
	CErrorDialog (mail_editor->mainid, 20, 20, _ (" Mail "), _ (" popen() failed for 'mail' command. \n Check that 'mail' is in your path, and accepts \n    mail -s <subject> -c <copies_to_address> <to_address> "));
#endif
    } else {
	errno = 0;
	if (to)
	    if (*to)
		fprintf (p, "To: %s\n", to);
	if (from)
	    if (*from)
		fprintf (p, "From: %s\n", from);
	if (subject)
	    if (*subject)
		fprintf (p, "Subject: %s\n", subject);
	if (cc)
	    if (*cc)
		fprintf (p, "CC: %s\n", cc);
	if (bcc)
	    if (*bcc)
		fprintf (p, "Bcc: %s\n", bcc);
	if (reply_to)
	    if (*reply_to)
		fprintf (p, "Reply-To: %s\n", reply_to);
	fprintf (p, "\n");
	edit_write_stream (mail_editor->editor, p);
	pclose (p);
    }
}

#if 0

void pipe_mail (char *to, char *subject, char *cc)
{
    long i;
    int mail_pipe, error_pipe, len;
    FILE *a, *b;
    char *s;
    char *argv[10] =
    {"mail", "-W", 0, "-c", 0, 0, 0, 0};

    argv[2] = subject;
    argv[4] = cc;
    argv[5] = to;

    triple_pipe_open (&mail_pipe, &error_pipe, 0, 1, argv[0], argv);

    switch (fork ()) {
    case -1:
	close (mail_pipe);
	close (error_pipe);
	CErrorDialog (mail_editor->mainid, 20, 20, _(" Mail "), _(" Error forking mail "));
	return;
    case 0:
	a = fdopen (mail_pipe, "w");
	edit_write_stream (mail_editor->editor, a);
	fclose (a);
	exit (1);
    default:
	break;
    }

    len = 0;
    s = read_pipe (error_pipe, &len);
    if (len)
	CErrorDialog (mail_editor->mainid, 20, 20, _(" Mail "), "%s", s);
    close (error_pipe);
}

#endif

void mail_subject_to_cc_dialog (Window in, int x, int y)
{
    Window win;
    CEvent cwevent;
    CState s;
    int y2, w;

    if (!mail_editor->editor->last_byte) {
	CErrorDialog (mail_editor->mainid, 20, 20, _ (" Mail "), _ (" Type out a message first "));
	return;
    }
    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("mail", in, x, y, _ (" Send Mail "));
    CGetHintPos (&x, &y);

    (CDrawText ("mail.tcmd", win, x, y, _ ("Will run `sendmail -t' with a composed header")))->position = POSITION_FILL;
/* Toolhint */
    CSetToolHint ("mail.tcmd", _ ("Your system sendmail command may not accept the option -t.\nIf so, create a wrapper script or install GNU sendmail"));
    CGetHintPos (0, &y);
    y2 = y;

    (CDrawText ("mail.treply", win, x, y, "Reply-To: "))->hotkey = 'R';
    CGetHintPos (&w, 0);
    w -= WIDGET_SPACING;
    (CDrawTextInput ("mail.reply", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.reply")))->hotkey = 'R';
/* Toolhint */
    CSetToolHint ("mail.reply", "Your email address");
    CSetToolHint ("mail.treply", "Your email address");
    CGetHintPos (0, &y);

    (CDrawText ("mail.tfrom", win, x, y, "From: "))->hotkey = 'F';
    CSetWidgetSize ("mail.tfrom", w - x, FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2);
    (CDrawTextInput ("mail.from", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.from")))->hotkey = 'F';
/* Toolhint */
    CSetToolHint ("mail.from", "Your email address");
    CSetToolHint ("mail.tfrom", "Your email address");
    CGetHintPos (0, &y);

    (CDrawText ("mail.tto", win, x, y, "To: "))->hotkey = 'T';
    CSetWidgetSize ("mail.tto", w - x, FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2);
    (CDrawTextInput ("mail.to", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.to")))->hotkey = 'T';
/* Toolhint */
    CSetToolHint ("mail.to", _ ("Recipient address"));
    CSetToolHint ("mail.tto", _ ("Recipient address"));
    CGetHintPos (0, &y);

    (CDrawText ("mail.tsubject", win, x, y, "Subject: "))->hotkey = 'S';
    CSetWidgetSize ("mail.tsubject", w - x, FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2);
    (CDrawTextInput ("mail.subject", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.subject")))->hotkey = 'S';
    CGetHintPos (0, &y);

    (CDrawText ("mail.tcc", win, x, y, "CC: "))->hotkey = 'C';
    CSetWidgetSize ("mail.tcc", w - x, FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2);
    (CDrawTextInput ("mail.cc", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.cc")))->hotkey = 'C';
    CGetHintPos (0, &y);

    (CDrawText ("mail.tbcc", win, x, y, "Bcc: "))->hotkey = 'B';
    CSetWidgetSize ("mail.tbcc", w - x, FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2);
    (CDrawTextInput ("mail.bcc", win, w, y, (FONT_MEAN_WIDTH) * 50, AUTO_HEIGHT, 1024, CLastInput ("mail.bcc")))->hotkey = 'B';
    CGetHintPos (0, &y);

    get_hint_limits (&x, 0);
    CDrawPixmapButton ("mail.send", win, x, y2, PIXMAP_BUTTON_TICK);
    CGetHintPos (0, &y2);
    CDrawPixmapButton ("mail.cancel", win, x, y2, PIXMAP_BUTTON_CROSS);

    CIdent ("mail")->position = WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("mail");
    CMapDialog ("mail");
    CFocus (CIdent ("mail.to"));

    for (;;) {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("mail"))
	    break;
	if (!strcmp (cwevent.ident, "mail.cancel") || cwevent.command == CK_Cancel)
	    break;
	if (!strcmp (cwevent.ident, "mail.send") || cwevent.command == CK_Enter) {
	    if (!*((CIdent ("mail.to"))->text)) {
		CErrorDialog (mail_editor->mainid, 20, 20, _ (" Mail "), _ (" You must specify a `To:' address "));
	    } else {
		CHourGlass (win);
		pipe_mail ((CIdent ("mail.reply"))->text, (CIdent ("mail.from"))->text,
			   (CIdent ("mail.to"))->text, (CIdent ("mail.subject"))->text,
		(CIdent ("mail.cc"))->text, (CIdent ("mail.bcc"))->text);
		CUnHourGlass (win);
		break;
	    }
	}
    }
    CDestroyWidget ("mail");
    CRestoreState (&s);
}


void do_mail (CWidget * edit)
{
    mail_editor = edit;
    mail_subject_to_cc_dialog (edit->mainid, 20, 20);
}

#endif
