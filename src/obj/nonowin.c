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

#include <stdio.h>
#include <time.h>
#include <assert.h>

#include <riscos/vdu/geom.h>
#include <riscos/vdu/hourglass.h>
#include <riscos/vdu/colourtrans.h>
#include <riscos/vdu/action.h>
#include <riscos/wimp/op.h>

#include <yacitros/desktop/win.h>
#include <yacitros/desktop/menu.h>
#include <yacitros/desktop/label.h>
#include <yacitros/desktop/template.h>
#include <yacitros/desktop/transmit.h>
#include <yacitros/desktop/event.h>
#include <yacitros/desktop/win.h>
#include <yacitros/desktop/task.h>

#define LABEL(T,V) label_default(nonowin_label, (T), (V))

/* nonogram events */
void nonowin_drawarea(void *, const struct nonogram_rect *a);
void nonowin_rowfocus(void *, size_t r, int s);
void nonowin_colfocus(void *, size_t, int s);
void nonowin_rowmark(void *, size_t r, size_t s);
void nonowin_colmark(void *, size_t, size_t s);

/* WIMP events */
event_rspcode nonowin_open(void *, const wimp_winloc *a);
event_rspcode nonowin_close(void *);
void nonowin_redraw(void *, const win_offset *, const vdu_box *);
void nonowin_update(void *, const win_offset *, const vdu_box *);

void nonowin_redrawrowlights(void *, const win_offset *, const vdu_box *);
void nonowin_redrawcollights(void *, const win_offset *, const vdu_box *);

const char *nonowin_vhelp(void *, const mouse_state *ms, wimp_iconhandle ih);
const char *nonowin_hhelp(void *, const mouse_state *ms, wimp_iconhandle ih);
const char *nonowin_tickhelp(void *, const mouse_state *ms,
                             wimp_iconhandle ih);

void nonowin_okay(nonowin w, int v)
{
  wimp_iconflags wif1, wif2;
  w->correct = v;
  wif1.word = !!v << 21;
  wif2.word = 1 << 21;
  win_seticonflags(w->okay, 0, wif1, wif2);
}

void nonowin_init(label_bundle *msgs)
{
  if (!nonowin_label) nonowin_label = msgs;
  if (!nonowin_menu.root) {
    nonowin_menu.root = menu_create(LABEL("mt_root", "Nonogram"),
                                    LABEL("mi_root",
                                          ">File,Save,Algorithm,Action"));
    nonowin_menu.save = menu_create(LABEL("mt_save", "Save"),
                                    LABEL("mi_save", ">Text,>Draw"));
    nonowin_menu.algo = menu_create(LABEL("mt_algo", "Algorithm"),
                                    LABEL("mi_algo",
                                          "Fast,Complete,Hybrid"));
    nonowin_menu.action = menu_create(LABEL("mt_action", "Action"),
                                      LABEL("mi_action",
                                            "Exhaust,Run,Once,Clear,Discard"));
    if (!nonowin_menu.root || !nonowin_menu.action ||
        !nonowin_menu.save || !nonowin_menu.algo) {
      nonowin_term();
      return;
    }
    menu_attachsubmenu(nonowin_menu.root, 1, nonowin_menu.save);
    menu_attachsubmenu(nonowin_menu.root, 2, nonowin_menu.algo);
    menu_attachsubmenu(nonowin_menu.root, 3, nonowin_menu.action);
  }

  if (!nonowin_tpl) {
    nonowin_tpl = template_load("nonogram", NULL, 0, 1000);
    if (!nonowin_tpl) {
      nonowin_term();
      return;
    }
  }

  if (!nonowin_vwkt) {
    nonowin_vwkt = template_load("vwork", NULL, 0, 1000);
    if (!nonowin_vwkt) {
      nonowin_term();
      return;
    }
  }

  if (!nonowin_okt) {
    nonowin_okt = template_load("okay", NULL, 1, 1000);
    if (!nonowin_okt) {
      nonowin_term();
      return;
    }
  }

  if (!nonowin_infobox.wh) {
    template_handle tmp = template_load("file", NULL, 5, 1000);
    if (!tmp) {
      nonowin_term();
      return;
    }
    nonowin_infobox.wh = win_create(tmp, &nonowin_infobox);
    if (!nonowin_infobox.wh) {
      nonowin_term();
      return;
    }
    nonowin_infobox.data = template_release(tmp);
    win_conf_open(nonowin_infobox.wh, &nonowin_openinfo);
    win_conf_close(nonowin_infobox.wh, &nonowin_closeinfo);
  }

  if (!nonowin_savebox.wh) {
    template_handle tmp = template_load("save", NULL, 5, 1000);
    if (!tmp) {
      nonowin_term();
      return;
    }
    nonowin_savebox.wh = win_create(tmp, &nonowin_savebox);
    if (!nonowin_savebox.wh) {
      nonowin_term();
      return;
    }
    nonowin_savebox.data = template_release(tmp);
    nonowin_savebox.left = nonowin_savebox.top = false;
    nonowin_savebox.right = nonowin_savebox.bottom = true;
    nonowin_savebox.cells = true;
    win_conf_open(nonowin_savebox.wh, &nonowin_opensave);
    win_conf_close(nonowin_savebox.wh, &nonowin_closesave);
    win_conf_click(nonowin_savebox.wh, &nonowin_clicksave);
    win_conf_help(nonowin_savebox.wh, &nonowin_savehelp);
    transmit_init(&nonowin_savebox.trc);
  }
}

