/*
 * Copyright (c) 2010-2012 Stefan Bolte <portix@gmx.net>
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

#include "dwb.h"
#include "completion.h"
#include "util.h"
#include "view.h"
#include "session.h"
#include "soup.h"
#include "html.h"
#include "commands.h"
#include "local.h"
#include "entry.h"
#include "adblock.h"
#include "download.h"
#include "js.h"

static int inline dwb_floor(double x) { 
  return x >= 0 ? (int) x : (int) x - 1;
}
static int inline modulo(int x, int y) {
  return x - dwb_floor((double)x/y)  * y;
}
/* commands.h {{{*/
/* commands_simple_command(keyMap *km) {{{*/
DwbStatus 
commands_simple_command(KeyMap *km) {
  gboolean (*func)(void *, void *) = km->map->func;
  Arg *arg = &km->map->arg;
  arg->e = NULL;
  int ret;

  if (dwb.state.mode & AUTO_COMPLETE) {
    completion_clean_autocompletion();
  }

  ret = func(km, arg);
  switch (ret) {
    case STATUS_OK:
      if (km->map->hide == NEVER_SM) 
        dwb_set_normal_message(dwb.state.fview, false, "%s:", km->map->n.second);
      else if (km->map->hide == ALWAYS_SM) {
        CLEAR_COMMAND_TEXT();
        gtk_widget_hide(dwb.gui.entry);
      }
      break;
    case STATUS_ERROR: 
      dwb_set_error_message(dwb.state.fview, arg->e ? arg->e : km->map->error);
      break;
    case STATUS_END: 
      dwb_clean_key_buffer();
      return ret;
    default: break;
  }
  if (! km->map->arg.ro)
    km->map->arg.p = NULL;
  dwb_clean_key_buffer();
  return ret;
}/*}}}*/
static WebKitWebView * 
commands_get_webview_with_nummod() {
  if (dwb.state.nummod > 0 && dwb.state.nummod <= g_list_length(dwb.state.views)) 
    return WEBVIEW(g_list_nth(dwb.state.views, NUMMOD - 1));
  else 
    return CURRENT_WEBVIEW();
}
static GList * 
commands_get_view_with_nummod() {
  if (dwb.state.nummod > 0 && dwb.state.nummod <= g_list_length(dwb.state.views)) 
    return g_list_nth(dwb.state.views, NUMMOD - 1);
  return dwb.state.fview;
}

/* commands_add_view(KeyMap *, Arg *) {{{*/
DwbStatus 
commands_add_view(KeyMap *km, Arg *arg) {
  view_add(arg->p, false);
  if (arg->p == NULL)
    dwb_open_startpage(dwb.state.fview);
  return STATUS_OK;
}/*}}}*/

/* commands_set_setting {{{*/
DwbStatus 
commands_set_setting(KeyMap *km, Arg *arg) {
  dwb.state.mode = SETTINGS_MODE;
  entry_focus();
  return STATUS_OK;
}/*}}}*/

/* commands_set_key {{{*/
DwbStatus 
commands_set_key(KeyMap *km, Arg *arg) {
  dwb.state.mode = KEY_MODE;
  entry_focus();
  return STATUS_OK;
}/*}}}*/

/* commands_focus_input {{{*/
DwbStatus
commands_focus_input(KeyMap *km, Arg *a) {
  char *value;
  DwbStatus ret = STATUS_OK;

  if ( (value = js_call_as_function(MAIN_FRAME(), CURRENT_VIEW()->hint_object, "focusInput", NULL, &value)) ) {
    if (!g_strcmp0(value, "_dwb_no_input_")) 
      ret = STATUS_ERROR;
    g_free(value);
  }
  
  return ret;
}/*}}}*/

