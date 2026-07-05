/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* windowmandetector.c - detects the running X11 window manager
   Copyright (C) 1996-2022 Paul Sheer
 */


/*
   Architecture:
     Twenty-five static detector functions, identically prototyped:
         static int detect_<wmname>(void);
     Each returns 1 if that WM is found, 0 otherwise.

     Detectors are ordered in a static array.  detect_window_manager()
     iterates the array and returns the WMType of the first match.

   Efficacy of the linear-scan approach:
     GOOD: correct detection order avoids false positives.  The most
     specific checks (env vars unique to one compositor) run first.
     WM-specific atoms come next.  Generic EWMH _NET_WM_NAME string
     matching runs last.  This prevents Hyprland from being mis-
     identified as "wlroots wm", or Sway from being mis-identified as
     i3, because the specific checks short-circuit before the generic
     ones fire.

     The scan stops at first match — correct when only one WM is
     running, which is the common case.  An exhaustive variant that
     scores confidence across all detectors could handle the rare
     nested-WM case (e.g. Xnest), but that is not needed here.

     All detectors are self-contained, using only the globals
     CDisplay and CRoot, already available in every cooledit
     component that includes app_glob.c.
 */

#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "coolwidget.h"
#include "app_glob.c"
#include "coollocal.h"

#define MID_X 20
#define MID_Y 20


typedef enum {
    WM_UNKNOWN = 0,
    WM_MUTTER,
    WM_KWIN,
    WM_I3,
    WM_SWAY,
    WM_HYPRLAND,
    WM_AWESOME,
    WM_ICEWM,
    WM_FVWM,
    WM_BSPWM,
    WM_DWM,
    WM_QTILE,
    WM_ENLIGHTENMENT,
    WM_WINDOW_MAKER,
    WM_TWM,
    WM_MWM,
    WM_AFTERSTEP,
    WM_BLACKBOX,
    WM_FLUXBOX,
    WM_OPENBOX,
    WM_SAWFISH,
    WM_FVWM95,
    WM_CTWM,
    WM_RATPOISON,
    WM_WMII,
    WM_XMONAD,
    WM_COUNT
} WMType;


static const char *wm_type_name[] = {
    "Unknown",
    "Mutter / GNOME Shell",
    "KWin",
    "i3",
    "Sway",
    "Hyprland",
    "Awesome",
    "IceWM",
    "FVWM",
    "bspwm",
    "dwm",
    "qtile",
    "Enlightenment",
    "Window Maker",
    "twm",
    "mwm",
    "AfterStep",
    "Blackbox",
    "Fluxbox",
    "Openbox",
    "Sawfish",
    "FVWM95",
    "ctwm",
    "ratpoison",
    "wmii",
    "xmonad"
};


/* ------------------------------------------------------------------ */
/*  Helper: read _NET_WM_NAME from the WM check window via EWMH        */
/*  Returns a strdup'd copy, or NULL on failure.  Caller must free.    */
/* ------------------------------------------------------------------ */

static char *ewmh_get_wm_name (void)
{E_
    Atom net_check, net_name, utf8, type;
    int fmt;
    unsigned long nitems, bytes;
    unsigned char *data = NULL;
    Window wm_win = None;
    char *result = NULL;

    if (!CDisplay)
	return NULL;

    net_check = XInternAtom (CDisplay, "_NET_SUPPORTING_WM_CHECK", False);
    net_name  = XInternAtom (CDisplay, "_NET_WM_NAME", False);
    utf8      = XInternAtom (CDisplay, "UTF8_STRING", False);

    if (XGetWindowProperty (CDisplay, CRoot, net_check, 0, 1, False,
			    XA_WINDOW, &type, &fmt, &nitems, &bytes,
			    &data) == Success && data && type == XA_WINDOW) {
	wm_win = *(Window *) data;
	XFree (data);
	data = NULL;

	if (wm_win != None &&
	    XGetWindowProperty (CDisplay, wm_win, net_name, 0, 64, False,
				utf8, &type, &fmt, &nitems, &bytes,
				&data) == Success && data) {
	    result = strdup ((char *) data);
	    XFree (data);
            data = NULL;
	}
    }
    if (data)
	XFree (data);
    /* Fallback: some WMs (ratpoison) set _NET_WM_NAME directly on root */
    if (!result &&
	XGetWindowProperty (CDisplay, CRoot, net_name, 0, 64, False,
			    utf8, &type, &fmt, &nitems, &bytes,
			    &data) == Success && data) {
	result = strdup ((char *) data);
	XFree (data);
	data = NULL;
    }
    if (data)
	XFree (data);
    return result;
}


/* ------------------------------------------------------------------ */
/*  Helper: check whether an atom exists on the root window            */
/*  Returns 1 if the atom is present, 0 otherwise.                     */
/* ------------------------------------------------------------------ */

static int root_property_exists (const char *atom_name)
{E_
    Atom prop, type;
    int fmt;
    unsigned long nitems, bytes;
    unsigned char *data = NULL;
    int r = 0;

    if (!CDisplay)
	return 0;

    prop = XInternAtom (CDisplay, atom_name, True);
    if (prop == None)
	return 0;

    if (XGetWindowProperty (CDisplay, CRoot, prop, 0, 1, False,
			    AnyPropertyType, &type, &fmt, &nitems, &bytes,
			    &data) == Success && data) {
	if (nitems > 0)
	    r = 1;
	XFree (data);
    }
    return r;
}


/* ------------------------------------------------------------------ */
/*  Helper: EWMH string-match detector (avoids redundant code)         */
/*  Returns 1 if _NET_WM_NAME matches the given string exactly.        */
/* ------------------------------------------------------------------ */

static int ewmh_name_is (const char *expected)
{E_
    char *name;
    int r = 0;
    name = ewmh_get_wm_name ();
    if (name) {
	r = !strcasecmp (name, expected);
	free (name);
    }
    return r;
}