void nonowin_term(void)
{
  menu_destroy(nonowin_menu.root), nonowin_menu.root = NULL;
  menu_destroy(nonowin_menu.save), nonowin_menu.save = NULL;
  menu_destroy(nonowin_menu.algo), nonowin_menu.algo = NULL;
  menu_destroy(nonowin_menu.action), nonowin_menu.action = NULL;
  free(template_release(nonowin_tpl)), nonowin_tpl = NULL;
  free(template_release(nonowin_vwkt)), nonowin_vwkt = NULL;
  free(template_release(nonowin_okt)), nonowin_okt = NULL;
  win_destroy(nonowin_infobox.wh), nonowin_infobox.wh = NULL;
  free(nonowin_infobox.data), nonowin_infobox.data = NULL;
  win_destroy(nonowin_savebox.wh), nonowin_savebox.wh = NULL;
  free(nonowin_savebox.data), nonowin_savebox.data = NULL;
}

#define XSCALE 16
#define YSCALE 16
#define XGAP 4
#define YGAP 4
#define XOFFSET 48
#define YOFFSET 48

void nonowin_cell2work(nonowin w, const struct nonogram_rect *c,
                       vdu_box *a)
{
  /* OPT: use magnification factors in (w) to determine coords */
  a->min.x = c->min.x * XSCALE + XOFFSET;
  a->max.x = c->max.x * XSCALE - XGAP + XOFFSET;
  a->min.y = YGAP - YOFFSET - c->max.y * YSCALE;
  a->max.y = 0 - YOFFSET - c->min.y * YSCALE;
}