/* commands_add_search_field(KeyMap *km, Arg *) {{{*/
DwbStatus
commands_add_search_field(KeyMap *km, Arg *a) {
  char *value;
  if ( (value = js_call_as_function(MAIN_FRAME(), CURRENT_VIEW()->hint_object, "addSearchEngine", NULL, &value)) ) {
    if (!g_strcmp0(value, "_dwb_no_hints_")) {
      return STATUS_ERROR;
    }
  }
  dwb.state.mode = SEARCH_FIELD_MODE;
  dwb_set_normal_message(dwb.state.fview, false, "Keyword:");
  entry_focus();
  g_free(value);
  return STATUS_OK;

}/*}}}*/

DwbStatus 
commands_insert_mode(KeyMap *km, Arg *a) {
  return dwb_change_mode(INSERT_MODE);
}
DwbStatus 
commands_normal_mode(KeyMap *km, Arg *a) {
  dwb_change_mode(NORMAL_MODE, true);
  return STATUS_OK;
}

/* commands_toggle_property {{{*/
DwbStatus 
commands_toggle_property(KeyMap *km, Arg *a) {
  char *prop = a->p;
  gboolean value;
  WebKitWebSettings *settings = webkit_web_view_get_settings(CURRENT_WEBVIEW());
  g_object_get(settings, prop, &value, NULL);
  g_object_set(settings, prop, !value, NULL);
  dwb_set_normal_message(dwb.state.fview, true, "Property %s set to %s", prop, !value ? "true" : "false");
  return STATUS_OK;
}/*}}}*/

/* commands_toggle_proxy {{{*/
DwbStatus
commands_toggle_proxy(KeyMap *km, Arg *a) {
  WebSettings *s = g_hash_table_lookup(dwb.settings, "proxy");
  s->arg.b = !s->arg.b;

  dwb_set_proxy(NULL, s);

  return STATUS_OK;
}/*}}}*/

/*commands_find {{{*/
DwbStatus  
commands_find(KeyMap *km, Arg *arg) { 
  View *v = CURRENT_VIEW();
  dwb.state.mode = FIND_MODE;
  dwb.state.forward_search = arg->b;
  if (v->status->search_string) {
    g_free(v->status->search_string);
    v->status->search_string = NULL;
  }
  entry_focus();
  return STATUS_OK;
}/*}}}*/

DwbStatus  
commands_search(KeyMap *km, Arg *arg) { 
  if (!dwb_search(arg)) 
    return STATUS_ERROR;
  return STATUS_OK;
}

/* commands_show_hints {{{*/
DwbStatus
commands_show_hints(KeyMap *km, Arg *arg) {
  return dwb_show_hints(arg);
}/*}}}*/

DwbStatus 
commands_show(KeyMap *km, Arg *arg) {
  html_load(dwb.state.fview, arg->p);
  return STATUS_OK;
}

/* commands_allow_cookie {{{*/
DwbStatus
commands_allow_cookie(KeyMap *km, Arg *arg) {
  switch (arg->n) {
    case COOKIE_ALLOW_PERSISTENT: 
      return dwb_soup_allow_cookie(&dwb.fc.cookies_allow, dwb.files.cookies_allow, arg->n);
    case COOKIE_ALLOW_SESSION:
      return dwb_soup_allow_cookie(&dwb.fc.cookies_session_allow, dwb.files.cookies_session_allow, arg->n);
    case COOKIE_ALLOW_SESSION_TMP:
      dwb_soup_allow_cookie_tmp();
      break;
    default: 
      break;

  }
  return STATUS_OK;
}/*}}}*/

/* commands_bookmark {{{*/
DwbStatus 
commands_bookmark(KeyMap *km, Arg *arg) {
  gboolean noerror = STATUS_ERROR;
  if ( (noerror = dwb_prepend_navigation(dwb.state.fview, &dwb.fc.bookmarks)) == STATUS_OK) {
    util_file_add_navigation(dwb.files.bookmarks, dwb.fc.bookmarks->data, true, -1);
    dwb.fc.bookmarks = g_list_sort(dwb.fc.bookmarks, (GCompareFunc)util_navigation_compare_first);
    dwb_set_normal_message(dwb.state.fview, true, "Saved bookmark: %s", webkit_web_view_get_uri(CURRENT_WEBVIEW()));
  }
  return noerror;
}/*}}}*/