/* ------------------------------------------------------------------ */
/*  Helper: read WM_CLASS from the WM_S0 selection owner               */
/*  Used for pre-EWMH WMs that own the ICCCM manager selection.        */
/*  Returns a strdup'd copy of the instance name, or NULL.             */
/* ------------------------------------------------------------------ */

static char *wm_s0_get_instance (void)
{E_
    Atom wm_s0, wm_class, type;
    int fmt;
    unsigned long nitems, bytes;
    unsigned char *data = NULL;
    Window owner;
    char *result = NULL;

    if (!CDisplay)
	return NULL;

    wm_s0 = XInternAtom (CDisplay, "WM_S0", False);
    owner = XGetSelectionOwner (CDisplay, wm_s0);
    if (owner == None)
	return NULL;

    wm_class = XInternAtom (CDisplay, "WM_CLASS", False);
    if (XGetWindowProperty (CDisplay, owner, wm_class, 0, 128, False,
			    XA_STRING, &type, &fmt, &nitems, &bytes,
			    &data) == Success && data) {
	result = strdup ((char *) data);
	XFree (data);
    }
    return result;
}


/* ================================================================== */
/*  Detector functions — all share the same prototype:                 */
/*      static int detect_<name>(void);                                */
/*  Return 1 if this WM is identified, 0 if not.                       */
/* ================================================================== */


/* --- Sway (Wayland via Xwayland) ------------------------------------
   SWAYSOCK is the definitive marker; Sway sets it before spawning
   Xwayland so all X11 clients inherit it.
   Fallback: I3SOCK + WAYLAND_DISPLAY together (i3 never sets the
   latter, so the combination is Sway-specific).
--------------------------------------------------------------------- */

static int detect_sway (void)
{E_
    if (getenv ("SWAYSOCK"))
	return 1;
    if (getenv ("I3SOCK") && getenv ("WAYLAND_DISPLAY"))
	return 1;
    return 0;
}

static const char *help_msg_sway (void)
{
    return "\
\n\
Sway is a Wayland compositor.  CoolEdit runs under Xwayland.  All of\n\
Sway's default keybindings use Super (Mod4), so there are no conflicts\n\
with CoolEdit's Alt- and Ctrl-based keys.\n\
\n\
Config file:  ~/.config/sway/config\n\
Reload with:  swaymsg reload\n\
";
}


/* --- Hyprland (Wayland via Xwayland) ---------------------------------
   HYPRLAND_INSTANCE_SIGNATURE is unique per Hyprland instance and
   inherited by Xwayland clients.
   Fallback: XDG_CURRENT_DESKTOP contains "Hyprland".
--------------------------------------------------------------------- */

static int detect_hyprland (void)
{E_
    const char *s;
    if (getenv ("HYPRLAND_INSTANCE_SIGNATURE"))
	return 1;
    s = getenv ("XDG_CURRENT_DESKTOP");
    if (s && strstr (s, "Hyprland"))
	return 1;
    return 0;
}

static const char *help_msg_hyprland (void)
{
    return "\
\n\
Hyprland is a Wayland compositor.  CoolEdit runs under Xwayland.  All\n\
of Hyprland's default keybindings use Super (Mod4), so there are no\n\
conflicts with CoolEdit's Alt- and Ctrl-based keys.\n\
\n\
Config file:  ~/.config/hypr/hyprland.conf\n\
Reload with:  hyprctl reload\n\
";
}


/* --- i3 (native X11) -------------------------------------------------
   I3_SOCKET_PATH is an atom set on the root window by i3 at startup.
   No other WM sets it — this is the definitive i3 check.
--------------------------------------------------------------------- */

static int detect_i3 (void)
{E_
    return root_property_exists ("I3_SOCKET_PATH");
}

static const char *help_msg_i3 (void)
{
    return "\
\n\
i3 defaults to Mod4 (Super/Windows key), which does not conflict with\n\
CoolEdit at all.  If your config uses Mod1 (Alt) instead, edit:\n\
\n\
    ~/.config/i3/config\n\
\n\
and either switch to the modern default:\n\
\n\
    set $mod Mod4\n\
\n\
or rebind the specific conflicting keys away from Alt+arrow and Alt+L:\n\
\n\
    bindsym $mod+Left  focus left\n\
    bindsym $mod+Down  focus down\n\
    bindsym $mod+Up    focus up\n\
    bindsym $mod+Right focus right\n\
    bindsym $mod+l     focus up\n\
\n\
Then reload with:  i3-msg reload\n\
";
}


/* --- ratpoison (native X11) ------------------------------------------
   RP_COMMAND_REQUEST is a custom atom on the root window unique to
   ratpoison.  If present, ratpoison is definitively the WM.
--------------------------------------------------------------------- */

static int detect_ratpoison (void)
{E_
    if (root_property_exists ("RP_COMMAND_REQUEST"))
	return 1;
    return ewmh_name_is ("ratpoison");
}

static const char *help_msg_ratpoison (void)
{
    return "\
\n\
Ratpoison uses Ctrl+t as an escape prefix for all commands.  Because\n\
CoolEdit has no Ctrl+T binding of its own, there are no direct key-\n\
combination conflicts between the two.\n\
\n\
If you find that the C-t prefix itself gets in the way (e.g. you type\n\
C-t followed by a letter that ratpoison intercepts), change the prefix\n\
by adding the following line to ~/.ratpoisonrc:\n\
\n\
    escape C-b\n\
\n\
Then reload with:\n\
    ratpoison -c source ~/.ratpoisonrc\n\
";
}


/* --- Blackbox (native X11) -------------------------------------------
   _BLACKBOX_HINTS is a custom atom set on the root window.  No other
   WM uses the _BLACKBOX_ prefix.  Definitive.
--------------------------------------------------------------------- */

