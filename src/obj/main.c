// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
   NonoWIMP: RISC OS WIMP program to solve Nonograms
   Copyright (C) 2000-7,2012  Steven Simpson

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   Author contact: <https://github.com/simpsonst>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __TARGET_UNIXLIB__
#define __UNIXLIB_INTERNALS 1
#include <unixlib/local.h>
int __riscosify_control = __RISCOSIFY_NO_PROCESS;
#endif

#include <swis.h>

#include <riscos/swi/URI.h>
#include <riscos/types.h>

#include <yacitros/desktop/task.h>
#include <yacitros/desktop/event.h>
#include <yacitros/desktop/template.h>
#include <yacitros/desktop/win.h>
#include <yacitros/desktop/iconbar.h>
#include <yacitros/desktop/menu.h>
#include <yacitros/desktop/label.h>

#include "nonowin.h"

typedef struct {
  int quit;
  wimp_iconhandle ih;
  menu_handle menu;
  win_menuat matt;
  win_handle infowin;
  void *infowin_work;
  label_bundle labels;
} program_data;

#if 0
void autoquit(program_data *pd);
const char *baricon_help(program_data *,
                         const mouse_state *m, wimp_iconhandle ih);
int baricon_save(program_data *r, wimp_iconhandle ih, const os_point *pos,
                 signed long sz, unsigned long type, char *name, int len);
int baricon_load(program_data *r, wimp_iconhandle ih, const os_point *pos,
                 signed long sz, unsigned long type, const char *name);
void baricon_menusel(program_data *h, const wimp_menuchoice *c);
void baricon_menuop(program_data *h, const wimp_menuchoice *c,
                    const os_point *);
menu_handle baricon_menugen(program_data *, wimp_iconhandle, os_point *);
const char *baricon_menuhelp(program_data *, const wimp_menuchoice *);

event_rspcode infowin_open(program_data *, const wimp_winloc *);
event_rspcode infowin_close(program_data *);
const char *infowin_help(program_data *,
                         const mouse_state *, wimp_iconhandle);
#endif

const char *baricon_help(void *vr, const mouse_state *m, wimp_iconhandle ih)
{
  program_data *r = vr;
  return label_default(&r->labels, "ih_ibar", "\\TNonogram Solver.|M"
                       "\\Sopen an empty Nonogram \\w.|M"
                       "\\Dopen a puzzle \\w.");
}

int baricon_load(void *vr, wimp_iconhandle ih, const vdu_point *pos,
                 signed long sz, file_type type, const char *name)
{
  nonowin nh;

  nh = nonowin_create();
  if (!nh)
    return false;

  if (nonowin_loadpuzzlefile(nh, type, name))
    return true;

  nonowin_destroy(nh);
  return false;
}

int baricon_save(void *vr, wimp_iconhandle ih, const vdu_point *pos,
                 signed long sz, file_type type, char *name, int len)
{
  strncpy(name, "<Wimp$Scrap>", len);
  return false; /* unsafe */
}

void baricon_menusel(void *vh, const wimp_menuchoice *c)
{
  program_data *h = vh;
  switch (c[0]) {
  case 0:
#if 0
    win_openmenu(h->infowin, menu_x() - 256, menu_y() + 128);
#endif
    break;
  case 1:
    nonowin_create();
    break;
  case 2:
    h->quit = true;
    break;
  }
}

void baricon_menuop(void *vh, const wimp_menuchoice *c,
                    const vdu_point *pos)
{
  program_data *h = vh;
  switch (c[0]) {
  case 0:
    if (win_opensubmenu(h->infowin, pos->x, pos->y) < 0)
      task_complain(task_err());
    break;
  }
}

menu_handle baricon_menugen(void *vh, wimp_iconhandle ih, vdu_point *pos)
{
  program_data *h = vh;
  if (pos) {
    pos->x -= 64;
    pos->y = 96 + menu_height(h->menu);
  }
  return h->menu;
}

