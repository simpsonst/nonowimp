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

#ifndef NONOINT_H
#define NONOINT_H

#include <string.h>
#include <stdlib.h>

#include <riscos/types.h>

#include <yacitros/desktop/types.h>

#include "nonowin.h"

int nonowin_load(void *, wimp_iconhandle ih, const vdu_point *pos,
                 long sz, file_type, const char *name);
void nonowin_inval(nonowin w);
int nonowin_savegrid_draw(nonowin w, FILE *fp);
void nonowin_updatetitle(nonowin w);
void nonowin_updatestatus(nonowin w);
const char *nonowin_help(void *, const mouse_state *, wimp_iconhandle);
void nonowin_updatesolutions(nonowin w);
event_rspcode nonowin_openinfo(void *, const wimp_winloc *loc);
event_rspcode nonowin_closeinfo(void *);
void nonowin_resetsave(nonowin_savectxt *c);
event_rspcode nonowin_keysave(void *vc, const wimp_caretpos *cb,
                              wimp_iconhandle ih, wimp_keycode k);
event_rspcode nonowin_opensave(void *, const wimp_winloc *loc);
event_rspcode nonowin_closesave(void *);
int nonowin_saveokay(nonowin_savectxt *c, const char *newname);
int nonowin_deposit(void *c, const char *n, int safe);
void nonowin_savedrag(void *c, const vdu_box *a);
event_rspcode nonowin_clicksave(void *, const mouse_state *, wimp_iconhandle);
const char *nonowin_savehelp(void *, const mouse_state *, wimp_iconhandle);
void nonowin_menusel(void *, const wimp_menuchoice *);
void nonowin_menuop(void *, const wimp_menuchoice *, const vdu_point *);
menu_handle nonowin_menugen(void *, wimp_iconhandle ih, vdu_point *pos);

void nonowin_okay(nonowin w, int);
void nonowin_resize(nonowin w);
void nonowin_unload(nonowin w);
void nonowin_found(void *);
const char *nonowin_menuhelp(void *, const wimp_menuchoice *);

struct nonowin_info {
  nonowin cur;
  win_handle wh;
  void *data;
};

struct nonowin_save {
  nonowin cur;
  win_handle wh;
  void *data;
  transmit_record trc;
  enum { TEXTFILE, DRAWFILE } type;
  unsigned keep : 1, safe : 1, top : 1, left : 1,
    right : 1, bottom : 1, cells : 1;
};

#define MAX_LEVEL 10

struct nonowin_win {
  win_handle wh, vwork, hwork, okay;
  void *wh_d, *vwork_d, *hwork_d, *okay_d;
  event_idlerref ph, inval;
  win_menuat ma;
  vdu_sfact scale;
  nonogram_cell *grid;
  nonogram_puzzle puzzle;
  nonogram_solver solver;
  int algo;
  vdu_palentry statecol[MAX_LEVEL + 1];
  int solutions;
  char filename[300], drawname[300], textname[300];
  unsigned data_in : 1, haltnext : 1, counting : 1, finished : 1,
    fresh : 1, correct : 1;
};

extern nonowin_infoctxt nonowin_infobox;
extern nonowin_savectxt nonowin_savebox;
extern struct nonowin_menus {
  menu_handle root, save, action, algo;
} nonowin_menu;
extern template_handle nonowin_tpl, nonowin_vwkt, nonowin_okt;

extern const struct nonogram_display nonowin_display;
extern const struct nonogram_client nonowin_client;
extern label_bundle *nonowin_label;

void nonowin_setstatecols(vdu_palentry *vpe, nonogram_level max);

#endif