/* commands_quickmark(KeyMap *km, Arg *arg) {{{*/
DwbStatus
commands_quickmark(KeyMap *km, Arg *arg) {
  if (dwb.state.nv == OPEN_NORMAL)
    dwb_set_open_mode(arg->i);
  entry_focus();
  dwb.state.mode = arg->n;
  return STATUS_OK;
}/*}}}*/

/* commands_reload(KeyMap *km, Arg *){{{*/
DwbStatus
commands_reload(KeyMap *km, Arg *arg) {
  GList *gl = dwb.state.nummod > 0 && dwb.state.nummod <= g_list_length(dwb.state.views)
    ? g_list_nth(dwb.state.views, dwb.state.nummod-1) 
    : dwb.state.fview;
  dwb_reload(gl);
  return STATUS_OK;
}/*}}}*/

/* commands_reload_bypass_cache {{{*/
DwbStatus
commands_reload_bypass_cache(KeyMap *km, Arg *arg) {
  webkit_web_view_reload_bypass_cache(commands_get_webview_with_nummod());
  return STATUS_OK;
}
/*}}}*/

/* commands_stop_loading {{{*/
DwbStatus
commands_stop_loading(KeyMap *km, Arg *arg) {
  webkit_web_view_stop_loading(commands_get_webview_with_nummod());
  return STATUS_OK;
}
/*}}}*/

/* commands_view_source(Arg) {{{*/
DwbStatus
commands_view_source(KeyMap *km, Arg *arg) {
  WebKitWebView *web = CURRENT_WEBVIEW();
  webkit_web_view_set_view_source_mode(web, !webkit_web_view_get_view_source_mode(web));
  webkit_web_view_reload(web);
  return STATUS_OK;
}/*}}}*/

/* commands_zoom_in(void *arg) {{{*/
DwbStatus
commands_zoom_in(KeyMap *km, Arg *arg) {
  View *v = dwb.state.fview->data;
  WebKitWebView *web = WEBKIT_WEB_VIEW(v->web);

  for (int i=0; i<NUMMOD; i++) {
    if ((webkit_web_view_get_zoom_level(web) > 4.0)) {
      return STATUS_ERROR;
    }
    webkit_web_view_zoom_in(web);
  }
  return STATUS_OK;
}/*}}}*/

/* commands_zoom_out(void *arg) {{{*/
DwbStatus
commands_zoom_out(KeyMap *km, Arg *arg) {
  View *v = dwb.state.fview->data;
  WebKitWebView *web = WEBKIT_WEB_VIEW(v->web);

  for (int i=0; i<NUMMOD; i++) {
    if ((webkit_web_view_get_zoom_level(web) < 0.25)) {
      return STATUS_ERROR;
    }
    webkit_web_view_zoom_out(web);
  }
  return STATUS_OK;
}/*}}}*/

/* commands_scroll {{{*/
DwbStatus 
commands_scroll(KeyMap *km, Arg *arg) {
  GList *gl = arg->p ? arg->p : dwb.state.fview;
  dwb_scroll(gl, dwb.misc.scroll_step, arg->n);
  return STATUS_OK;
}/*}}}*/

/* commands_set_zoom_level(KeyMap *km, Arg *arg) {{{*/
DwbStatus 
commands_set_zoom_level(KeyMap *km, Arg *arg) {
  GList *gl = arg->p ? arg->p : dwb.state.fview;
  double zoomlevel = dwb.state.nummod < 0 ? arg->d : (double)dwb.state.nummod  / 100;
  webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(((View*)gl->data)->web), zoomlevel);
  return STATUS_OK;
}/*}}}*/

/* History {{{*/
DwbStatus 
commands_history(KeyMap *km, Arg *arg) {
  return dwb_history(arg);
}/*}}}*/