static int detect_blackbox (void)
{E_
    if (root_property_exists ("_BLACKBOX_HINTS"))
	return 1;
    return ewmh_name_is ("Blackbox");
}

static const char *help_msg_blackbox (void)
{
    return "\
\n\
Blackbox itself has NO built-in keyboard shortcuts.  Shortcuts come from\n\
the optional bbkeys daemon.  If you are not running bbkeys, there are no\n\
conflicts with CoolEdit.\n\
\n\
If bbkeys is running and you are seeing conflicts, edit ~/.bbkeysrc and\n\
remove or comment out conflicting bindings in the [keybindings] section.\n\
Key entries use the format:  Mod1-F1 = :ShowRootMenu\n\
Remove or comment out any lines that use keys CoolEdit needs (Alt+F1..F10,\n\
Alt+arrows, Ctrl+Alt+arrows).\n\
\n\
Alternatively, simply do not start bbkeys and all keyboard shortcuts are\n\
handled by CoolEdit alone.\n\
";
}


/* --- Openbox (native X11) --------------------------------------------
   _OB_THEME is a custom root-window atom unique to Openbox.
   Fallback: EWMH _NET_WM_NAME == "Openbox".
--------------------------------------------------------------------- */

static int detect_openbox (void)
{E_
    if (root_property_exists ("_OB_THEME"))
	return 1;
    return ewmh_name_is ("Openbox");
}

static const char *help_msg_openbox (void)
{
    return "\
\n\
Openbox uses Alt+F4 (close), Ctrl+Alt+Up/Down (desktop switch), and\n\
Alt+Shift+Up/Down (send window to desktop).  These conflict with\n\
CoolEdit's debug menu, bookmark navigation, and block selection.\n\
\n\
If you do not already have ~/.config/openbox/rc.xml:\n\
\n\
    cp /etc/xdg/openbox/rc.xml ~/.config/openbox/rc.xml\n\
\n\
Edit ~/.config/openbox/rc.xml.  In the <keyboard> section, remove or\n\
comment out these <keybind> blocks:\n\
\n\
    <keybind key=\"A-F4\">       ... Close ...       </keybind>\n\
    <keybind key=\"C-A-Up\">     ... GoToDesktop ... </keybind>\n\
    <keybind key=\"C-A-Down\">   ... GoToDesktop ... </keybind>\n\
    <keybind key=\"S-A-Up\">     ... SendToDesktop . </keybind>\n\
    <keybind key=\"S-A-Down\">   ... SendToDesktop . </keybind>\n\
\n\
Then reload with:  openbox --reconfigure\n\
\n\
Note: Ctrl+Alt+Left/Right (switch desktop left/right) do NOT conflict\n\
with CoolEdit and can be left in place.\n\
";
}


/* --- Enlightenment (native X11) --------------------------------------
   ENLIGHTENMENT_VERSION is a custom atom on the root window set by
   both E16 and E17+.  Definitive — no other WM sets it.
--------------------------------------------------------------------- */

static int detect_enlightenment (void)
{E_
    return root_property_exists ("ENLIGHTENMENT_VERSION");
}

static const char *help_msg_enlightenment (void)
{
    return "\
\n\
Enlightenment (E17+) binds Alt+F1..F12 for virtual desktop switching\n\
and Ctrl+Alt+arrows/F/I/Insert for window operations.  These collide\n\
with CoolEdit's debug menu, bookmark navigation, and text selection.\n\
\n\
Fix this in the Settings panel:\n\
\n\
    Main Menu -> Settings -> All -> Input -> Key Bindings\n\
\n\
In the Key Bindings dialog, find and delete (or reassign) each binding:\n\
    Alt+F1..F10   -> (delete)    [desktop switching]\n\
    Ctrl+Alt+F    -> (delete)    [toggle fullscreen]\n\
    Ctrl+Alt+I    -> (delete)    [iconify window]\n\
    Ctrl+Alt+Up   -> (delete)    [raise window]\n\
    Ctrl+Alt+Down -> (delete)    [lower window]\n\
    Ctrl+Alt+Insert -> (delete)  [launch terminal]\n\
    Shift+Alt+Up  -> (delete)    [move area up]\n\
    Shift+Alt+Down -> (delete)   [move area down]\n\
\n\
Changes take effect immediately.  No restart needed.\n\
";
}


/* --- mwm / Motif Window Manager (native X11) -------------------------
   _MOTIF_WM_INFO is a property on the root window set by mwm.
   Pre-EWMH — no _NET_WM_NAME.  This atom is the definitive check.
--------------------------------------------------------------------- */

static int detect_mwm (void)
{E_
    return root_property_exists ("_MOTIF_WM_INFO");
}

static const char *help_msg_mwm (void)
{
    return "\
\n\
Motif Window Manager (mwm) uses default Alt+Fn key bindings that\n\
intercept CoolEdit's debug menu keys:\n\
\n\
    Alt+F3   f.lower      Alt+F4   f.kill\n\
    Alt+F5   f.restore    Alt+F8   f.resize\n\
    Alt+F9   f.minimize   Alt+F10  f.maximize\n\
\n\
    (Alt+F7 is f.move — CoolEdit does not use it, so it can stay.)\n\
\n\
To free these for CoolEdit, unbind them in ~/.mwmrc:\n\
\n\
    Keys DefaultKeyBindings\n\
    {\n\
        Alt<Key>F3   -\n\
        Alt<Key>F4   -\n\
        Alt<Key>F5   -\n\
        Alt<Key>F8   -\n\
        Alt<Key>F9   -\n\
        Alt<Key>F10  -\n\
    }\n\
\n\
Then restart mwm with:  mwm -restart\n\
";
}


