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

#include "nonoint.h"

#include <yacitros/desktop/task.h>
#include <yacitros/desktop/win.h>
#include <yacitros/desktop/label.h>

void nonowin_menusel(void *vw, const wimp_menuchoice *c)
{
  nonowin w = vw;
  int newalgo;
  switch (c[0]) {
#if 0
  case 0:
    nonowin_infobox.cur = w;
    nonowin_updatestatus(w);
    nonowin_updatesolutions(w);
    nonowin_updatetitle(w);
    win_openmenu(nonowin_infobox.wh, menu_x() - 256, menu_y() + 128);
    break;
  case 1:
    switch (c[1]) {
    case 0:
      nonowin_savebox.cur = w;
      nonowin_savebox.type = TEXTFILE;
      nonowin_resetsave(&nonowin_savebox);
      win_openmenu(nonowin_savebox.wh, menu_x() - 256, menu_y() + 128);
      break;
    case 1:
      nonowin_savebox.cur = w;
      nonowin_savebox.type = DRAWFILE;
      nonowin_resetsave(&nonowin_savebox);
      win_openmenu(nonowin_savebox.wh, menu_x() - 256, menu_y() + 128);
      break;
    }
    break;
#endif
  case 2:
    if (w->data_in && !w->fresh) {
      task_error(false, "Can't change algorithm now.");
      break;
    }
    switch (c[1]) {
    case 0:
      newalgo = nonogram_AFAST;
      break;
    case 1:
      newalgo = nonogram_ACOMPLETE;
      break;
    case 2:
      newalgo = nonogram_AHYBRID;
      break;
    case 3:
      newalgo = nonogram_AOLSAK;
      break;
    case 4:
      newalgo = nonogram_AFASTOLSAK;
      break;
    case 5:
      newalgo = nonogram_AFASTOLSAKCOMPLETE;
      break;
    case 6:
      newalgo = nonogram_AFASTODDONES;
      break;
    case 7:
      newalgo = nonogram_AFASTODDONESCOMPLETE;
      break;
    default:
      newalgo = nonogram_ANULL;
      break;
    case 9:
      newalgo = nonogram_AFCOMP;
      break;
    case 10:
      newalgo = nonogram_AFFCOMP;
      break;
    }
    if (w->data_in)
      nonogram_unload(&w->solver);
    if (nonogram_setalgo(&w->solver, newalgo) < 0)
      task_error(false, "Can't change algorithm.");
    else {
      w->algo = newalgo;
      nonowin_setstatecols(w->statecol, nonogram_getlinesolvers(&w->solver));
    }
    if (w->data_in)
      nonogram_load(&w->solver, &w->puzzle, w->grid,
                    nonogram_puzzleheight(&w->puzzle)
                    * nonogram_puzzlewidth(&w->puzzle));
    if (win_redrawall(w->vwork) < 0)
      task_complain(task_err());
    if (win_redrawall(w->hwork) < 0)
      task_complain(task_err());
    break;
  case 3:
    switch (c[1]) {
    case 0:
      if (w->ph) {
        if (w->counting)
          nonowin_halt(w);
        else
          w->counting = true, w->haltnext = false;
      } else {
        w->counting = true;
        w->haltnext = false;
        nonowin_restart(w);
      }
      break;
    case 1:
      if (w->ph) {
        if (!w->counting && !w->haltnext)
          nonowin_halt(w);
        else if (w->haltnext)
          w->haltnext = false;
        else
          w->counting = false;
      } else {
        w->counting = false;
        w->haltnext = false;
        nonowin_restart(w);
      }
      break;
    case 2:
      nonowin_restart(w);
      w->counting = false;
      w->haltnext = true;
      break;
    case 3:
      nonowin_clear(w);
      break;
    case 4:
      nonowin_discard(w);
      break;
    }
    break;
  }
}