static int coord_div(int sor, int dend)
{
  return (sor < 0) ? -((dend - 1 - sor) / dend) : sor / dend;
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

void nonowin_work2cell(nonowin w, const vdu_box *a,
                       struct nonogram_rect *c)
{
  int tmp;
  /* OPT: use magnification factors in (w) to determine coords */
  tmp = coord_div(a->min.x - XOFFSET + XGAP, XSCALE);
  c->min.x = MAX(tmp, 0);
  c->max.x = coord_div(a->max.x - XOFFSET - 1, XSCALE) + 1;
  tmp = coord_div(YGAP - YOFFSET - a->max.y, YSCALE);
  c->min.y = MAX(tmp, 0);
  c->max.y = coord_div(0 - YOFFSET - a->min.y - 1, YSCALE) + 1;
}

int nonowin_vpane(void *w, const wimp_windim *parent, wimp_windim *cur)
{
  cur->visible.min.x = parent->visible.min.x;
  cur->visible.max.x = parent->visible.min.x + XOFFSET - XGAP;
  cur->visible.min.y = parent->visible.min.y;
  cur->visible.max.y = parent->visible.max.y - YOFFSET + YGAP;
  cur->scroll.x = YGAP;
  cur->scroll.y = parent->scroll.y - YOFFSET + YGAP;
  return true;
}

int nonowin_hpane(void *w, const wimp_windim *parent, wimp_windim *cur)
{
  cur->visible.min.x = parent->visible.min.x + XOFFSET - XGAP;
  cur->visible.max.x = parent->visible.max.x;
  cur->visible.min.y = parent->visible.max.y - YOFFSET + XGAP;
  cur->visible.max.y = parent->visible.max.y;
  cur->scroll.x = parent->scroll.x + XOFFSET - XGAP;
  cur->scroll.y = -XGAP;
  return true;
}

int nonowin_okaypane(void *w, const wimp_windim *parent, wimp_windim *cur)
{
  cur->visible.min.x = parent->visible.min.x;
  cur->visible.max.x = parent->visible.min.x + XOFFSET - XGAP;
  cur->visible.min.y = parent->visible.max.y - YOFFSET + XGAP;
  cur->visible.max.y = parent->visible.max.y;
  cur->scroll.x = 0;
  cur->scroll.y = 0;
  return true;
}

nonowin nonowin_create(void)
{
  template_handle t;
  nonowin w;

  nonowin_init(NULL);
  if (!nonowin_menu.root || !nonowin_tpl) return NULL;

  w = malloc(sizeof *w);
  if (!w) {
    task_error(false, "Out of memory opening new puzzle.");
    return NULL;
  }

  t = template_copy(nonowin_tpl);
  if (!t) {
    free(w);
    task_error(false, "Out of memory opening template for new puzzle.");
    return NULL;
  }
  w->wh = win_create(t, w);
  w->wh_d = template_release(t);

  t = template_copy(nonowin_vwkt);
  if (!t) {
    win_destroy(w->wh);
    free(w->wh_d);
    free(w);
    task_error(false, "Out of memory opening template for vertical area.");
    return NULL;
  }
  w->vwork = win_create(t, w);
  w->vwork_d = template_release(t);

  t = template_copy(nonowin_vwkt);
  if (!t) {
    win_destroy(w->vwork);
    free(w->vwork_d);
    win_destroy(w->wh);
    free(w->wh_d);
    free(w);
    task_error(false, "Out of memory opening template for horizontal area.");
    return NULL;
  }
  w->hwork = win_create(t, w);
  w->hwork_d = template_release(t);

  t = template_copy(nonowin_okt);
  if (!t) {
    win_destroy(w->hwork);
    free(w->hwork_d);
    win_destroy(w->vwork);
    free(w->vwork_d);
    win_destroy(w->wh);
    free(w->wh_d);
    free(w);
    task_error(false, "Out of memory opening template for 'ok' area.");
    return NULL;
  }
  w->okay = win_create(t, w);
  w->okay_d = template_release(t);

  nonogram_initsolver(&w->solver);
  nonogram_setclient(&w->solver, &nonowin_client, w);
  nonogram_setdisplay(&w->solver, &nonowin_display, w);
  w->algo = nonogram_AFFCOMP;
  nonogram_setalgo(&w->solver, w->algo);
  nonowin_setstatecols(w->statecol, nonogram_getlinesolvers(&w->solver));

  win_conf_redraw(w->wh, &nonowin_redraw);
  win_conf_update(w->wh, &nonowin_update);
  win_conf_open(w->wh, &nonowin_open);
  win_conf_close(w->wh, &nonowin_close);
  win_conf_load(w->wh, &nonowin_load);
  win_conf_help(w->wh, &nonowin_help);

  win_conf_pane(w->vwork, &nonowin_vpane);
  win_conf_redraw(w->vwork, &nonowin_redrawrowlights);
  win_conf_update(w->vwork, &nonowin_redrawrowlights);
  win_conf_help(w->vwork, &nonowin_vhelp);
  win_makepane(w->vwork, w->wh);

  win_conf_pane(w->hwork, &nonowin_hpane);
  win_conf_redraw(w->hwork, &nonowin_redrawcollights);
  win_conf_update(w->hwork, &nonowin_redrawcollights);
  win_conf_help(w->hwork, &nonowin_hhelp);
  win_makepane(w->hwork, w->wh);

  win_conf_pane(w->okay, &nonowin_okaypane);
  win_conf_help(w->okay, &nonowin_tickhelp);
  win_makepane(w->okay, w->wh);

  w->ma = win_attachmenu(w->wh, &nonowin_menugen,
                         &nonowin_menusel,
                         &nonowin_menuop,
                         &nonowin_menuhelp, w);

  w->scale.xmul = w->scale.ymul = w->scale.xdiv = w->scale.ydiv = 1;
  w->ph = NULL; /* not claiming null events */
  w->grid = NULL;
  w->data_in = false; /* renders grid, puzzle and solver as irrelevant */
  w->inval = NULL;
  strcpy(w->filename, "<Untitled>");
  strcpy(w->drawname, "DrawFile");
  strcpy(w->textname, "TextFile");
  nonowin_resize(w);
  if (win_open(w->wh) < 0)
    task_complain(task_err());

  return w;
}

void nonowin_resize(nonowin w)
{
  struct nonogram_rect cells;
  vdu_box new_ext, new2;
  if (w->data_in) {
    size_t width, height;
    height = nonogram_puzzleheight(&w->puzzle);
    width = nonogram_puzzlewidth(&w->puzzle);
    cells.min.x = cells.min.y = 0;
    cells.max.x = width;
    cells.max.y = height;
  } else {
    cells.min.x = cells.min.y = 0;
    cells.max.x = 10;
    cells.max.y = 10;
  }

  nonowin_cell2work(w, &cells, &new_ext);
  new_ext.max.x += XGAP;
  new_ext.min.y -= YGAP;
  new_ext.min.x -= XOFFSET;
  new_ext.max.y += YOFFSET;
  if (win_setextent(w->wh, &new_ext) < 0)
    task_complain(task_err());
  else if (win_redrawall(w->wh) < 0)
    task_complain(task_err());

  new2 = new_ext;
  new_ext.min.x += XOFFSET - (XOFFSET - 2 * XSCALE - XGAP);
  new_ext.max.x = new_ext.min.x + XOFFSET;
  if (win_setextent(w->vwork, &new_ext) < 0)
    task_complain(task_err());
  else if (win_redrawall(w->vwork) < 0)
    task_complain(task_err());

  new2.max.y -= YOFFSET - (YOFFSET - 2 * YSCALE - YGAP);
  new2.min.y = new2.max.y - YOFFSET;
  if (win_setextent(w->hwork, &new2) < 0)
    task_complain(task_err());
  else if (win_redrawall(w->hwork) < 0)
    task_complain(task_err());
}

void nonowin_unload(nonowin w)
{
  if (!w->data_in)
    return;
  w->data_in = false;
  nonowin_halt(w);
  nonogram_unload(&w->solver);
  nonogram_freegrid(w->grid), w->grid = NULL;
  nonogram_freepuzzle(&w->puzzle);
}

void nonowin_clear(nonowin w)
{
  size_t width, height;
  struct nonogram_rect allarea;

  if (!w->data_in) return;
  nonowin_halt(w);

  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);

  nonogram_unload(&w->solver);
  nonogram_cleargrid(w->grid, width, height);
  nonogram_load(&w->solver, &w->puzzle, w->grid, width * height);

  w->solutions = 0;
  w->finished = false;
  w->fresh = true;
  nonowin_okay(w, false);
  nonowin_updatestatus(w);
  nonowin_updatesolutions(w);
  allarea.min.x = allarea.min.y = 0;
  allarea.max.x = width;
  allarea.max.y = height;
  nonowin_drawarea(w, &allarea);
  if (win_redrawall(w->hwork) < 0)
    task_complain(task_err());
  if (win_redrawall(w->vwork) < 0)
    task_complain(task_err());
}