/* --- KWin (native X11 or Xwayland) -----------------------------------
   KWIN_RUNNING is KWin's proprietary root-window atom, set before
   any client connects.  No other WM uses it.
   Fallback: EWMH _NET_WM_NAME == "KWin".
--------------------------------------------------------------------- */

static int detect_kwin (void)
{E_
    if (root_property_exists ("KWIN_RUNNING"))
	return 1;
    return ewmh_name_is ("KWin");
}

static const char *help_msg_kwin (void)
{
    return "\
\n\
KWin (KDE Plasma) remaps via kwriteconfig5.  Run these commands as your user:\n\
\n\
    kwriteconfig5 --file kwinrc --group Windows --key \"Window Operations Menu\" \"Alt+Ctrl+F3,none\"\n\
    kwriteconfig5 --file kwinrc --group Windows --key \"Window Close\" \"Alt+Ctrl+F4,none\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 1\" \"Shift+Ctrl+F1\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 2\" \"Shift+Ctrl+F2\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 3\" \"Shift+Ctrl+F3\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 5\" \"Shift+Ctrl+F5\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 6\" \"Shift+Ctrl+F6\"\n\
    kwriteconfig5 --file kwinrc --group Desktops --key \"Switch to Desktop 10\" \"Shift+Ctrl+F10\"\n\
    kwriteconfig5 --file kwinrc --group Windows --key \"Walk Through Desktops\" \"Shift+Ctrl+Tab\"\n\
\n\
Then reload with:  qdbus org.kde.KWin /KWin reconfigure\n\
On Plasma 6 use kwriteconfig6 instead of kwriteconfig5.\n\
";
}


/* --- Mutter / GNOME Shell (native X11 or Xwayland) -------------------
   EWMH _NET_WM_NAME is either "GNOME Shell" (normal operation) or
   "Mutter" (standalone debug mode).  Both must be checked.
   No private atoms exist.
--------------------------------------------------------------------- */

static int detect_mutter (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strcasecmp (name, "GNOME Shell") || !strcasecmp (name, "Mutter"))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_mutter (void)
{
    return "\
\n\
Mutter (GNOME) remaps via gsettings.  Run these commands as your user:\n\
\n\
    gsettings set org.gnome.desktop.wm.keybindings panel-main-menu       \"['<Alt><Ctrl>F1']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings panel-run-dialog      \"['<Alt><Ctrl>F2']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings close                 \"['<Alt><Ctrl>F4']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings unmaximize            \"['<Alt><Ctrl>F5']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings cycle-group           \"['<Alt><Ctrl>F6']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings begin-resize          \"['<Alt><Ctrl>F8']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings toggle-maximized      \"['<Alt><Ctrl>F10']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-up    \"['<Super><Alt>Up']\"\n\
    gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-down  \"['<Super><Alt>Down']\"\n\
\n\
To reset a single key to default:  gsettings reset org.gnome.desktop.wm.keybindings <key>\n\
";
}


/* --- Awesome (native X11) --------------------------------------------
   EWMH _NET_WM_NAME == "awesome".  No custom atoms, no env vars.
--------------------------------------------------------------------- */

static int detect_awesome (void)
{E_
    return ewmh_name_is ("awesome");
}

static const char *help_msg_awesome (void)
{
    return "\
\n\
Awesome WM uses Super (Mod4) by default for all window-manager bindings,\n\
so there are no conflicts with CoolEdit's Alt- and Ctrl-based keys.\n\
\n\
Config file:  ~/.config/awesome/rc.lua\n\
Restart with:  awesome-client 'awesome.restart()'\n\
";
}


/* --- IceWM (native X11) ----------------------------------------------
   EWMH _NET_WM_NAME starts with "IceWM" (e.g. "IceWM 2.9.6 (Linux/x86_64)").
--------------------------------------------------------------------- */

static int detect_icewm (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strncmp (name, "IceWM", 5))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_icewm (void)
{
    return "\
\n\
Create or update the following file in your $HOME directory:\n\
\n\
    /root/.icewm/preferences\n\
\n\
Containing:\n\
\n\
    KeyWinRaise=\"Alt+Ctrl+F1\"\n\
    KeyWinOccupyAll=\"Alt+Ctrl+F2\"\n\
    KeyWinLower=\"Alt+Ctrl+F3\"\n\
    KeyWinClose=\"Alt+Ctrl+F4\"\n\
    KeyWinRestore=\"Alt+Ctrl+F5\"\n\
    KeyWinNext=\"Alt+Ctrl+F6\"\n\
    KeyWinMove=\"Alt+Ctrl+F7\"\n\
    KeyWinSize=\"Alt+Ctrl+F8\"\n\
    KeyWinMinimize=\"Alt+Ctrl+F9\"\n\
    KeyWinMaximize=\"Alt+Ctrl+F10\"\n\
    KeySysWorkspaceLast=\"\"\n\
\n\
";
}


/* --- FVWM (native X11) -----------------------------------------------
   EWMH _NET_WM_NAME is "fvwm3" or "fvwm" depending on version.
--------------------------------------------------------------------- */

static int detect_fvwm (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strcasecmp (name, "fvwm3") || !strcasecmp (name, "fvwm"))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_fvwm (void)
{
    return "\
\n\
fvwm3 remaps via its config file.  Locate the system default:\n\
\n\
    fvwm3 --is-myvars | grep FVWM_DATADIR\n\
\n\
Then create or update ~/.fvwm/config containing:\n\
\n\
    Read /usr/local/share/fvwm/config\n\
    Key F1   A M   Nop\n\
    Key Up   WTSF CM  Nop\n\
    Key Down WTSF CM  Nop\n\
    Key F1   A CM  Menu MenuFvwmRoot\n\
    Key Up   WTSF C4  ShuffleDir up\n\
    Key Down WTSF C4  ShuffleDir down\n\
\n\
The 'Nop' lines unbind Alt+F1, Ctrl+Alt+Up, Ctrl+Alt+Down.\n\
The 'C4' lines (Ctrl+Super) rebind window-shuffle to safe alternatives.\n\
";
}