void nonowin_menuop(void *vw, const wimp_menuchoice *c, const vdu_point *p)
{
  nonowin w = vw;
  switch (c[0]) {
  case 0:
    nonowin_infobox.cur = w;
    nonowin_updatesolutions(w);
    nonowin_updatestatus(w);
    nonowin_updatetitle(w);
    if (win_opensubmenu(nonowin_infobox.wh, p->x, p->y) < 0)
      task_complain(task_err());
    break;
  case 1:
    switch (c[1]) {
    case 0:
      nonowin_savebox.cur = w;
      nonowin_savebox.type = TEXTFILE;
      nonowin_resetsave(&nonowin_savebox);
      if (win_opensubmenu(nonowin_savebox.wh, p->x, p->y) < 0)
        task_complain(task_err());
      break;
    case 1:
      nonowin_savebox.cur = w;
      nonowin_savebox.type = DRAWFILE;
      nonowin_resetsave(&nonowin_savebox);
      if (win_opensubmenu(nonowin_savebox.wh, p->x, p->y) < 0)
        task_complain(task_err());
      break;
    }
    break;
  }
}

menu_handle nonowin_menugen(void *vw, wimp_iconhandle ih, vdu_point *pos)
{
  int i;

  nonowin w = vw;
  menu_select(nonowin_menu.root, 0, w->data_in);
  menu_select(nonowin_menu.save, 0, w->data_in);
  menu_select(nonowin_menu.save, 1, w->data_in);
  menu_select(nonowin_menu.action, 0, w->data_in && !w->finished);
  menu_select(nonowin_menu.action, 1, w->data_in && !w->finished);
  menu_select(nonowin_menu.action, 2, w->data_in && !w->finished);
  menu_select(nonowin_menu.action, 3, w->data_in);
  menu_select(nonowin_menu.action, 4, w->data_in);


  for (i = 0; i < 9; i++) {
    menu_select(nonowin_menu.algo, i, 1);
    menu_select(nonowin_menu.algo, i, !w->data_in || w->fresh);
  }

  menu_tick(nonowin_menu.algo, 0, w->data_in &&
            w->algo == nonogram_AFAST);
  menu_tick(nonowin_menu.algo, 1, w->data_in &&
            w->algo == nonogram_ACOMPLETE);
  menu_tick(nonowin_menu.algo, 2, w->data_in &&
            w->algo == nonogram_AHYBRID);
  menu_tick(nonowin_menu.algo, 3, w->data_in &&
            w->algo == nonogram_AOLSAK);
  menu_tick(nonowin_menu.algo, 4, w->data_in &&
            w->algo == nonogram_AFASTOLSAK);
  menu_tick(nonowin_menu.algo, 5, w->data_in &&
            w->algo == nonogram_AFASTOLSAKCOMPLETE);
  menu_tick(nonowin_menu.algo, 6, w->data_in &&
            w->algo == nonogram_AFASTODDONES);
  menu_tick(nonowin_menu.algo, 7, w->data_in &&
            w->algo == nonogram_AFASTODDONESCOMPLETE);
  menu_tick(nonowin_menu.algo, 8, w->data_in &&
            w->algo == nonogram_ANULL);
  menu_tick(nonowin_menu.algo, 9, w->data_in &&
            w->algo == nonogram_AFCOMP);
  menu_tick(nonowin_menu.algo, 10, w->data_in &&
            w->algo == nonogram_AFFCOMP);
  menu_tick(nonowin_menu.action, 0, w->ph != NULL && w->counting);
  menu_tick(nonowin_menu.action, 1, w->ph != NULL &&
            !w->haltnext && !w->counting);
  menu_tick(nonowin_menu.action, 2, w->ph != NULL && w->haltnext);
  if (pos) pos->x -= 64;
  return nonowin_menu.root;
}