void nonowin_inval(nonowin w)
{
  event_cancelidler(w->inval), w->inval = NULL;
  if (win_redrawall(w->wh) < 0)
    task_complain(task_err());
}

void nonowin_idle(void *vw)
{
  nonowin w = vw;
  int tries = 20;
  (void) vdu_hourglasson();
  w->fresh = false;
  if (w->correct)
    nonowin_okay(w, false);
  switch (nonogram_runcycles_tries(&w->solver, &tries)) {
  case nonogram_FINISHED:
    /* all possibilities have been tried */
    w->finished = true;
    nonowin_updatesolutions(w);
    nonowin_halt(w);
    break;
  case nonogram_FOUND:
    /* a solution is found, but there are other possibilities */
    nonowin_updatesolutions(w);
    if (!w->counting)
      nonowin_halt(w);
    break;
  case nonogram_LINE:
    /* a whole line is completed, but no solution found */
    if (w->haltnext)
      nonowin_halt(w);
    break;
  }
  (void) vdu_hourglassoff();
}

void nonowin_found(void *vw)
{
  nonowin w = vw;
  w->solutions++;
  nonowin_okay(w, true);
}

void nonowin_restart(nonowin w)
{
  if (!w->data_in || w->ph) return;
  w->ph = event_setidler(&nonowin_idle, w);
  w->haltnext = false;
  if (w->correct)
    nonowin_okay(w, false);
  nonowin_updatestatus(w);
}

void nonowin_halt(nonowin w)
{
  if (w->ph) {
    event_cancelidler(w->ph);
    w->ph = NULL;
  }
  nonowin_updatestatus(w);
}