/* --- bspwm (native X11) ----------------------------------------------
   EWMH _NET_WM_NAME == "bspwm".  No custom atoms, no env vars.
--------------------------------------------------------------------- */

static int detect_bspwm (void)
{E_
    return ewmh_name_is ("bspwm");
}

static const char *help_msg_bspwm (void)
{
    return "\
\n\
bspwm has no built-in keybindings.  It relies on the external sxhkd hotkey\n\
daemon.  The upstream example sxhkdrc uses Super (Mod4) for every binding,\n\
so there are no conflicts with CoolEdit's Alt- or Ctrl-based keys.\n\
\n\
If you have added custom Alt bindings to your sxhkdrc, check:\n\
    ~/.config/sxhkd/sxhkdrc\n\
and ensure none collide with Alt+L/B/M/P/U/I/X/\\ or Alt+arrows/F-keys.\n\
\n\
To reload after changes:  pkill -USR1 -x sxhkd\n\
";
}


/* --- dwm (native X11) -------------------------------------------------
   EWMH _NET_WM_NAME == "dwm" (or "LG3D" on older JDK-workaround builds).
--------------------------------------------------------------------- */

static int detect_dwm (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strcasecmp (name, "dwm") || !strcasecmp (name, "LG3D"))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_dwm (void)
{
    return "\
\n\
dwm uses Alt (Mod1) by default at compile time.  Five bindings collide\n\
with CoolEdit:\n\
\n\
    dwm Alt+B (toggle bar)    vs  CoolEdit Alt+B (match bracket)\n\
    dwm Alt+I (inc masters)   vs  CoolEdit Alt+I (insert unicode)\n\
    dwm Alt+L (grow master)   vs  CoolEdit Alt+L (goto)\n\
    dwm Alt+M (monocle)       vs  CoolEdit Alt+M (mail)\n\
    dwm Alt+P (dmenu)         vs  CoolEdit Alt+P (paragraph format)\n\
\n\
Fix: rebind dwm to Super.  In config.h, change:\n\
\n\
    #define MODKEY Mod1Mask   -->   #define MODKEY Mod4Mask\n\
\n\
Then rebuild and restart:\n\
\n\
    sudo make clean install\n\
";
}


/* --- qtile (native X11) ----------------------------------------------
   EWMH _NET_WM_NAME == "qtile".  No custom atoms, no env vars.
--------------------------------------------------------------------- */

static int detect_qtile (void)
{E_
    return ewmh_name_is ("qtile");
}

static const char *help_msg_qtile (void)
{
    return "\
\n\
Qtile defaults to Super (Mod4) for all window-manager bindings.  There are\n\
no conflicts with any of CoolEdit's Alt- or Ctrl-based key combinations.\n\
\n\
Ctrl+Alt+F1..F7 are kernel-level VT switches, not qtile bindings; they do\n\
not conflict with CoolEdit's Ctrl+F-key combos.\n\
\n\
No changes needed.  Config file:  ~/.config/qtile/config.py\n\
To reload after changes:  qtile cmd-obj -o cmd -f reload_config\n\
";
}


/* --- Window Maker (native X11) ---------------------------------------
   EWMH _NET_WM_NAME == "Window Maker" (not set on pre-0.95.4 builds).
   No custom root-window atoms.  Older versions need process-scan
   fallback for "wmaker" (not implemented here — those versions
   predate EWMH entirely and are museum-grade).
--------------------------------------------------------------------- */

static int detect_window_maker (void)
{E_
    return ewmh_name_is ("Window Maker");
}

static const char *help_msg_window_maker (void)
{
    return "\
\n\
Window Maker uses Ctrl+arrow and Ctrl+Shift+Up keys for window\n\
management.  These conflict with CoolEdit's cursor movement keys:\n\
\n\
    Ctrl+Up          FocusNextKey       Ctrl+Down    MiniaturizeKey\n\
    Ctrl+Left        PrevWorkspaceKey   Ctrl+Right   NextWorkspaceKey\n\
    Shift+Up         FocusPrevKey       Ctrl+Shift+Up RaiseLowerKey\n\
\n\
Fix: Open WPrefs (or Desktop -> Preferences Utility), go to Keyboard\n\
Shortcuts, select each conflicting entry and press Clear.  Alternatively\n\
edit ~/GNUstep/Defaults/WindowMaker and add:\n\
\n\
    FocusNextKey = NONE;\n\
    MiniaturizeKey = NONE;\n\
    PrevWorkspaceKey = NONE;\n\
    NextWorkspaceKey = NONE;\n\
    FocusPrevKey = NONE;\n\
    RaiseLowerKey = NONE;\n\
\n\
Then restart Window Maker (right-click desktop -> Exit -> Restart).\n\
";
}


/* --- AfterStep (native X11) ------------------------------------------
   EWMH _NET_WM_NAME == "AfterStep".  No custom atoms, no env vars.
--------------------------------------------------------------------- */

static int detect_afterstep (void)
{E_
    return ewmh_name_is ("AfterStep");
}

static const char *help_msg_afterstep (void)
{
    return "\
\n\
AfterStep's default feel file binds Ctrl+arrow keys to virtual desktop\n\
scrolling.  These conflict with CoolEdit's word-movement keys.\n\
\n\
Open ~/.afterstep/feel and comment out (or remove) these four lines:\n\
\n\
    Key Left    A   C   Scroll -100 0\n\
    Key Right   A   C   Scroll +100 0\n\
    Key Up      A   C   Scroll 0 -100\n\
    Key Down    A   C   Scroll 0 +100\n\
\n\
Then reload: Desktop menu -> Desktop -> Feels -> feel\n\
\n\
Alternatively, switch to the ICCCM-compliant feel with zero WM\n\
keybindings: Desktop menu -> Desktop -> Feels -> feel.ICCCM\n\
\n\
AfterStep has no other default keyboard shortcuts.\n\
";
}