const char *nonowin_menuhelp(void *vw, const wimp_menuchoice *c)
{
  nonowin w = vw;
  switch (c[0]) {
  case 0:
    if (w->data_in)
      return label_default(nonowin_label, "mh_root_file",
                           "\\Rget information about this Nonogram.");
    return label_default(nonowin_label, "mg_root_file",
                         "\\Gthere is no Nonogram to give information about.");
  case 1:
    switch (c[1]) {
    case -1:
      return label_default(nonowin_label, "mh_root_save",
                           "\\Rsave this grid as different types of files.");
    case 0:
      if (w->data_in)
        return label_default(nonowin_label, "mh_save_text",
                             "\\Rsave this grid as a text file.");
      return label_default(nonowin_label, "mg_save_x",
                           "\\Gthere is no grid to save.");
    case 1:
      if (w->data_in)
        return label_default(nonowin_label, "mh_save_draw",
                             "\\Rsave this grid as a Draw file.");
      return label_default(nonowin_label, "mg_save_x",
                           "\\Gthere is no grid to save.");
    }
  case 2:
    switch (c[1]) {
    case -1:
      return label_default(nonowin_label, "mh_root_algo",
                           "\\Rchange the line-solving algorithm.");
    case 0:
      return label_default(nonowin_label, "mh_algo_fast",
                           "\\Suse the Fast algorithm.");
    case 1:
      return label_default(nonowin_label, "mh_algo_comp",
                           "\\Suse the Complete algorithm.");
    case 2:
      return label_default(nonowin_label, "mh_algo_hyb",
                           "\\Suse the Fast/Complete hybrid algorithm.");
    case 3:
      return label_default(nonowin_label, "mh_algo_olsak",
                           "\\Suse the Olsak algorithm.");
    case 4:
      return label_default(nonowin_label, "mh_algo_folsak",
                           "\\Suse the Fast/Olsak hybrid algorithm.");
    case 5:
      return label_default(nonowin_label, "mh_algo_folsakc",
                           "\\Suse the Fast/Olsak/Complete hybrid algorithm.");
    case 6:
      return label_default(nonowin_label, "mh_algo_fodd",
                           "\\Suse the Fast/Oddones hybrid algorithm.");
    case 7:
      return label_default(nonowin_label, "mh_algo_foddc",
                           "\\Suse the Fast/Oddones/Complete "
                           "hybrid algorithm.");
    default:
      return label_default(nonowin_label, "mh_algo_null",
                           "\\Suse the Null algorithm.");
    case 9:
      return label_default(nonowin_label, "mh_algo_fcomp",
                           "\\Suse the Fast-Complete algorithm.");
    case 10:
      return label_default(nonowin_label, "mh_algo_ffcomp",
                           "\\Suse the Fast/Fast-Complete algorithm.");
    }
  case 3:
    switch (c[1]) {
    case -1:
      return label_default(nonowin_label, "mh_root_action",
                           "\\Rstart or stop solving this Nonogram.");
    case 0:
      if (!w->data_in)
        return label_default(nonowin_label, "mg_action_solve",
                             "\\Gthere is no Nonogram to solve.");
      if (w->finished)
        return label_default(nonowin_label, "mg_action_done",
                             "\\Gthe Nonogram is solved.");
      if (w->ph != NULL && !w->haltnext)
        return label_default(nonowin_label, "mh_action_stop",
                             "\\Ssuspend the solution of this Nonogram.");
      if (w->fresh)
        return label_default(nonowin_label, "mh_action_exhaust",
                             "\\Sbegin solving this Nonogram.");
      return label_default(nonowin_label, "mh_action_cont",
                           "\\Scontinue solving this Nonogram.");
    case 1:
      if (!w->data_in)
        return label_default(nonowin_label, "mg_action_solve",
                             "\\Gthere is no Nonogram to solve.");
      if (w->finished)
        return label_default(nonowin_label, "mg_action_done",
                             "\\Gthe Nonogram is solved.");
      if (w->ph != NULL && !w->haltnext)
        return label_default(nonowin_label, "mh_action_stop",
                             "\\Ssuspend the solution of this Nonogram.");
      if (w->fresh)
        return label_default(nonowin_label, "mh_action_start",
                             "\\Sbegin solving this Nonogram.");
      return label_default(nonowin_label, "mh_action_cont",
                           "\\Scontinue solving this Nonogram.");
    case 2:
      if (!w->data_in)
        return label_default(nonowin_label, "mg_action_solve",
                             "\\Gthere is no Nonogram to solve.");
      if (w->finished)
        return label_default(nonowin_label, "mg_action_done",
                             "\\Gthe Nonogram is solved.");
      return label_default(nonowin_label, "mh_action_once",
                           "\\Ssolve a single line of this Nonogram.");
    case 3:
      if (!w->data_in)
        return label_default(nonowin_label, "mg_action_clear",
                             "\\Gthere is no grid to clear.");
      return label_default(nonowin_label, "mh_action_clear",
                           "\\Sclear the grid to begin solving again.");
    case 4:
      if (!w->data_in)
        return label_default(nonowin_label, "mg_action_discard",
                             "\\Gthere is no Nonogram to discard.");
      return label_default(nonowin_label, "mh_action_discard",
                           "\\Sdiscard this Nonogram.");
    }
  }
  return NULL;
}