void nonowin_destroy(nonowin w)
{
  nonowin_unload(w);
  nonogram_termsolver(&w->solver);
  event_cancelidler(w->inval);
  win_detachmenu(w->ma);
  win_destroy(w->wh);
  free(w->wh_d);
  win_destroy(w->vwork);
  free(w->vwork_d);
  win_destroy(w->hwork);
  free(w->hwork_d);
  win_destroy(w->okay);
  free(w->okay_d);
  free(w);
}

void nonowin_discard(nonowin w)
{
  nonowin_unload(w);
  nonowin_setfilename(w, "<Untitled>");
  nonowin_resize(w);
}

/* nonogram events */
void nonowin_drawarea(void *vw, const struct nonogram_rect *a)
{
  nonowin w = vw;
  vdu_box work;
  nonowin_cell2work(w, a, &work);
  if (win_update(w->wh, &work) < 0)
    task_complain(task_err());
}

/* OPT: get these to do something useful */
void nonowin_rowfocus(void *vw, size_t r, int s)
{
  nonowin w = vw;
  struct nonogram_rect onecell;
  vdu_box square;

  onecell.min.x = 1;
  onecell.max.x = 2;
  onecell.max.y = (onecell.min.y = r) + 1;
  nonowin_cell2work(w, &onecell, &square);
  if (win_update(w->vwork, &square) < 0)
    task_complain(task_err());
}

void nonowin_colfocus(void *vw, size_t c, int s)
{
  nonowin w = vw;
  struct nonogram_rect onecell;
  vdu_box square;

  onecell.max.x = (onecell.min.x = c) + 1;
  onecell.min.y = 1;
  onecell.max.y = 2;
  nonowin_cell2work(w, &onecell, &square);
  if (win_update(w->hwork, &square) < 0)
    task_complain(task_err());
}

void nonowin_rowmark(void *vw, size_t from, size_t to)
{
  nonowin w = vw;
  struct nonogram_rect column;
  vdu_box square;

  column.min.x = 0;
  column.max.x = 1;
  column.min.y = from;
  column.max.y = to;
  nonowin_cell2work(w, &column, &square);
  if (win_update(w->vwork, &square) < 0)
    task_complain(task_err());
}

void nonowin_colmark(void *vw, size_t from, size_t to)
{
  nonowin w = vw;
  struct nonogram_rect row;
  vdu_box square;

  row.min.x = from;
  row.max.x = to;
  row.min.y = 0;
  row.max.y = 1;
  nonowin_cell2work(w, &row, &square);
  if (win_update(w->hwork, &square) < 0)
    task_complain(task_err());
}

/* WIMP events */
event_rspcode nonowin_open(void *vw, const wimp_winloc *a)
{
  nonowin w = vw;
  if (win_openat(w->wh, a) < 0)
    task_complain(task_err());
  return event_CLAIM;
}

event_rspcode nonowin_close(void *vw)
{
  nonowin w = vw;
  nonowin_destroy(w);
  return event_CLAIM;
}

void nonowin_plotcell(nonowin w, const win_offset *gctx,
                      const struct nonogram_point *pos)
{
  size_t width, height;
  struct nonogram_rect onecell;
  _kernel_oserror *oe;
  vdu_box square;
  vdu_point centre;

  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);

  onecell.max.x = (onecell.min.x = pos->x) + 1;
  onecell.max.y = (onecell.min.y = pos->y) + 1;
  nonowin_cell2work(w, &onecell, &square);

  switch (w->grid[pos->x + pos->y * width]) {
  case nonogram_DOT:
    if ((oe = vdu_ctranssetnearest(NULL, vdu_PALENTRY(255,255,255),
                                   vdu_FOREGROUND, vdu_OVERWRITE)) != NULL)
      task_complain(oe);
    if (win_fillrectangle(gctx, &square) < 0)
      task_complain(task_err());
    if ((oe = vdu_ctranssetnearest(NULL, vdu_PALENTRY(0,0,0),
                                   vdu_FOREGROUND, vdu_OVERWRITE)) != NULL)
      task_complain(oe);
    vdu_findboxcentre(&centre, &square);
    if (win_fillcircle(gctx, &centre, 1) < 0)
      task_complain(task_err());
    break;
  case nonogram_SOLID:
    if ((oe = vdu_ctranssetnearest(NULL, vdu_PALENTRY(0,0,0),
                                   vdu_FOREGROUND, vdu_OVERWRITE)) != NULL)
      task_complain(oe);
    if (win_fillrectangle(gctx, &square) < 0)
      task_complain(task_err());
    break;
  default:
    if ((oe = vdu_ctranssetnearest(NULL, vdu_PALENTRY(255,255,255),
                                   vdu_FOREGROUND, vdu_OVERWRITE)) != NULL)
      task_complain(oe);
    if (win_fillrectangle(gctx, &square) < 0)
      task_complain(task_err());
    break;
  }
}