/* --- Fluxbox (native X11) --------------------------------------------
   EWMH _NET_WM_NAME == "Fluxbox".  Also carries _BLACKBOX_ATTRIBUTES
   on the root window (inherited from Blackbox), used as confirmation.
--------------------------------------------------------------------- */

static int detect_fluxbox (void)
{E_
    if (ewmh_name_is ("Fluxbox"))
	return 1;
    /* Fluxbox also inherits _BLACKBOX_ATTRIBUTES — but Blackbox's
       _BLACKBOX_HINTS check fires first, so this fallback is safe. */
    if (root_property_exists ("_BLACKBOX_ATTRIBUTES"))
	return 1;
    return 0;
}

static const char *help_msg_fluxbox (void)
{
    return "\
\n\
Fluxbox binds Alt+F1..F12 to workspace switching in its default keys\n\
file.  These conflict with CoolEdit's Alt+F-key bindings.\n\
\n\
Fix this by copying and editing the keys file:\n\
\n\
    cp /usr/share/fluxbox/keys ~/.fluxbox/keys\n\
\n\
Edit ~/.fluxbox/keys and remove (or comment with #) these lines:\n\
\n\
    Mod1 F1 :Workspace 1      Mod1 F6 :Workspace 6\n\
    Mod1 F2 :Workspace 2      Mod1 F8 :Workspace 8\n\
    Mod1 F3 :Workspace 3      Mod1 F9 :Workspace 9\n\
    Mod1 F4 :Workspace 4      Mod1 F10 :Workspace 10\n\
    Mod1 F5 :Workspace 5\n\
\n\
(F7, F11, F12 are safe — CoolEdit does not bind them.)\n\
\n\
To keep workspace switching under the Super key instead:\n\
\n\
    Mod4 F1 :Workspace 1\n\
    ...etc...\n\
\n\
Then reload with:  fluxbox-remote reconfigure\n\
";
}


/* --- Sawfish (native X11) --------------------------------------------
   EWMH _NET_WM_NAME == "Sawfish" (or "sawmill" for 0.x era).
--------------------------------------------------------------------- */

static int detect_sawfish (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strcasecmp (name, "Sawfish") || !strcasecmp (name, "sawmill"))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_sawfish (void)
{
    return "\
\n\
Sawfish's default keyboard shortcuts use the 'W' modifier, which defaults\n\
to Meta.  On standard PC keyboards lacking a physical Meta key, Sawfish\n\
falls back to Alt.  The conflicting bindings are:\n\
\n\
    W-Left   previous workspace  (usually Alt+Left  — conflict)\n\
    W-Right  next workspace      (usually Alt+Right — conflict)\n\
    W-Up     raise window        (usually Alt+Up    — conflict)\n\
    W-Down   lower window        (usually Alt+Down  — conflict)\n\
    W-Tab    cycle windows       (usually Alt+Tab   — no conflict)\n\
\n\
To free Alt+arrows for CoolEdit, unbind them in ~/.sawfishrc:\n\
\n\
    (unbind-keys global-keymap \"W-Left\" 'previous-workspace)\n\
    (unbind-keys global-keymap \"W-Right\" 'next-workspace)\n\
    (unbind-keys window-keymap \"W-Up\" 'raise-window)\n\
    (unbind-keys window-keymap \"W-Down\" 'lower-window)\n\
\n\
Alternatively, switch W to the Super key.  Add to ~/.sawfishrc:\n\
\n\
    (setq wm-modifier-value 'Super)\n\
\n\
Then restart Sawfish (M-x restart from the Sawfish UI).\n\
";
}


/* --- ctwm (native X11) -----------------------------------------------
   EWMH _NET_WM_NAME == "ctwm" (since 4.0.0, 2017).  Pre-4.0 builds
   used the legacy _WIN_SUPPORTING_WM_CHECK atom on root.
--------------------------------------------------------------------- */

static int detect_ctwm (void)
{E_
    return ewmh_name_is ("ctwm");
}

static const char *help_msg_ctwm (void)
{
    return "\
\n\
No default keyboard shortcuts — all interaction is mouse-driven by default.\n\
Window operations use titlebar buttons and mouse gestures on window\n\
decorations.  ctwm supports keyboard shortcuts (including WarpRing for\n\
Alt+Tab-style window switching), but they must be configured explicitly in\n\
~/.ctwmrc before they become active.\n\
\n\
No key binding conflicts with CoolEdit.\n\
";
}


/* --- wmii (native X11) -----------------------------------------------
   EWMH _NET_WM_NAME == "wmii".  No custom atoms on root.
--------------------------------------------------------------------- */

static int detect_wmii (void)
{E_
    return ewmh_name_is ("wmii");
}

static const char *help_msg_wmii (void)
{
    return "\
\n\
wmii uses Alt (Mod1) by default.  Three bindings collide with CoolEdit:\n\
\n\
    wmii Alt+L (focus right)  vs  CoolEdit Alt+L (goto)\n\
    wmii Alt+M (max layout)   vs  CoolEdit Alt+M (mail)\n\
    wmii Alt+P (program menu) vs  CoolEdit Alt+P (paragraph format)\n\
\n\
(Some versions also bind Alt+B for previous tag — CoolEdit uses Alt+B\n\
for match bracket.)\n\
\n\
Fix: rebind wmii to Super.  In ~/.wmii/wmiirc, change:\n\
\n\
    MODKEY=Mod1   -->   MODKEY=Mod4\n\
\n\
Then quit wmii and restart it:\n\
    wmiir xwrite /ctl quit\n\
";
}