const char *baricon_menuhelp(void *vh, const wimp_menuchoice *c)
{
  program_data *h = vh;
  switch (c[0]) {
  case 0:
    return label_default(&h->labels, "mh_ibar_info",
                         "\\Rget information about "
                         "this version of the Nonogram Solver.");
  case 1:
    return label_default(&h->labels, "mh_ibar_new",
                         "\\Sopen a new Nonogram window.");
  case 2:
    return label_default(&h->labels, "mh_ibar_quit",
                         "\\Squit the Nonogram Solver.");
  }
  return NULL;
}

void autoquit(void *vpd)
{
  program_data *pd = vpd;
  pd->quit = true;
}

event_rspcode infowin_open(void *vr, const wimp_winloc *loc)
{
  program_data *r = vr;
  if (win_openat(r->infowin, loc) < 0)
    task_complain(task_err());
  return event_CLAIM;
}

event_rspcode infowin_close(void *vr)
{
  program_data *r = vr;
  win_close(r->infowin);
  return event_CLAIM;
}

const char *infowin_help(void *vr, const mouse_state *ms, wimp_iconhandle i)
{
  program_data *r = vr;
  if (i == 8)
    return label_default(&r->labels, "ih_info_website",
                         "\\Svisit the website.");

  return label_default(&r->labels, "wh_info",
                       "This \\w displays information "
                       "about Nonogram Solver.");
}

#define DEFAULT_WEBSITE \
"http://www.comp.lancs.ac.uk/~ss/software/nonowimp/"

event_rspcode infowin_click(void *vr, const mouse_state *ms, wimp_iconhandle i)
{
  program_data *r = vr;

  if (i == 8) {
    const char *location =
      label_default(&r->labels, "str_website", DEFAULT_WEBSITE);
    task_complain(_swix(URI_Dispatch, _INR(0,2), 0, location, 0));
    return event_CLAIM;
  }

  return event_SHIRK;
}

int main(void)
{
  program_data app;
  template_handle tmp;
  if (label_open(&app.labels, "<Nonogram$Dir>.Messages") < 0) {
    fprintf(stderr, "Failed to open messages\n");
    if (task_err())
      fprintf(stderr, "%s\n", task_err()->errmess);
    exit(EXIT_FAILURE);
  }


  app.quit = false;
  if (!task_init(label_default(&app.labels,
                               "tn_nonogram", "Nonogram Solver"),
                 &app.quit)) {
    template_init("<Nonogram$Dir>.Templates");
    nonowin_init(&app.labels);

    /* initialise iconbar */
    iconbar_init(&app);
    win_conf_help(iconbar, &baricon_help);
    win_conf_load(iconbar, &baricon_load);
    win_conf_save(iconbar, &baricon_save);
    app.ih = iconbar_appicon("!Nonogram", sprite_WIMPAREA);

    /* initialise Info window */
    tmp = template_load("info", 0, 12, 1000);
    app.infowin = win_create(tmp, &app);
    app.infowin_work = template_release(tmp);
    win_conf_open(app.infowin, &infowin_open);
    win_conf_close(app.infowin, &infowin_close);
    win_conf_help(app.infowin, &infowin_help);
    win_conf_click(app.infowin, &infowin_click);
    {
      extern const char *const linkversion;
      extern const char *const linkdate;
#if 0
      extern const char *const linktime;
#endif
      char str[200];
      sprintf(str, "%s (%s)", linkversion, linkdate);
      win_setfield(app.infowin, 3, str);
    }

    /* create icon bar menu */
    app.menu = menu_create(label_default(&app.labels, "mt_ibar", "Nonogram"),
                           label_default(&app.labels, "mi_ibar",
                                         ">Info,New|Quit"));

    app.matt = win_attachmenu(iconbar, &baricon_menugen, &baricon_menusel,
                               &baricon_menuop, &baricon_menuhelp, &app);

    /* event loop */
    while (!app.quit) {
      if (event_process() < 0) {
        task_complain(task_err());
        break;
      }
    }

    // Fix bug when remotely operating application by sending a
    // Message_Quit while the menu and info window happen to be open.
    menu_close();

    /* close down */
    win_destroy(app.infowin), free(app.infowin_work);
    win_detachmenu(app.matt);
    menu_destroy(app.menu);
    iconbar_term();
    template_term();
    task_term();
    label_close(&app.labels);
  }

  return 0;
}
