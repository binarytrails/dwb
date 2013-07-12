/*
 * Copyright (c) 2013 Stefan Bolte <portix@gmx.net>
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

#ifndef __DWB_IPC_H__
#define __DWB_IPC_H__
#include <dwbremote.h>

void ipc_start(GtkWidget *);
void ipc_send_hook(char *hook, const char *format, ...);

#define IPC_SEND_HOOK(name, hook, ...); do { \
    if (dwb.state.ipc_hooks & IPC_HOOK_##hook) ipc_send_hook(name, __VA_ARGS__); \
    } while(0)
#endif