/* --- xmonad (native X11) ---------------------------------------------
   EWMH _NET_WM_NAME == "xmonad" (may be overridden to "LG3D" by
   the SetWMName hook — check both).
--------------------------------------------------------------------- */

static int detect_xmonad (void)
{E_
    char *name;
    int r = 0;

    name = ewmh_get_wm_name ();
    if (name) {
	if (!strcasecmp (name, "xmonad") || !strcasecmp (name, "LG3D"))
	    r = 1;
	free (name);
    }
    return r;
}

static const char *help_msg_xmonad (void)
{
    return "\
\n\
xmonad uses Alt (mod1Mask) by default.  Three bindings collide:\n\
\n\
    xmonad Alt+L (expand master)  vs  CoolEdit Alt+L (goto)\n\
    xmonad Alt+M (focus master)   vs  CoolEdit Alt+M (mail)\n\
    xmonad Alt+P (dmenu)          vs  CoolEdit Alt+P (paragraph format)\n\
\n\
Fix: rebind xmonad to Super.  In ~/.xmonad/xmonad.hs, change:\n\
\n\
    import XMonad\n\
    main = xmonad def\n\
      { modMask = mod4Mask\n\
      , terminal = \"alacritty\"  -- or your preferred terminal\n\
      }\n\
\n\
Then recompile and restart:\n\
\n\
    xmonad --recompile && xmonad --restart\n\
";
}


/* --- twm (native X11) -------------------------------------------------
   Pre-EWMH — no _NET_SUPPORTING_WM_CHECK.  Detection by elimination:
   1. WM_S0 selection is owned
   2. _MOTIF_WM_INFO absent (rules out mwm)
   3. WM_CLASS on selection owner == "twm" / "Twm"
--------------------------------------------------------------------- */

static int detect_twm (void)
{E_
    char *instance;

    if (root_property_exists ("_MOTIF_WM_INFO"))
	return 0;		/* mwm, not twm */

    instance = wm_s0_get_instance ();
    if (instance) {
	/* WM_CLASS is two null-terminated strings: instance\0class\0 */
	if (!strcmp (instance, "twm") || !strcmp (instance, "Twm")) {
	    free (instance);
	    return 1;
	}
	free (instance);
    }
    return 0;
}

static const char *help_msg_twm (void)
{
    return "\
\n\
No default keyboard shortcuts — all interaction is mouse-driven.  Click\n\
and drag window titlebars to move, click titlebuttons to iconify or\n\
resize, and click (or hold Meta+click) on the desktop for menus.\n\
\n\
No key binding conflicts with CoolEdit.\n\
";
}


/* --- FVWM95 (native X11) ----------------------------------------------
   Pre-EWMH (1997 fork).  No _NET_WM_NAME.  Use WM_S0 selection owner
   + WM_CLASS: instance="fvwm95", class="Fvwm95".
--------------------------------------------------------------------- */

static int detect_fvwm95 (void)
{E_
    char *instance;

    instance = wm_s0_get_instance ();
    if (instance) {
	if (!strcmp (instance, "fvwm95") || !strcmp (instance, "Fvwm95")) {
	    free (instance);
	    return 1;
	}
	/* Also check past the first \0 for the class name */
	char *cls = instance + strlen (instance) + 1;
	if (cls < instance + 256 && (!strcmp (cls, "Fvwm95"))) {
	    free (instance);
	    return 1;
	}
	free (instance);
    }
    return 0;
}

static const char *help_msg_fvwm95 (void)
{
    return "\
\n\
FVWM95 ships with a few default keyboard shortcuts that conflict with\n\
CoolEdit:\n\
\n\
    Alt+F1   root menu           (conflict — CoolEdit Debug: Enter Command)\n\
    Ctrl+F1  switch to desktop 1 (conflict — CoolEdit Man Page)\n\
    Ctrl+F2  switch to desktop 2 (conflict — CoolEdit Save Desktop)\n\
    Ctrl+F3  switch to desktop 3 (conflict — CoolEdit New Window)\n\
    Ctrl+F4  switch to desktop 4 (OK — CoolEdit does not bind Ctrl+F4)\n\
    Alt+F4   Close window        (commonly present — CoolEdit Debug: Continue)\n\
\n\
To free these for CoolEdit, add to ~/.fvwm95rc:\n\
\n\
    Key F1   A M  -\n\
    Key F1   A C  -\n\
    Key F2   A C  -\n\
    Key F3   A C  -\n\
    Key F4   A M  -\n\
\n\
Then restart FVWM95 or select \"Restart fvwm95\" from the root menu.\n\
";
}


/* ================================================================== */
/*  Detector table and master dispatch function                        */
/* ================================================================== */

typedef int (*wm_detector_fn) (void);
typedef const char *(*wm_help_msg_fn) (void);

struct wm_detectors_s {
    wm_detector_fn      fn;
    WMType              type;
    wm_help_msg_fn      help_fn;
};

