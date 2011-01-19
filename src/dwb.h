/*
 * Copyright (c) 2010-2011 Stefan Bolte <portix@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DWB_H
#define DWB_H
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <libsoup/soup.h>
#include <locale.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>
#include <gdk/gdkkeysyms.h> 

#define NAME "dwb"
/* SETTINGS MAKROS {{{*/
#define KEY_SETTINGS "Dwb Key Settings"
#define SETTINGS "Dwb Settings"

#define SINGLE_INSTANCE 1
#define NEW_INSTANCE 2

#define STRING_LENGTH 1024

// SETTTINGS_VIEW %s: bg-color  %s: fg-color %s: border
#define SETTINGS_VIEW "<head>\n<style type=\"text/css\">\n \
  body { background-color: %s; color: %s; text-align:center; }\n\
  table { border-spacing:0px; }\n\
  th { border:%s; }\n \
  .line { vertical-align: middle; }\n \
  .text { float: left; font: normal 12px helvetica; }\n \
  .key { text-align: center;  font-size: 12; }\n \
  .active { background-color: #660000; }\n \
  h2 { font: bold 16px verdana; font-variant: small-caps; }\n \
  .alignCenter { margin-left: 25%%; width: 50%%; }\n \
  input { font:normal 12px helvetica;  }\
</style>\n \
<script type=\"text/javascript\">\n  \
function get_value(e) { value = e.value ? e.id + \" \" + e.value : e.id; console.log(value); e.blur(); } \
</script>\n<noscript>Enable scripts to add settings!</noscript>\n</head>\n"
#define HTML_H2  "<h2>%s -- Profile: %s</h2>"

#define HTML_BODY_START "<body>\n"
#define HTML_BODY_END "</body>\n"
#define HTML_FORM_START "<form onsubmit=\"return false\">\n<table width=100%; style=\"border:thin solid black\">"
#define HTML_FORM_END "</form>\n</table>\n</div>\n"
#define HTML_DIV_START "<tr>\n"
#define HTML_DIV_KEYS_TEXT "<th width=100%%><div class=\"text\">%s</div>\n</th> "
#define HTML_DIV_KEYS_VALUE "<th><div class=\"key\">\n <input onchange=\"get_value(this)\" id=\"%s\" value=\"%s %s\"/>\n</div></th>\n"
#define HTML_DIV_SETTINGS_VALUE "<th><div class=\"key\">\n <input onchange=\"get_value(this);\" id=\"%s\" value=\"%s\"/>\n</div>\n</th>"
#define HTML_DIV_SETTINGS_CHECKBOX "<th><div class=\"key\"\n <input id=\"%s\" type=\"checkbox\" onchange=\"get_value(this);\" %s>\n</div>\n</th>"
#define HTML_DIV_END "</tr>\n"
/*}}}*/
#define INSERT "Insert Mode"

#define NO_URL                      "No URL in current context"
#define NO_HINTS                    "No Hints in current context"

#define HINT_SEARCH_SUBMIT "_dwb_search_submit_"


/* MAKROS {{{*/ 
#define LENGTH(X)   (sizeof(X)/sizeof(X[0]))
#define GLENGTH(X)  (sizeof(X)/g_array_get_element_size(X)) 
#define NN(X)       ( ((X) == 0) ? 1 : (X) )

#define CLEAN_STATE(X) (X->state & ~(GDK_SHIFT_MASK) & ~(GDK_BUTTON1_MASK) & ~(GDK_BUTTON2_MASK) & ~(GDK_BUTTON3_MASK) & ~(GDK_BUTTON4_MASK) & ~(GDK_BUTTON5_MASK) & ~(GDK_LOCK_MASK) & ~(GDK_MOD2_MASK) &~(GDK_MOD3_MASK) & ~(GDK_MOD5_MASK))
#define CLEAN_SHIFT(X) (X->state & ~(GDK_SHIFT_MASK) & ~(GDK_LOCK_MASK))
#define CLEAN_COMP_MODE(X)          (X & ~(COMPLETION_MODE) & ~(AUTO_COMPLETE))

#define GET_TEXT()                  (gtk_entry_get_text(GTK_ENTRY(dwb.gui.entry)))
#define CURRENT_VIEW()              ((View*)dwb.state.fview->data)
#define VIEW(X)                     ((View*)X->data)
#define WEBVIEW(X)                  (WEBKIT_WEB_VIEW(((View*)X->data)->web))
#define CURRENT_WEBVIEW()           (WEBKIT_WEB_VIEW(((View*)dwb.state.fview->data)->web))
#define VIEW_FROM_ARG(X)            (X && X->p ? ((GSList*)X->p)->data : dwb.state.fview->data)
#define WEBVIEW_FROM_ARG(arg)       (WEBKIT_WEB_VIEW(((View*)(arg && arg->p ? ((GSList*)arg->p)->data : dwb.state.fview->data))->web))
#define CLEAR_COMMAND_TEXT(X)       dwb_set_status_bar_text(VIEW(X)->lstatus, NULL, NULL, NULL)

#define CURRENT_HOST()            (webkit_security_origin_get_host(webkit_web_frame_get_security_origin(webkit_web_view_get_main_frame(CURRENT_WEBVIEW()))))

#define FREE(X)                     if ((X)) g_free((X))
#define DIGIT(X)   (X->keyval >= GDK_0 && X->keyval <= GDK_9)
#define ALPHA(X)    ((X->keyval >= GDK_A && X->keyval <= GDK_Z) ||  (X->keyval >= GDK_a && X->keyval <= GDK_z) || X->keyval == GDK_space)

#define DWB_TAB_KEY(e)              (e->keyval == GDK_Tab || e->keyval == GDK_ISO_Left_Tab)

// Settings
#define GET_CHAR(prop)              ((char*)(((WebSettings*)g_hash_table_lookup(dwb.settings, prop))->arg.p))
#define GET_BOOL(prop)              (((WebSettings*)g_hash_table_lookup(dwb.settings, prop))->arg.b)
#define GET_INT(prop)               (((WebSettings*)g_hash_table_lookup(dwb.settings, prop))->arg.i)
#define GET_DOUBLE(prop)            (((WebSettings*)g_hash_table_lookup(dwb.settings, prop))->arg.d)
/*}}}*/


/* TYPES {{{*/

typedef struct _Arg Arg;
typedef struct _Color Color;
typedef struct _Completions Completions;
typedef struct _Dwb Dwb;
typedef struct _FileContent FileContent;
typedef struct _Files Files;
typedef struct _Font DwbFont;
typedef struct _FunctionMap FunctionMap;
typedef struct _Gui Gui;
typedef struct _Key Key;
typedef struct _KeyMap KeyMap;
typedef struct _KeyValue KeyValue;
typedef struct _Misc Misc;
typedef struct _Navigation Navigation;
typedef struct _Plugin Plugin;
typedef struct _Quickmark Quickmark;
typedef struct _Settings Settings;
typedef struct _State State;
typedef struct _View View;
typedef struct _ViewStatus ViewStatus;
typedef struct _WebSettings WebSettings;
/*}}}*/
typedef gboolean (*Command_f)(void*);
typedef gboolean (*Func)(void *, void*);
typedef void (*void_func)(void*);
typedef void (*S_Func)(void *, WebSettings *);
typedef void *(*Content_Func)(const char *);

typedef unsigned int CompletionType;
#define COMP_NONE         0x00
#define COMP_BOOKMARK     0x01
#define COMP_HISTORY      0x02
#define COMP_SETTINGS     0x03
#define COMP_KEY          0x04
#define COMP_COMMAND      0x05
#define COMP_USERSCRIPT   0x06
#define COMP_INPUT        0x07
#define COMP_SEARCH       0x08

typedef unsigned int TabBarVisible;
#define HIDE_TB_NEVER     0x02
#define HIDE_TB_ALWAYS    0x03
#define HIDE_TB_TILED     0x05

typedef unsigned int Mode;
#define NORMAL_MODE           1<<0
#define INSERT_MODE           1<<1
#define QUICK_MARK_SAVE       1<<3
#define QUICK_MARK_OPEN       1<<4 
#define HINT_MODE             1<<5
#define FIND_MODE             1<<6
#define COMPLETION_MODE       1<<7
#define AUTO_COMPLETE         1<<8
#define COMMAND_MODE          1<<9
#define SEARCH_FIELD_MODE     1<<10
#define SETTINGS_MODE         1<<12
#define KEY_MODE              1<<13
#define DOWNLOAD_GET_PATH     1<<14
#define SAVE_SESSION          1<<15


typedef unsigned int ShowMessage;
#define NEVER_SM      0x00 
#define ALWAYS_SM     0x01 
#define POST_SM       0x02

typedef unsigned int Open;
#define OPEN_NORMAL      0x00 
#define OPEN_NEW_VIEW    0x01
#define OPEN_NEW_WINDOW  0x02
#define OPEN_DOWNLOAD    0x03

typedef unsigned int Layout;
#define NORMAL_LAYOUT    0
#define BOTTOM_STACK     1<<0 
#define MAXIMIZED        1<<1 

typedef unsigned int DwbType;
#define CHAR        0x01
#define INTEGER     0x02
#define DOUBLE      0x03
#define BOOLEAN     0x04
#define COLOR_CHAR  0x05
#define HTML_STRING 0x06

typedef unsigned int SettingsScope;
#define APPLY_GLOBAL    0x01
#define APPLY_PER_VIEW  0x02

typedef unsigned int DownloadAction;
#define DL_ACTION_DOWNLOAD  0x01
#define DL_ACTION_EXECUTE   0x02

enum _Direction {
  SCROLL_UP             = GDK_SCROLL_UP,
  SCROLL_DOWN           = GDK_SCROLL_DOWN,
  SCROLL_LEFT           = GDK_SCROLL_LEFT, 
  SCROLL_RIGHT          = GDK_SCROLL_RIGHT, 
  SCROLL_HALF_PAGE_UP,
  SCROLL_HALF_PAGE_DOWN,
  SCROLL_PAGE_UP,
  SCROLL_PAGE_DOWN, 
  SCROLL_TOP,
  SCROLL_BOTTOM,
} Direction;
/*}}}*/


/* STRUCTS {{{*/
struct _Navigation {
  char *first;
  char *second;
};
struct _Arg {
  guint n;
  int i;
  double d;
  gpointer p;
  gboolean b;
  char *e;
};
struct _Key {
  char *str;
  guint mod;
};
struct _KeyValue {
  const char *id;
  Key key;
};

#define FM_COMMANDLINE    1<<0
#define FM_DONT_SAVE      1<<1
struct _FunctionMap {
  Navigation n;
  int prop; 
  Func func;
  const char *error; 
  ShowMessage hide;
  Arg arg;
  gboolean entry;
};
struct _KeyMap {
  const char *key;
  guint mod;
  FunctionMap *map;
};
struct _Quickmark {
  char *key; 
  Navigation *nav;
};
struct _Completions {
  GList *active_comp;
  GList *completions;
  GList *auto_c;
  GList *active_auto_c;
  gboolean autocompletion;
  GList *path_completion;
  GList *active_path;
};

struct _State {
  GList *views;
  GList *fview;
  WebKitWebSettings *web_settings;
  Mode mode;
  GString *buffer;
  int nummod;
  Open nv;
  DwbType type;
  guint scriptlock;
  int size;
  GHashTable *settings_hash;
  SettingsScope setting_apply;
  gboolean forward_search;

  SoupCookieJar *cookiejar;
  SoupCookie *last_cookie;
  GSList *last_cookies;
  gboolean cookies_allowed;

  gboolean complete_history;
  gboolean complete_bookmarks;
  gboolean complete_searchengines;
  gboolean complete_commands;
  gboolean complete_userscripts;

  gboolean view_in_background;

  Layout layout;
  GList *last_com_history;

  GList *undo_list;

  char *search_engine;
  char *form_name;

  WebKitDownload *download;
  DownloadAction dl_action;
  char *download_command;
  char *mimetype_request;

  TabBarVisible tabbar_visible;
};

struct _WebSettings {
  Navigation n;
  gboolean builtin;
  gboolean global;
  DwbType type;
  Arg arg;
  S_Func func;
};
struct _ViewStatus {
  guint message_id;
  gboolean add_history;
  char *search_string;
  GList *downloads;
  char *current_host;
  int items_blocked;
  gboolean block;
  gboolean block_current;
  gboolean custom_encoding;
  char *mimetype;
  gboolean plugin_blocker;
  gboolean adblocker;
  Plugin *plugins;
};
struct _Plugin {
  char *uri;
  Plugin *next;
};

struct _View {
  GtkWidget *vbox;
  GtkWidget *web;
  GtkWidget *tabevent;
  GtkWidget *tablabel;
  GtkWidget *statusbox;
  GtkWidget *rstatus;
  GtkWidget *lstatus;
  GtkWidget *scroll; 
  GtkWidget *entry;
  GtkWidget *autocompletion;
  GtkWidget *compbox;
  GtkWidget *bottombox;
  View *next;
  ViewStatus *status;
  GHashTable *setting;
};
struct _Color {
  GdkColor active_fg;
  GdkColor active_bg;
  GdkColor normal_fg;
  GdkColor normal_bg;
  GdkColor tab_active_fg;
  GdkColor tab_active_bg;
  GdkColor tab_normal_fg;
  GdkColor tab_normal_bg;
  GdkColor insert_bg;
  GdkColor insert_fg;
  GdkColor error;
  GdkColor active_c_fg;
  GdkColor active_c_bg;
  GdkColor normal_c_fg;
  GdkColor normal_c_bg;
  GdkColor download_fg;
  GdkColor download_bg;
  char *settings_bg_color;
  char *settings_fg_color;
};
struct _Font {
  PangoFontDescription *fd_normal;
  PangoFontDescription *fd_bold;
  PangoFontDescription *fd_oblique;
  int active_size;
  int normal_size;
};
struct _Setting {
  gboolean inc_search;
  gboolean wrap_search;
};

struct _Gui {
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *topbox;
  GtkWidget *paned;
  GtkWidget *right;
  GtkWidget *left;
  GtkWidget *entry;
  GtkWidget *downloadbar;
  int width;
  int height;
};
struct _Misc {
  const char *name;
  const char *prog_path;
  char *systemscripts;
  char *scripts;
  const char *profile;
  const char *default_search;
  SoupSession *soupsession;
  SoupURI *proxyuri;

  GIOChannel *si_channel;
  GList *userscripts;

  double factor;
  int max_c_items;
  int message_delay;
  int history_length;

  char *settings_border;
  int argc;
  char **argv;

  gboolean tabbed_browsing;


  char *startpage;
  char *download_com;

  char *content_block_regex;
};
struct _Files {
  const char *bookmarks;
  const char *content_block_allow;
  const char *cookies;
  const char *cookies_allow;
  const char *download_path;
  const char *fifo;
  const char *history;
  const char *keys;
  const char *mimetypes;
  const char *quickmarks;
  const char *scriptdir;
  const char *searchengines;
  const char *session;
  const char *settings;
  const char *stylesheet;
  const char *unifile;
  const char *userscripts;
  const char *plugins_allow;
  const char *adblock;
};
struct _FileContent {
  GList *bookmarks;
  GList *history;
  GList *quickmarks;
  GList *searchengines;
  GList *se_completion;
  GList *keys;
  GList *settings;
  GList *cookies_allow;
  GList *commands;
  GList *mimetypes;
  GList *content_block_allow;
  GList *content_allow;
  GList *plugins_allow;
  GList *adblock;
};

struct _Dwb {
  Gui gui;
  Color color;
  DwbFont font;
  Misc misc;
  State state;
  Completions comps;
  GList *keymap;
  GHashTable *settings;
  Files files;
  FileContent fc;
};

/*}}}*/

/* VARIABLES {{{*/
Dwb dwb;
/*}}}*/

gboolean dwb_insert_mode(Arg *);
void dwb_normal_mode(gboolean);

void dwb_load_uri(Arg *);
void dwb_execute_user_script(KeyMap *km, Arg *a);

void dwb_focus_entry(void);
void dwb_focus_scroll(GList *);

gboolean dwb_update_search(gboolean forward);

void dwb_set_normal_message(GList *, gboolean, const char *, ...);
void dwb_set_error_message(GList *, const char *, ...);
void dwb_set_status_text(GList *, const char *, GdkColor *,  PangoFontDescription *);
void dwb_set_status_bar_text(GtkWidget *, const char *, GdkColor *,  PangoFontDescription *);
void dwb_update_status_text(GList *gl, GtkAdjustment *);
void dwb_update_status(GList *gl);
void dwb_update_layout(void);
void dwb_focus(GList *gl);

gboolean dwb_prepend_navigation(GList *, GList **);
void dwb_prepend_navigation_with_argument(GList **, const char *, const char *);

Navigation * dwb_navigation_from_webkit_history_item(WebKitWebHistoryItem *);
gboolean dwb_update_hints(GdkEventKey *);
gboolean dwb_search(Arg *);
void dwb_submit_searchengine(void);
void dwb_save_searchengine(void);
char * dwb_execute_script(const char *, gboolean);
void dwb_resize(double );
void dwb_toggle_tabbar(void);

void dwb_grab_focus(GList *);
void dwb_source_remove(GList *);
gboolean dwb_handle_mail(const char *uri);

int dwb_entry_position_word_back(int position);
int dwb_entry_position_word_forward(int position);
void dwb_entry_set_text(const char *text);

void dwb_set_proxy(GList *, WebSettings *);

void dwb_set_single_instance(GList *, WebSettings *);
void dwb_new_window(Arg *arg);

gboolean dwb_eval_editing_key(GdkEventKey *);
void dwb_parse_command_line(const char *);
GHashTable * dwb_get_default_settings(void);

char * dwb_get_host(const char *uri);
GList * dwb_get_host_blocked(GList *, char *host);

gboolean dwb_end(void);
Key dwb_str_to_key(char *);

GList * dwb_keymap_add(GList *, KeyValue );

void dwb_save_settings(void);
gboolean dwb_save_files(gboolean);
CompletionType dwb_eval_completion_type(void);

void dwb_append_navigation_with_argument(GList **, const char *, const char *);
void dwb_clean_load_end(GList *);
gboolean dwb_block_ad(GList *gl, const char *);
#endif
