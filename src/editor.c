/*
 * Copyright (c) 2010-2014 Stefan Bolte <portix@gmx.net>
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

#include <string.h>
#include <pthread.h>
#include "dwb.h"
#include "editor.h"
#include "util.h"
#include "dom.h"

typedef struct _EditorInfo {
    char *filename;
    char *id;
    GList *gl;
    WebKitDOMElement *element;
    char *tagname;
    gboolean contenteditable;
} EditorInfo;

pthread_mutex_t s_disconnect_mutex = PTHREAD_MUTEX_INITIALIZER;

static void
disconnect_navigation(GList *gl)
{
    pthread_mutex_lock(&s_disconnect_mutex);
    if (VIEW(gl)->status->signals[SIG_EDITOR_NAVIGATION] != 0)
    {
        g_signal_handler_disconnect(VIEW(gl)->web, VIEW(gl)->status->signals[SIG_EDITOR_NAVIGATION]);
        VIEW(gl)->status->signals[SIG_EDITOR_NAVIGATION] = 0;
    }
    pthread_mutex_unlock(&s_disconnect_mutex);
}

/* dwb_editor_watch (GChildWatchFunc) {{{*/
static void
editor_watch(GPid pid, int status, EditorInfo *info) 
{
    gsize length;
    WebKitDOMElement *e = NULL;
    WebKitWebView *wv;

    char *content = util_get_file_content(info->filename, &length);

    if (content == NULL) 
        return;

    if (!info->gl || !g_list_find(dwb.state.views, info->gl->data)) 
    {
        if (info->id == NULL) 
            goto clean;
        else 
            wv = CURRENT_WEBVIEW();
    }
    else 
        wv = WEBVIEW(info->gl);
    if (info->id != NULL) 
    {
        WebKitDOMDocument *doc = webkit_web_view_get_dom_document(wv);
        e = webkit_dom_document_get_element_by_id(doc, info->id);

        if (e == NULL && (e = info->element) == NULL ) 
            goto clean;
    }
    else if (info->element)
        e = info->element;
    else 
        goto clean;

    /*  g_file_get_contents adds an additional newline */
    if (length > 0 && content[length-1] == '\n')
        content[length-1] = 0;

    if (!strcasecmp(info->tagname, "INPUT")) 
        webkit_dom_html_input_element_set_value(WEBKIT_DOM_HTML_INPUT_ELEMENT(e), content);
    else if (!strcasecmp(info->tagname, "TEXTAREA")) 
        webkit_dom_html_text_area_element_set_value(WEBKIT_DOM_HTML_TEXT_AREA_ELEMENT(e), content);
    else if (info->contenteditable)
        webkit_dom_html_element_set_inner_text(WEBKIT_DOM_HTML_ELEMENT(e), content, NULL);

clean:
    if (info->gl)
        disconnect_navigation(info->gl);

    unlink(info->filename);
    g_free(info->filename);
    g_free(info->id);
    g_free(info);
}/*}}}*/


static gboolean 
navigation_cb(WebKitWebView *web, WebKitWebFrame *frame, WebKitNetworkRequest *request, WebKitWebNavigationAction *action,
    WebKitWebPolicyDecision *policy, EditorInfo *info) 
{
    if (frame == webkit_web_view_get_main_frame(web))
    {
        info->element = NULL;
        g_free(info->id);
        info->id = NULL;
        disconnect_navigation(info->gl);
    }
    return false;
}

/* dwb_open_in_editor(void) ret: gboolean success {{{*/
DwbStatus
editor_open(void) 
{
    DwbStatus ret = STATUS_ERROR;
    char **commands = NULL;
    char *commandstring = NULL, *tagname, *path;
    char *value = NULL;
    GPid pid;
    gboolean success;
    gboolean contenteditable = false;

    char *editor = GET_CHAR("editor");

    if (editor == NULL) 
        return STATUS_ERROR;

    WebKitDOMDocument *doc = webkit_web_view_get_dom_document(CURRENT_WEBVIEW());
    WebKitDOMElement *active = dom_get_active_element(doc);

    if (active == NULL) 
        return STATUS_ERROR;

    tagname = webkit_dom_element_get_tag_name(active);
    if (tagname == NULL) 
    {
        goto clean;
    }
    if (! strcasecmp(tagname, "INPUT")) 
        value = webkit_dom_html_input_element_get_value(WEBKIT_DOM_HTML_INPUT_ELEMENT(active));
    else if (! strcasecmp(tagname, "TEXTAREA"))
        value = webkit_dom_html_text_area_element_get_value(WEBKIT_DOM_HTML_TEXT_AREA_ELEMENT(active));
    else if (webkit_dom_element_has_attribute(active, "contenteditable")) 
    {
        value = webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(active));
        contenteditable = true;
    }

    if (value == NULL) 
    {
        goto clean;
    }

    path = util_get_temp_filename("edit");

    commandstring = util_string_replace(editor, "dwb_uri", path);
    if (commandstring == NULL)  
    {
        goto clean;
    }

    g_file_set_contents(path, value, -1, NULL);
    commands = g_strsplit(commandstring, " ", -1);
    g_free(commandstring);

    success = g_spawn_async(NULL, commands, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
    g_strfreev(commands);
    if (!success) 
    {
        goto clean;
    }

    EditorInfo *info = dwb_malloc(sizeof(EditorInfo));
    char *id = webkit_dom_element_get_id(active);
    if (id != NULL && strlen(id) > 0) {
        info->id = id;
    }
    else  {
        info->id = NULL;
    }
    info->tagname = tagname;
    info->element = active;
    info->filename = path;
    info->gl = dwb.state.fview;
    info->contenteditable = contenteditable;
    g_child_watch_add(pid, (GChildWatchFunc)editor_watch, info);
    VIEW(dwb.state.fview)->status->signals[SIG_EDITOR_NAVIGATION] = 
        g_signal_connect(CURRENT_WEBVIEW(), "navigation-policy-decision-requested", G_CALLBACK(navigation_cb), info);
    dwb_set_normal_message(dwb.state.fview, true, "Spawning editor '%s'", editor);

    ret = STATUS_OK;
clean:
    g_free(value);

    return ret;
}/*}}}*/