static const struct wm_detectors_s wm_detectors[] = {
    /* env-var checks first (zero X round-trips) */
    { detect_sway,          WM_SWAY,         help_msg_sway },
    { detect_hyprland,      WM_HYPRLAND,     help_msg_hyprland },

    /* custom-atom checks (one round-trip, definitive) */
    { detect_i3,            WM_I3,           help_msg_i3 },
    { detect_ratpoison,     WM_RATPOISON,    help_msg_ratpoison },
    { detect_blackbox,      WM_BLACKBOX,     help_msg_blackbox },
    { detect_openbox,       WM_OPENBOX,      help_msg_openbox },
    { detect_enlightenment, WM_ENLIGHTENMENT, help_msg_enlightenment },
    { detect_kwin,          WM_KWIN,         help_msg_kwin },

    /* EWMH string-match: EWMH-specific strings first, then generic */
    { detect_mutter,        WM_MUTTER,       help_msg_mutter },
    { detect_icewm,         WM_ICEWM,        help_msg_icewm },
    { detect_fvwm,          WM_FVWM,         help_msg_fvwm },
    { detect_window_maker,  WM_WINDOW_MAKER, help_msg_window_maker },
    { detect_afterstep,     WM_AFTERSTEP,    help_msg_afterstep },
    { detect_fluxbox,       WM_FLUXBOX,      help_msg_fluxbox },
    { detect_sawfish,       WM_SAWFISH,      help_msg_sawfish },
    { detect_bspwm,         WM_BSPWM,        help_msg_bspwm },
    { detect_dwm,           WM_DWM,          help_msg_dwm },
    { detect_qtile,         WM_QTILE,        help_msg_qtile },
    { detect_ctwm,          WM_CTWM,         help_msg_ctwm },
    { detect_wmii,          WM_WMII,         help_msg_wmii },
    { detect_xmonad,        WM_XMONAD,       help_msg_xmonad },
    { detect_awesome,       WM_AWESOME,      help_msg_awesome },

    /* pre-EWMH WMs last (weak signals, fallback only) */
    { detect_mwm,           WM_MWM,          help_msg_mwm },
    { detect_twm,           WM_TWM,          help_msg_twm },
    { detect_fvwm95,        WM_FVWM95,       help_msg_fvwm95 },

    { NULL,                 WM_UNKNOWN       }
};


/*
   detect_window_manager() — iterate detectors, return the first match.
   Returns "Unknown" if no detector fires.

   The env-var and atom-based detectors are O(1) or a single X round-
   trip.  EWMH-based detectors cost two round-trips each (one to read
   _NET_SUPPORTING_WM_CHECK, one to read _NET_WM_NAME).  Because the
   array is sorted with cheap checks first, the average case is fast.

   The pre-EWMH WMs (twm, FVWM95) use the WM_S0 selection-owner
   method — one round-trip to get the owner, one to read WM_CLASS.
   They are placed before the EWMH string-match tier because an
   affirmative result short-circuits the scan.

   Combined worst case (none of the fast checks hit, then many EWMH
   string-compare detectors run): each EWMH detector calls
   ewmh_get_wm_name() independently, producing redundant X round-
   trips.  An optimisation would be to cache the EWMH name once, but
   that adds shared mutable state.  The current design favours
   simplicity — the redundant round-trips total a few milliseconds
   and happen exactly once at application start-up.

   Note: twm and xmonad both use WM_CLASS for detection.  xmonad is
   an EWMH WM but its _NET_WM_NAME can be "LG3D" by user override,
   so the EWMH check is used (see detect_xmonad).  twm is pre-EWMH
   and must use the WM_S0/WM_CLASS path.
 */

const char *detect_window_manager (void)
{E_
    int i;

    for (i = 0; wm_detectors[i].fn; i++) {
	if (wm_detectors[i].fn ()) {
	    if (wm_detectors[i].type > 0 &&
		wm_detectors[i].type < WM_COUNT)
		return wm_type_name[wm_detectors[i].type];
	    return "Unknown";
	}
    }
    return "Unknown";
}

const char *get_wm_help_text (void)
{E_
    int i;

    for (i = 0; wm_detectors[i].fn; i++) {
	if (wm_detectors[i].fn ()) {
	    if (wm_detectors[i].help_fn)
		return wm_detectors[i].help_fn ();
	    return NULL;
	}
    }
    return NULL;
}

void show_wm_key_conflicts (unsigned long data)
{E_
    const char *wm_name;
    const char *help_text;
    char heading[256];
    char descr[512];
    Window win;
    CEvent cwevent;
    CState s;
    int x, y;

    (void) data;

    wm_name = detect_window_manager ();
    help_text = get_wm_help_text ();

    snprintf (heading, sizeof (heading), "%s", _("Window Manager Key Conflicts"));
    snprintf (descr, sizeof (descr),
	_("I have detected you have %s window manager.\n"
	  "To avoid key conflicts with CoolEdit, here are\n"
	  "instructions to correct your window manager key bindings."),
	wm_name);

    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_wmkeyconflicts", 0, MID_X, MID_Y, heading);
    CGetHintPos (&x, &y);
    CDrawText ("_wmkeyconflicts.text", win, x, y, "%s", descr);
    CGetHintPos (0, &y);
    CPushFont ("editor", 0);
    CDrawTextbox ("_wmkeyconflicts.tbox", win, x, y,
		  FONT_MEAN_WIDTH * 70 + 30,
		  FONT_HEIGHT * 20 + TEXT_RELIEF * 2 + 2,
		  0, 0,
		  help_text ? help_text : "",
		  TEXTBOX_MAN_PAGE);
    CPopFont ();
    CGetHintPos (NULL, &y);
    (CDrawPixmapButton ("_wmkeyconflicts.cancel", win, 0, y, PIXMAP_BUTTON_CROSS))->position = POSITION_BOTTOM | POSITION_CENTRE;
    CSetSizeHintPos ("_wmkeyconflicts");
    CMapDialog ("_wmkeyconflicts");
    CFocus (CIdent ("_wmkeyconflicts.cancel"));
    CIdent ("_wmkeyconflicts")->position = WINDOW_ALWAYS_RAISED;
    for (;;) {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_wmkeyconflicts"))
	    break;
	if (!cwevent.handled && cwevent.command == CK_Cancel)
	    break;
	if (!strcmp (cwevent.ident, "_wmkeyconflicts.cancel"))
	    break;
    }
    CDestroyWidget ("_wmkeyconflicts");
    CRestoreState (&s);
}