/* commands_open(KeyMap *km, Arg *arg) {{{*/
DwbStatus  
commands_open(KeyMap *km, Arg *arg) {
  if (dwb.state.nv & OPEN_NORMAL)
    dwb_set_open_mode(arg->n & ~SET_URL);

  if (arg)
    dwb.state.type = arg->i;

  if (arg && arg->p && ! (arg->n & SET_URL)) {
    dwb_load_uri(NULL, arg->p);
  }
  else {
    entry_focus();
    if (arg && (arg->n & SET_URL))
      entry_set_text(arg->p ? arg->p : CURRENT_URL());
  }
  return STATUS_OK;
} /*}}}*/

/* commands_open(KeyMap *km, Arg *arg) {{{*/
DwbStatus  
commands_open_startpage(KeyMap *km, Arg *arg) {
  return dwb_open_startpage(NULL);
} /*}}}*/

/* commands_remove_view(KeyMap *km, Arg *arg) {{{*/
DwbStatus 
commands_remove_view(KeyMap *km, Arg *arg) {
  view_remove(NULL);
  return STATUS_OK;
}/*}}}*/

static gboolean
commands_hide_tabbar(int *running) {
  if (! (dwb.state.bar_visible & BAR_VIS_TOP)) {
    gtk_widget_hide(dwb.gui.topbox);
  }
  *running = 0;
  return false;
}