void nonowin_plotcells(nonowin w, const win_offset *gctx,
                       const struct nonogram_rect *area)
{
  struct nonogram_point pos;
  if (!w->data_in) return;
  /* NEEDED: 2D loop through cells, redrawing them */
  for (pos.y = area->min.y; pos.y < area->max.y; pos.y++)
    for (pos.x = area->min.x; pos.x < area->max.x; pos.x++)
      nonowin_plotcell(w, gctx, &pos);
}

void nonowin_redraw(void *vw, const win_offset *gctx, const vdu_box *clip)
{
  nonowin w = vw;
  size_t width, height;
  struct nonogram_rect cells;

  if (!w->data_in) return;
  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);
  /* NEEDED: draw grid in (*a) */
  nonowin_work2cell(w, clip, &cells);
  if (cells.max.x > width) cells.max.x = width;
  if (cells.max.y > height) cells.max.y = height;
  nonowin_plotcells(w, gctx, &cells);
  /* NEEDED: also test for flags and focus */
}

void nonowin_update(void *vw, const win_offset *gctx, const vdu_box *clip)
{
  nonowin w = vw;
  struct nonogram_rect cells;

  nonowin_work2cell(w, clip, &cells);
  nonowin_plotcells(w, gctx, &cells);
}

const char *nonowin_help(void *vw, const mouse_state *ms, wimp_iconhandle ih)
{
  nonowin w = vw;
  size_t width, height;
  wimp_winstate wst;
  vdu_box pos;
  struct nonogram_rect ca;
  static char temp[400];
  const char *pref;

  if (ms->buttons.word)
    return LABEL("wh_idiot", "Don't bother clicking; it "
                 "doesn't do anything in this \\w.");

  if (!w->data_in)
    return LABEL("wh_empty", "\\Wis an empty Nonogram puzzle.|M"
                 "Drag a Nonogram puzzle file here to prepare to solve it.");
  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);
  pos.min = pos.max = ms->pos;
  pos.max.x += 4;
  pos.max.y += 4;
  if (win_getstate(w->wh, &wst) < 0) {
    task_complain(task_err());
    return "error";
  }
  wimp_screen2work_area(&wst.loc.dim, &pos, &pos);
  nonowin_work2cell(w, &pos, &ca);

  if (w->fresh)
    pref = LABEL("wh_clear", "\\Wshowing an untouch Nonogram "
                 "puzzle, and is ready to begin solving it.");
  else if (w->finished)
    pref = LABEL("wh_finished",
                 "\\Wshowing an exhausted Nonogram puzzle.");
  else if (w->ph != NULL && !w->haltnext)
    pref = LABEL("wh_running", "\\Wsolving a Nonogram puzzle.");
  else
    pref = LABEL("wh_paused",
                 "\\Wpaused whilst solving a Nonogram puzzle.");
  if (ca.min.x + 1 != ca.max.x || ca.min.y + 1 != ca.max.y ||
      ca.max.x > width || ca.max.y > height)
    return pref;
  snprintf(temp, sizeof temp, "%s%s(%zu,%zu)%s", pref,
           LABEL("wh_preco", "|MThe co-ordinates are "),
           ca.min.x, ca.min.y,
           LABEL("wh_postco", "."));
  return temp;
}

void nonowin_setstatecols(vdu_palentry *vpe, nonogram_level max)
{
  assert(max <= MAX_LEVEL);
  for (nonogram_level state = 0; state <= max; state++) 
    if (max) {
      unsigned char red = (0x10u - 0x00u) * (max - state) / max;
      unsigned char green = (0xffu - 0x50u) * state / max + 0x50u;
      vpe[state] = vdu_PALENTRY(red, green, 0);
    } else
      vpe[state] = vdu_PALENTRY(0, 0, 0);
}

void nonowin_plotrowflag(nonowin r, const win_offset *gctx,
                         size_t rowno, int state)
{
  struct nonogram_rect onecell;
  vdu_box square;
  vdu_palentry vpe;
  _kernel_oserror *oe;

  onecell.max.x = (onecell.min.x = 0) + 1;
  onecell.max.y = (onecell.min.y = rowno) + 1;
  nonowin_cell2work(r, &onecell, &square);

#if 1
  vpe = r->statecol[state];
#else
  vpe = state ? (state == 2 ? vdu_PALENTRY(0,0xf0,0)
                 : vdu_PALENTRY(0,0x80,0))
    : vdu_PALENTRY(0x10,0x50,0);
#endif
  if ((oe = vdu_ctranssetnearest(NULL, vpe, vdu_FOREGROUND, vdu_OVERWRITE))
      != NULL)
    task_complain(oe);
  if (win_fillrectangle(gctx, &square) < 0)
    task_complain(task_err());
}

void nonowin_plotrowfocus(nonowin r, const win_offset *gctx,
                          size_t rowno, int state)
{
  struct nonogram_rect onecell;
  vdu_box square;
  vdu_palentry vpe;
  _kernel_oserror *oe;

  onecell.max.x = (onecell.min.x = 1) + 1;
  onecell.max.y = (onecell.min.y = rowno) + 1;
  nonowin_cell2work(r, &onecell, &square);

  vpe = state ? vdu_PALENTRY(0xf0,0xb0,0) : vdu_PALENTRY(0xd0,0,0);
  if ((oe = vdu_ctranssetnearest(NULL, vpe, vdu_FOREGROUND, vdu_OVERWRITE))
      != NULL)
    task_complain(oe);
  if (win_fillrectangle(gctx, &square) < 0)
    task_complain(task_err());
}

void nonowin_plotcolflag(nonowin r, const win_offset *gctx,
                         size_t colno, int state)
{
  struct nonogram_rect onecell;
  vdu_box square;
  vdu_palentry vpe;
  _kernel_oserror *oe;

  onecell.max.x = (onecell.min.x = colno) + 1;
  onecell.max.y = (onecell.min.y = 0) + 1;
  nonowin_cell2work(r, &onecell, &square);

#if 1
  vpe = r->statecol[state];
#else
  vpe = state ? (state == 2 ? vdu_PALENTRY(0,0xf0,0)
                 : vdu_PALENTRY(0,0x80,0))
    : vdu_PALENTRY(0x10,0x50,0);
#endif
  if ((oe = vdu_ctranssetnearest(NULL, vpe, vdu_FOREGROUND, vdu_OVERWRITE))
      != NULL)
    task_complain(oe);
  if (win_fillrectangle(gctx, &square) < 0)
    task_complain(task_err());
}

void nonowin_plotcolfocus(nonowin r, const win_offset *gctx,
                          size_t colno, int state)
{
  struct nonogram_rect onecell;
  vdu_box square;
  vdu_palentry vpe;
  _kernel_oserror *oe;

  onecell.max.x = (onecell.min.x = colno) + 1;
  onecell.max.y = (onecell.min.y = 1) + 1;
  nonowin_cell2work(r, &onecell, &square);

  vpe = state ? vdu_PALENTRY(0xf0,0xb0,0) : vdu_PALENTRY(0xd0,0,0);
  if ((oe = vdu_ctranssetnearest(NULL, vpe, vdu_FOREGROUND, vdu_OVERWRITE))
      != NULL)
    task_complain(oe);
  if (win_fillrectangle(gctx, &square) < 0)
    task_complain(task_err());
}

void nonowin_redrawrowlights(void *vw, const win_offset *gctx,
                             const vdu_box *clip)
{
  nonowin w = vw;
  size_t width, height;
  struct nonogram_rect cells;
  size_t rowno;

  if (!w->data_in) return;
  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);
  nonowin_work2cell(w, clip, &cells);
  if (cells.max.y > height) cells.max.y = height;

  for (rowno = cells.min.y; rowno < cells.max.y; rowno++) {
    if (cells.min.x == 0 && cells.max.x > 0)
      nonowin_plotrowflag(w, gctx, rowno,
                          nonogram_getrowmark(&w->solver, rowno));
    if (cells.min.x <= 1u && cells.max.x > 1u)
      nonowin_plotrowfocus(w, gctx, rowno,
                           nonogram_getrowfocus(&w->solver, rowno));
  }
}