/* commands_focus(KeyMap *km, Arg *arg) {{{*/
DwbStatus 
commands_focus(KeyMap *km, Arg *arg) {
  static int running;
  if (dwb.state.views->next) {
    int pos = modulo(g_list_position(dwb.state.views, dwb.state.fview) + NUMMOD * arg->n, g_list_length(dwb.state.views));
    GList *g = g_list_nth(dwb.state.views, pos);
    dwb_focus_view(g);
    if (! (dwb.state.bar_visible & BAR_VIS_TOP) && dwb.misc.tabbar_delay > 0) {
      gtk_widget_show(dwb.gui.topbox);
      if (running != 0) 
        g_source_remove(running);
      running = g_timeout_add(dwb.misc.tabbar_delay * 1000, (GSourceFunc)commands_hide_tabbar, &running);
    }
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* commands_focus_view{{{*/
DwbStatus
commands_focus_nth_view(KeyMap *km, Arg *arg) {
  static int running;
  if (!dwb.state.views->next) 
    return STATUS_ERROR;
  GList *l = g_list_nth(dwb.state.views, dwb.state.nummod - 1);
  if (!l) 
    return STATUS_ERROR;
  dwb_focus_view(l);
  if (! (dwb.state.bar_visible & BAR_VIS_TOP)) {
    gtk_widget_show(dwb.gui.topbox);
    if (running != 0) 
      g_source_remove(running);
    running = g_timeout_add(2000, (GSourceFunc)commands_hide_tabbar, &running);
  }
  return STATUS_OK;
}/*}}}*/

/* commands_yank {{{*/
DwbStatus
commands_yank(KeyMap *km, Arg *arg) {
  GdkAtom atom = GDK_POINTER_TO_ATOM(arg->p);
  const char *text = NULL;
  WebKitWebView *wv = commands_get_webview_with_nummod();
  if (arg->n == CA_URI) 
    text = webkit_web_view_get_uri(wv);
  else if (arg->n == CA_TITLE)
    text = webkit_web_view_get_title(wv);

  return dwb_set_clipboard(text, atom);
}/*}}}*/

/* commands_paste {{{*/
DwbStatus
commands_paste(KeyMap *km, Arg *arg) {
  GdkAtom atom = GDK_POINTER_TO_ATOM(arg->p);
  char *text = NULL;

  if ( (text = dwb_clipboard_get_text(atom)) ) {
    if (dwb.state.nv == OPEN_NORMAL)
      dwb_set_open_mode(arg->n);
    dwb_load_uri(NULL, text);
    g_free(text);
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* dwb_entry_movement() {{{*/
DwbStatus
commands_entry_movement(KeyMap *m, Arg *a) {
  entry_move_cursor_step(a->n, a->i, a->b);
  return STATUS_OK;
}/*}}}*/

/* commands_entry_history_forward {{{*/
DwbStatus 
commands_entry_history_forward(KeyMap *km, Arg *a) {
  if (dwb.state.mode == COMMAND_MODE) 
    return entry_history_forward(&dwb.state.last_com_history);
  else 
    return entry_history_forward(&dwb.state.last_nav_history);
}/*}}}*/

/* commands_entry_history_back {{{*/
DwbStatus 
commands_entry_history_back(KeyMap *km, Arg *a) {
  if (dwb.state.mode == COMMAND_MODE) 
    return entry_history_back(&dwb.fc.commands, &dwb.state.last_com_history);
  else 
    return entry_history_back(&dwb.fc.navigations, &dwb.state.last_nav_history);
}/*}}}*/

/* commands_entry_confirm {{{*/
DwbStatus 
commands_entry_confirm(KeyMap *km, Arg *a) {
  GdkEventKey e = { .state = 0, .keyval = GDK_KEY_Return };
  gboolean ret;
  g_signal_emit_by_name(dwb.gui.entry, "key-press-event", &e, &ret);
  return STATUS_OK;
}/*}}}*/
/* commands_entry_escape {{{*/
DwbStatus 
commands_entry_escape(KeyMap *km, Arg *a) {
  dwb_change_mode(NORMAL_MODE, true);
  return STATUS_OK;
}/*}}}*/

/* commands_save_session {{{*/
DwbStatus
commands_save_session(KeyMap *km, Arg *arg) {
  if (arg->n == NORMAL_MODE) {
    dwb.state.mode = SAVE_SESSION;
    session_save(NULL, true);
    dwb_end();
    return STATUS_END;
  }
  else {
    dwb.state.mode = arg->n;
    entry_focus();
    dwb_set_normal_message(dwb.state.fview, false, "Session name:");
  }
  return STATUS_OK;
}/*}}}*/

/* commands_bookmarks {{{*/
DwbStatus
commands_bookmarks(KeyMap *km, Arg *arg) {
  if (!g_list_length(dwb.fc.bookmarks)) {
    return STATUS_ERROR;
  }
  if (dwb.state.nv == OPEN_NORMAL)
    dwb_set_open_mode(arg->n);
  completion_complete(COMP_BOOKMARK, 0);
  entry_focus();
  if (dwb.comps.active_comp != NULL) {
    completion_set_entry_text((Completion*)dwb.comps.active_comp->data);
  }
  return STATUS_OK;
}/*}}}*/

/* commands_history{{{*/
DwbStatus  
commands_complete_type(KeyMap *km, Arg *arg) {
  return completion_complete(arg->n, 0);
}/*}}}*/

void
commands_toggle(Arg *arg, const char *filename, GList **tmp, const char *message) {
  char *host = NULL;
  const char *block = NULL;
  gboolean allowed;
  if (arg->n & ALLOW_HOST) {
    host = dwb_get_host(CURRENT_WEBVIEW());
    block = host;
  }
  else if (arg->n & ALLOW_URI) {
    block = webkit_web_view_get_uri(CURRENT_WEBVIEW());
  }
  else if (arg->p) {
    block = arg->p;
  }
  if (block != NULL) {
    if (arg->n & ALLOW_TMP) {
      GList *l;
      if ( (l = g_list_find_custom(*tmp, block, (GCompareFunc)g_strcmp0)) ) {
        g_free(l->data);
        *tmp = g_list_delete_link(*tmp, l);
        allowed = false;
      }
      else {
        *tmp = g_list_prepend(*tmp, g_strdup(block));
        allowed = true;
      }
      dwb_set_normal_message(dwb.state.fview, true, "%s temporarily %s for %s", message, allowed ? "allowed" : "blocked", block);
    }
    else {
      allowed = dwb_toggle_allowed(filename, block);
      dwb_set_normal_message(dwb.state.fview, true, "%s %s for %s", message, allowed ? "allowed" : "blocked", block);
    }
  }
  else 
    CLEAR_COMMAND_TEXT();
}

DwbStatus 
commands_toggle_plugin_blocker(KeyMap *km, Arg *arg) {
  commands_toggle(arg, dwb.files.plugins_allow, &dwb.fc.tmp_plugins, "Plugins");
  return STATUS_OK;
}

/* commands_toggle_scripts {{{ */
DwbStatus 
commands_toggle_scripts(KeyMap *km, Arg *arg) {
  commands_toggle(arg, dwb.files.scripts_allow, &dwb.fc.tmp_scripts, "Scripts");
  return STATUS_OK;
}/*}}}*/

/* commands_new_window_or_view{{{*/
DwbStatus 
commands_new_window_or_view(KeyMap *km, Arg *arg) {
  dwb_set_open_mode(arg->n | OPEN_EXPLICIT);
  return STATUS_OK;
}/*}}}*/

/* commands_save_files(KeyMap *km, Arg *arg) {{{*/
DwbStatus 
commands_save_files(KeyMap *km, Arg *arg) {
  if (dwb_save_files(false)) {
    dwb_set_normal_message(dwb.state.fview, true, "Configuration files saved");
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* commands_undo() {{{*/
DwbStatus
commands_undo(KeyMap *km, Arg *arg) {
  if (dwb.state.undo_list) {
    WebKitWebView *web = WEBVIEW(view_add(NULL, false));
    WebKitWebBackForwardList *bflist = webkit_web_back_forward_list_new_with_web_view(web);
    for (GList *l = dwb.state.undo_list->data; l; l=l->next) {
      Navigation *n = l->data;
      WebKitWebHistoryItem *item = webkit_web_history_item_new_with_data(n->first, n->second);
      webkit_web_back_forward_list_add_item(bflist, item);
      if (!l->next) {
        webkit_web_view_go_to_back_forward_item(web, item);
      }
      dwb_navigation_free(n);
    }
    g_list_free(dwb.state.undo_list->data);
    dwb.state.undo_list = g_list_delete_link(dwb.state.undo_list, dwb.state.undo_list);
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* commands_print (Arg *) {{{*/
DwbStatus
commands_print(KeyMap *km, Arg *arg) {
  WebKitWebView *wv = commands_get_webview_with_nummod();
  WebKitWebFrame *frame = webkit_web_view_get_focused_frame(wv);
  if (frame) {
    webkit_web_frame_print(frame);
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* commands_web_inspector {{{*/
DwbStatus
commands_web_inspector(KeyMap *km, Arg *arg) {
  if (GET_BOOL("enable-developer-extras")) {
    WebKitWebView *wv = commands_get_webview_with_nummod();
    webkit_web_inspector_show(webkit_web_view_get_inspector(wv));
    return STATUS_OK;
  }
  return STATUS_ERROR;
}/*}}}*/

/* commands_execute_userscript (Arg *) {{{*/
DwbStatus
commands_execute_userscript(KeyMap *km, Arg *arg) {
  if (!dwb.misc.userscripts) 
    return STATUS_ERROR;

  if (arg->p) {
    char *path = g_build_filename(dwb.files.userscripts, arg->p, NULL);
    Arg a = { .arg = path };
    dwb_execute_user_script(km, &a);
    g_free(path);
  }
  else {
    entry_focus();
    completion_complete(COMP_USERSCRIPT, 0);
  }

  return STATUS_OK;
}/*}}}*/

/* commands_toggle_hidden_files {{{*/
DwbStatus
commands_toggle_hidden_files(KeyMap *km, Arg *arg) {
  dwb.state.hidden_files = !dwb.state.hidden_files;
  dwb_reload(dwb.state.fview);
  return STATUS_OK;
}/*}}}*/

/* commands_quit {{{*/
DwbStatus
commands_quit(KeyMap *km, Arg *arg) {
  dwb_end();
  return STATUS_END;
}/*}}}*/

/* commands_reload_scripts {{{*/
DwbStatus
commands_reload_scripts(KeyMap *km, Arg *arg) {
  dwb_init_scripts();
  for (GList *l = dwb.state.views; l; l=l->next) {
    webkit_web_view_reload(WEBVIEW(l));
  }
  return STATUS_OK;
}/*}}}*/
DwbStatus
commands_reload_user_scripts(KeyMap *km, Arg *arg) {
  dwb_reload_userscripts();
  return STATUS_OK;
}

/* commands_reload_scripts {{{*/
DwbStatus
commands_fullscreen(KeyMap *km, Arg *arg) {
  if (dwb.state.bar_visible & BAR_PRESENTATION)
    return STATUS_ERROR;
  dwb.state.fullscreen = !dwb.state.fullscreen;
  if (dwb.state.fullscreen) 
    gtk_window_fullscreen(GTK_WINDOW(dwb.gui.window));
  else 
    gtk_window_unfullscreen(GTK_WINDOW(dwb.gui.window));
  return STATUS_OK;
}/*}}}*/
/* commands_reload_scripts {{{*/
DwbStatus
commands_open_editor(KeyMap *km, Arg *arg) {
  return dwb_open_in_editor();
}/*}}}*/

/* dwb_command_mode {{{*/
DwbStatus
commands_command_mode(KeyMap *km, Arg *arg) {
  return dwb_change_mode(COMMAND_MODE);
}/*}}}*/
/* dwb_command_mode {{{*/
DwbStatus
commands_only(KeyMap *km, Arg *arg) {
  DwbStatus ret = STATUS_ERROR;
  GList *l = dwb.state.views, *next;
  while (l) {
    next = l->next;
    if (l != dwb.state.fview) {
      view_remove(l);
      ret = STATUS_OK;
    }
    l = next;
  }
  return ret;
}/*}}}*/
static void 
commands_set_bars(int status) {
  gtk_widget_set_visible(dwb.gui.topbox, status & BAR_VIS_TOP);
  gtk_widget_set_visible(dwb.gui.bottombox, status & BAR_VIS_STATUS);
  if ((status & BAR_VIS_STATUS) ) {
    dwb_dom_remove_from_parent(WEBKIT_DOM_NODE(CURRENT_VIEW()->hover.element), NULL);
  }
}
/* commands_toggle_bars {{{*/
DwbStatus
commands_toggle_bars(KeyMap *km, Arg *arg) {
  dwb.state.bar_visible ^= arg->n;
  commands_set_bars(dwb.state.bar_visible);
  return STATUS_OK;
}/*}}}*/
/* commands_presentation_mode {{{*/
DwbStatus
commands_presentation_mode(KeyMap *km, Arg *arg) {
  static int last;
  if (dwb.state.bar_visible & BAR_PRESENTATION) {
    if (! dwb.state.fullscreen)
      gtk_window_unfullscreen(GTK_WINDOW(dwb.gui.window));
    commands_set_bars(last);
    dwb.state.bar_visible = last & ~BAR_PRESENTATION;
  }
  else {
    dwb.state.bar_visible |= BAR_PRESENTATION;
    if (! dwb.state.fullscreen)
      gtk_window_fullscreen(GTK_WINDOW(dwb.gui.window));
    commands_set_bars(0);
    last = dwb.state.bar_visible;
    dwb.state.bar_visible = BAR_PRESENTATION;
  }
  return STATUS_OK;
}/*}}}*/
/* commands_toggle_lock_protect {{{*/
DwbStatus
commands_toggle_lock_protect(KeyMap *km, Arg *arg) {
  GList *gl = dwb.state.nummod < 0 ? dwb.state.fview : g_list_nth(dwb.state.views, dwb.state.nummod-1);
  if (gl == NULL)
    return STATUS_ERROR;
  View *v = VIEW(gl);
  v->status->lockprotect ^= arg->n;
  dwb_update_status(gl);
  if (arg->n & LP_VISIBLE && gl != dwb.state.fview)
    gtk_widget_set_visible(v->scroll, LP_VISIBLE(v));
  return STATUS_OK;
}/*}}}*/
/* commands_execute_javascript {{{*/
DwbStatus
commands_execute_javascript(KeyMap *km, Arg *arg) {
  static char *script;
  if (arg->p == NULL && script == NULL)
    return STATUS_ERROR;
  if (arg->p) {
    FREE0(script);
    script = g_strdup(arg->p);
  }
  WebKitWebView *wv = commands_get_webview_with_nummod();
  dwb_execute_script(webkit_web_view_get_focused_frame(wv), script, false);
  return STATUS_OK;
}/*}}}*/
/* commands_set {{{*/
DwbStatus
commands_set(KeyMap *km, Arg *arg) {
  const char *command = util_str_chug(arg->p);
  char **args = g_strsplit(command, " ", 2);
  DwbStatus ret = dwb_set_setting(args[0], args[1]);
  g_strfreev(args);
  return ret;
}/*}}}*/
/* commands_set {{{*/
DwbStatus
commands_toggle_setting(KeyMap *km, Arg *arg) {
  const char *command = util_str_chug(arg->p);
  return dwb_toggle_setting(command);
}/*}}}*/
DwbStatus
commands_tab_move(KeyMap *km, Arg *arg) {
  int l = g_list_length(dwb.state.views);
  int newpos;
  if (dwb.state.views->next == NULL) 
    return STATUS_ERROR;
  switch (arg->n) {
    case TAB_MOVE_LEFT   : newpos = MAX(MIN(l-1, g_list_position(dwb.state.views, dwb.state.fview)-NUMMOD), 0); break;
    case TAB_MOVE_RIGHT  : newpos = MAX(MIN(l-1, g_list_position(dwb.state.views, dwb.state.fview)+NUMMOD), 0); break;
    default :  newpos = MAX(MIN(l, NUMMOD)-1, 0); break;
  }
  gtk_box_reorder_child(GTK_BOX(dwb.gui.topbox), CURRENT_VIEW()->tabevent, l-(newpos+1));
  gtk_box_reorder_child(GTK_BOX(dwb.gui.mainbox), CURRENT_VIEW()->scroll, newpos);
  dwb.state.views = g_list_remove_link(dwb.state.views, dwb.state.fview);
  GList *sibling = g_list_nth(dwb.state.views, newpos);
  g_list_position(dwb.state.fview, sibling);
  if (sibling == NULL) {
    GList *last = g_list_last(dwb.state.views);
    last->next = dwb.state.fview;
    dwb.state.fview->prev = last;
  }
  else if (sibling->prev == NULL) {
    dwb.state.views->prev = dwb.state.fview;
    dwb.state.fview->next = dwb.state.views;
    dwb.state.views = dwb.state.fview;
  }
  else {
    sibling->prev->next = dwb.state.fview;
    dwb.state.fview->prev = sibling->prev;
    sibling->prev = dwb.state.fview;
    dwb.state.fview->next = sibling;
  }
  dwb_focus(dwb.state.fview);
  dwb_update_layout();
  return STATUS_OK;
}
DwbStatus 
commands_clear_tab(KeyMap *km, Arg *arg) {
  GList *gl = commands_get_view_with_nummod();
  dwb_load_uri(gl, "about:blank");
  WebKitWebBackForwardList *bf_list = webkit_web_view_get_back_forward_list(WEBVIEW(gl));
  webkit_web_back_forward_list_clear(bf_list);
  return STATUS_OK;
}
DwbStatus 
commands_cancel_download(KeyMap *km, Arg *arg) {
  return download_cancel(dwb.state.nummod);
}
/*}}}*/