void nonowin_redrawcollights(void *vw, const win_offset *gctx,
                             const vdu_box *clip)
{
  nonowin w = vw;
  size_t width, height;
  struct nonogram_rect cells;
  size_t colno;

  if (!w->data_in) return;
  height = nonogram_puzzleheight(&w->puzzle);
  width = nonogram_puzzlewidth(&w->puzzle);
  nonowin_work2cell(w, clip, &cells);
  if (cells.max.x > width) cells.max.x = width;

  for (colno = cells.min.x; colno <= cells.max.x; colno++) {
    if (cells.min.y == 0 && cells.max.y > 0)
      nonowin_plotcolflag(w, gctx, colno,
                          nonogram_getcolmark(&w->solver, colno));
    if (cells.min.y <= 1u && cells.max.y > 1u)
      nonowin_plotcolfocus(w, gctx, colno,
                           nonogram_getcolfocus(&w->solver, colno));
  }
}

const char *nonowin_vhelp(void *vw, const mouse_state *ms,
                          wimp_iconhandle ih)
{
  nonowin w = vw;
  size_t width, height;
  vdu_box area;
  struct nonogram_rect cells;
  wimp_winstate wst;

  if (win_getstate(w->vwork, &wst) < 0) {
    task_complain(task_err());
    return "error";
  }

  area.min = area.max = ms->pos;
  area.max.x += 4;
  area.max.y += 4;
  wimp_screen2work_area(&wst.loc.dim, &area, &area);
  nonowin_work2cell(w, &area, &cells);

  if (w->data_in) {
    height = nonogram_puzzleheight(&w->puzzle);
    width = nonogram_puzzlewidth(&w->puzzle);
  }

  if (!w->data_in || cells.max.x > 2 || cells.max.y > height ||
      cells.max.y != cells.min.y + 1 || cells.max.x != cells.min.x + 1)
    return LABEL("wa_vwork",
                 "\\Wshowing the state of each row.");
  switch (cells.min.x) {
  case 0:
    return nonogram_getrowmark(&w->solver, cells.min.y) ?
      LABEL("wa_vwork_infoon",
            "This light is on because this row has unused information.") :
              LABEL("wa_vwork_infooff",
                    "This light is off because this row's information"
                    " is used up.");
  case 1:
    return nonogram_getrowfocus(&w->solver, cells.min.y) ?
      LABEL("wa_vwork_focuson",
            "This light is on because this row is being processed.") :
              LABEL("wa_vwork_focusoff",
                    "This light is off because this row is not being"
                    " processed.");
  }

  return "No help!";
}

const char *nonowin_hhelp(void *vw, const mouse_state *ms,
                          wimp_iconhandle ih)
{
  nonowin w = vw;
  size_t width, height;
  vdu_box area;
  struct nonogram_rect cells;
  wimp_winstate wst;

  if (win_getstate(w->hwork, &wst) < 0) {
    task_complain(task_err());
    return "error";
  }

  area.min = area.max = ms->pos;
  area.max.x += 4;
  area.max.y += 4;
  wimp_screen2work_area(&wst.loc.dim, &area, &area);
  nonowin_work2cell(w, &area, &cells);

  if (w->data_in) {
    height = nonogram_puzzleheight(&w->puzzle);
    width = nonogram_puzzlewidth(&w->puzzle);
  }

  if (!w->data_in || cells.max.y > 2 || cells.max.x > width ||
      cells.max.x != cells.min.x + 1 || cells.max.y != cells.min.y + 1)
    return LABEL("wa_hwork",
                 "\\Wshowing the state of each column.");
  switch (cells.min.y) {
  case 0:
    return nonogram_getcolmark(&w->solver, cells.min.x) ?
      LABEL("wa_hwork_infoon",
            "This light is on because this column has unused information.") :
              LABEL("wa_hwork_infooff",
                    "This light is off because this column's information"
                    " is used up.");
  case 1:
    return nonogram_getcolfocus(&w->solver, cells.min.x) ?
      LABEL("wa_hwork_focuson",
            "This light is on because this column is being processed.") :
              LABEL("wa_hwork_focusoff",
                    "This light is off because this column is not being"
                    " processed.");
  }

  return NULL;
}

const char *nonowin_tickhelp(void *vw, const mouse_state *ms,
                             wimp_iconhandle ih)
{
  nonowin w = vw;
  if (ih != 0) return NULL;
  if (!w->data_in) return NULL;

  if (w->correct)
    return LABEL("wh_ticked", "This means the solution is correct.");
  else
    return LABEL("wh_unticked", "This means the grid "
                 "currently does not hold a correct solution.");

  return NULL;
}

const struct nonogram_display nonowin_display = {
  &nonowin_drawarea,
  &nonowin_rowfocus,
  &nonowin_colfocus,
  &nonowin_rowmark,
  &nonowin_colmark
};

const struct nonogram_client nonowin_client = {
  &nonowin_found
};
