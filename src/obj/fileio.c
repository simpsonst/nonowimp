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

#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>

#include "nonoint.h"

#ifndef PATH_PER_SOLID
#include <edges.h>
#endif

#include <riscos/kbd/ikn.h>
#include <riscos/wimp/op.h>
#include <riscos/vdu/types.h>
#include <riscos/vdu/hourglass.h>
#include <riscos/draw/types.h>
#include <riscos/drawfile/types.h>

#include <yacitros/vector/op.h>

#include <yacitros/desktop/drag.h>
#include <yacitros/desktop/task.h>
#include <yacitros/desktop/win.h>
#include <yacitros/desktop/event.h>
#include <yacitros/desktop/label.h>
#include <yacitros/desktop/transmit.h>

static const char *get_leafname(const char *s)
{
  const char *leafname = strrchr(s, '.');
  if (leafname) {
    return ++leafname;
  } else {
    leafname = strchr(s, ':');
    if (leafname) {
      return ++leafname;
    } else {
      return s;
    }
  }
}

void nonowin_setfilename(nonowin w, const char *s)
{
  const char *dot;
  strncpy(w->filename, s, sizeof(w->filename));
  dot = get_leafname(s);
  win_settitle(w->wh, dot);
  strcpy(w->textname, dot);
  strcpy(w->drawname, dot);
  nonowin_updatetitle(w);
}

const char *nonowin_savehelp(void *vc, const mouse_state *ms,
                             wimp_iconhandle ih)
{
  nonowin_savectxt *c = vc;
  switch (ih) {
  case 0: /* okay */
    return label_default(nonowin_label, "ih_save_okay",
                         "Click here to save the data with the current"
                         " filename. If it is not a full pathname, you"
                         " must drag the icon to a directory display "
                         "first.");
  case 2: /* filename */
    return label_default(nonowin_label, "ih_save_filename",
                         "This shows the filename for this data. If it"
                         " is not a full pathname, drag "
                         "the icon into a directory display.");
  case 3: /* icon */
    return label_default(nonowin_label, "ih_save_file",
                         "Drag the icon to the directory in which you want"
                         " to save the file|MOr, drag it to the program"
                         " into which you want to transfer the data.");
  case 4: /* left button */
    if (c->type == TEXTFILE) break;
    return label_default(nonowin_label,
                         "ih_save_left", "Select this to make "
                         "row clues appear down the left of the grid.");
  case 5: /* right button */
    if (c->type == TEXTFILE) break;
    return label_default(nonowin_label, "ih_save_right",
                         "Select this to make "
                         "row clues appear down the right of the grid.");
  case 6: /* top button */
    if (c->type == TEXTFILE) break;
    return label_default(nonowin_label, "ih_save_top",
                         "Select this to make "
                         "column clues appear along the top of the grid.");
  case 7: /* bottom button */
    if (c->type == TEXTFILE) break;
    return label_default(nonowin_label,
                         "ih_save_bottom", "Select this to make column"
                         " clues appear along the bottom of the grid.");
  case 8: /* cells button */
    if (c->type == TEXTFILE) break;
    return label_default(nonowin_label, "ih_save_cells", "Select this to "
                         "produce a grid with currently known cells.");
  }
  return label_default(nonowin_label, "wa_save", "This box allows you to "
                       "save the grid to a file, or transfer it to another "
                       "application.");
}

void nonowin_resetsave(nonowin_savectxt *c)
{
  wimp_iconflags eor, clear;
  if (!c->cur) return;
  win_setval(c->wh, 3, c->type == TEXTFILE ? "sfile_fff" : "sfile_aff");
  win_setfield(c->wh, 2,
               c->type == TEXTFILE ? c->cur->textname : c->cur->drawname);

  eor.word = clear.word = 0;
  clear.bits_colour.selected = clear.bits_colour.disabled = 1;
  eor.bits_colour.disabled = c->type == TEXTFILE;

  /* set buttons according to c->left, top etc... */
  /* enable buttons for Draw file; disable for text file */
  eor.bits_colour.selected = c->left;
  win_seticonflags(c->wh, 4, eor, clear);
  eor.bits_colour.selected = c->right;
  win_seticonflags(c->wh, 5, eor, clear);
  eor.bits_colour.selected = c->top;
  win_seticonflags(c->wh, 6, eor, clear);
  eor.bits_colour.selected = c->bottom;
  win_seticonflags(c->wh, 7, eor, clear);
  eor.bits_colour.selected = c->cells;
  win_seticonflags(c->wh, 8, eor, clear);
}

event_rspcode nonowin_keysave(void *vc, const wimp_caretpos *cb,
                              wimp_iconhandle ih, wimp_keycode k);

event_rspcode nonowin_opensave(void *vc, const wimp_winloc *loc)
{
  nonowin_savectxt *c = vc;
  win_conf_key(c->wh, &nonowin_keysave);

  if (win_openat(c->wh, loc) < 0)
    task_complain(task_err());
  return event_CLAIM;
}

event_rspcode nonowin_closesave(void *vc)
{
  nonowin_savectxt *c = vc;
  win_close(c->wh);
  c->cur = NULL;
  win_conf_key(c->wh, NULL);
  return event_CLAIM;
}

int nonowin_saveokay(nonowin_savectxt *c, const char *newname)
{
  FILE *fp;
  char command[500];
  wimp_iconflags flags;
  int rc;

  /* test for complete pathname */
  if (c->safe && !strchr(newname, '.')) {
    task_error(false, "%s",
               label_default(nonowin_label, "em_dragsave",
                             "To save, drag the icon to"
                             " a directory display"));
    return false;
  }

  /* copy button states */
  win_geticonflags(c->wh, 4, &flags);
  c->left = flags.bits_colour.selected;
  win_geticonflags(c->wh, 5, &flags);
  c->right = flags.bits_colour.selected;
  win_geticonflags(c->wh, 6, &flags);
  c->top = flags.bits_colour.selected;
  win_geticonflags(c->wh, 7, &flags);
  c->bottom = flags.bits_colour.selected;
  win_geticonflags(c->wh, 8, &flags);
  c->cells = flags.bits_colour.selected;

  if (c->safe) {
    strcpy(c->type == TEXTFILE ? c->cur->textname : c->cur->drawname,
           newname);
  }

  /* invoke relevant save routine */
  fp = fopen(newname, c->type == TEXTFILE ? "w" : "wb");
  if (!fp) return false;
  if (c->type == TEXTFILE)
    rc = nonowin_savegrid_text(c->cur, fp);
  else
    rc = nonowin_savegrid_draw(c->cur, fp);
  fclose(fp);
  if (rc == 0) {
    if (c->type == TEXTFILE) {
      sprintf(command, "settype %s Text", newname);
    } else {
      sprintf(command, "settype %s DrawFile", newname);
    }
    rc = system(command);
    if (rc)
      task_error(false, "External type-setting command returned %d: \"%s\"",
                 rc, command);
  }

  if (!c->keep) {
    menu_close();
  } else {
    nonowin_resetsave(c);
  }
  return true;
}

int nonowin_deposit(void *vc, const char *n, int safe)
{
  nonowin_savectxt *c = vc;
  c->safe = safe;
  return nonowin_saveokay(c, n);
}

void nonowin_savedrag(void *vc, const vdu_box *a)
{
  nonowin_savectxt *c = vc;
  wimp_mouseevent me;
  char newname[300];
  _kernel_oserror *oe;
  const char *leafname;

  win_getfield(c->wh, 2, newname, sizeof newname);
  leafname = get_leafname(newname);
  if ((oe = wimp_getpointerinfo(&me)) != NULL) {
    task_complain(oe);
    return;
  }

  transmit_open(&c->trc, leafname, 100, c->type == TEXTFILE ? 0xfff : 0xaff,
                &me.mouse.pos, me.wh, me.ih, c,
                &nonowin_deposit, NULL);
}

event_rspcode nonowin_clicksave(void *vc, const mouse_state *ms,
                                wimp_iconhandle ih)
{
  nonowin_savectxt *c = vc;
  if (ih == 0 /* OK */) {
    char newname[300];
    win_getfield(c->wh, 2, newname, sizeof(newname));
    c->keep = ms->buttons.bits.adjust0;
    c->safe = true;
    nonowin_saveokay(c, newname);
    return event_CLAIM;
  } else if (ih == 3 /* file icon */  &&
             (ms->buttons.bits.select1 || ms->buttons.bits.adjust1)) {
    c->keep = ms->buttons.bits.adjust1;
    /* start drag */
    win_dragiconpos(c->wh, &nonowin_savedrag, c, 3);
  }
  return event_SHIRK;
}

event_rspcode nonowin_keysave(void *vc, const wimp_caretpos *cb,
                              wimp_iconhandle ih, wimp_keycode k)
{
  nonowin_savectxt *c = vc;
  if (ih == 2 && (k == kbd_KEYPAD_ENTER || k == kbd_RETURN)) {
    char newname[300];
    win_getfield(c->wh, 2, newname, sizeof(newname));
    c->keep = false;
    c->safe = true;
    nonowin_saveokay(c, newname);
    return event_CLAIM;
  }
  return event_SHIRK;
}

static int wrap_task_error(void *v, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  task_error_va(false, fmt, ap);
  va_end(ap);
  return 0;
}

int nonowin_loadpuzzle(nonowin w, FILE *fp)
{
  size_t width, height;
  int bal;
  nonowin_unload(w);
  nonowin_setfilename(w, "<Untitled>");

  if (nonogram_fscanpuzzle_ef(&w->puzzle, fp, &wrap_task_error, w) < 0) {
    nonowin_resize(w);
    return -1;
  }
  /* do a balance check */
  bal = nonogram_verifypuzzle(&w->puzzle);
  if (bal > 0)
    task_error(0, "%s%d%s", label_default(nonowin_label, "em_premorerow",
                                          "Row excess: "),
               bal, label_default(nonowin_label, "em_postmorerow", ""));
  else if (bal < 0)
    task_error(0, "%s%d%s", label_default(nonowin_label, "em_premorecol",
                                          "Column excess: "),
               -bal, label_default(nonowin_label, "em_postmorecol", ""));

  w->data_in = true;
  w->counting = false;
  width = nonogram_puzzlewidth(&w->puzzle);
  height = nonogram_puzzleheight(&w->puzzle);
  w->grid = nonogram_makegrid(width, height);
  nonogram_cleargrid(w->grid, width, height);
  nonogram_unload(&w->solver);
  nonogram_load(&w->solver, &w->puzzle, w->grid, width * height);

  w->solutions = 0;
  w->finished = false;
  w->fresh = true;
  nonowin_okay(w, false);
  nonowin_updatesolutions(w);
  nonowin_updatestatus(w);
  nonowin_resize(w);
  return 0;
}

int nonowin_loadpuzzlefile(nonowin w, file_type type, const char *name)
{
  FILE *fp;

  if ((type & 0xfff) != 0xfff) return false;

  fp = fopen(name, "r");
  if (!fp) return false;

  if (nonowin_loadpuzzle(w, fp) < 0)
    return false;
  fclose(fp);
  nonowin_setfilename(w, name);
  return true;
}

int nonowin_savegrid_text(nonowin w, FILE *fp)
{
  size_t x, y, width, height;
  if (!w->data_in)
    return -1;

  width = nonogram_puzzlewidth(&w->puzzle);
  height = nonogram_puzzleheight(&w->puzzle);
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      switch (w->grid[x + y * width]) {
      case nonogram_BLANK:
        fputc(' ', fp);
        break;
      case nonogram_DOT:
        putc('-', fp);
        break;
      case nonogram_SOLID:
        putc('#', fp);
        break;
      default:
        putc('\?', fp);
        break;
      }
    putc('\n', fp);
  }
  return 0;
}

#ifndef PATH_PER_SOLID
struct griddata {
  const nonogram_cell *d;
  size_t w, h;
};

static int testedge(void *vg,
                    const struct edges_point *a,
                    const struct edges_point *b)
{
  struct griddata *g = vg;
  nonogram_cell ac =
    a->x < 0 || a->y < 0 || (size_t) a->x >= g->w || (size_t) a->y >= g->h ?
    nonogram_BLANK : g->d[a->x + a->y * g->w];
  nonogram_cell bc =
    b->x < 0 || b->y < 0 || (size_t) b->x >= g->w || (size_t) b->y >= g->h ?
    nonogram_BLANK : g->d[b->x + b->y * g->w];

  ac = ac == nonogram_SOLID;
  bc = bc == nonogram_SOLID;
  return ac - bc;
}


struct pdisp {
  vector_pathobj *vp;
  unsigned open : 1;
  size_t height;
};

static void linemove(void *vd, const struct edges_point *p)
{
  struct pdisp *d = vd;
  if (d->open) {
    d->open = 0;
    vector_path_close(d->vp);
  }
  vector_path_move(d->vp,
                   vector_makeord(4 + 4 * p->x, vector_MM),
                   vector_makeord(4 + 4 * (d->height - p->y),
                                  vector_MM));
}

static void linedraw(void *vd, const struct edges_point *p)
{
  struct pdisp *d = vd;
  vector_path_draw(d->vp,
                   vector_makeord(4 + 4 * p->x, vector_MM),
                   vector_makeord(4 + 4 * (d->height - p->y),
                                  vector_MM));
  d->open = 1;
}

static const struct edges_plotter lineplotter =
  { &linemove, &linedraw };
#endif

int nonowin_savegrid_draw(nonowin w, FILE *fp)
{
  size_t width, height;
  char ruletext[100];

  size_t x, y;

  vector_fileobj *vf;
  vector_pathobj *vp;
  vector_textobj *vt;
  vector_groupobj *vg, *vog;
  vector_box box;

  if (!w->data_in) return -1;

  width = nonogram_puzzlewidth(&w->puzzle);
  height = nonogram_puzzleheight(&w->puzzle);
  (void) vdu_hourglasson();

  (void) wimp_preparecommandwindow("Generation");

  vf = vector_file_create();
  if (!vf) goto error;

  vog = vector_group_create();
  if (!vog) goto error;
  vector_append((vector_obj *) vog, (vector_obj *) vf);

  /* create grid */
  vp = vector_path_create();
  if (!vp) goto error;
  vector_append((vector_obj *) vp, (vector_obj *) vog);
  vector_path_setlinecolour(vp, vdu_PALENTRY(0, 0, 0));
  vector_path_setfillcolour(vp, drawfile_TRANSPARENT);
  vector_path_setwidth(vp, vector_makeord(0.25, vector_POINT));
  vector_path_setstartcap(vp, vector_CSQUARE);
  vector_path_setendcap(vp, vector_CSQUARE);
  for (x = 0; x <= width; x++) {
    box.min.x = vector_makeord(4 + 4 * x, vector_MM);
    box.max.x = vector_makeord(8 + 4 * x, vector_MM);
    vector_path_move(vp, vector_makeord(4 + 4 * x, vector_MM),
                     vector_makeord(4, vector_MM));
    vector_path_draw(vp, vector_makeord(4 + 4 * x, vector_MM),
                     vector_makeord(4 + 4 * height, vector_MM));

    if (x < width) {
      const nonogram_sizetype *ruledata = nonogram_getcoldata(&w->puzzle, x);
      size_t rulelen = nonogram_getcollen(&w->puzzle, x);

      /* put column numbers into group vg (one per column), and
         place each vg in vog */
      if (nonowin_savebox.bottom) {
        box.max.y = vector_makeord(3, vector_MM);
        vg = vector_group_create();
        if (!vg) goto error;
        vector_append((vector_obj *) vg, (vector_obj *) vog);
        if (rulelen == 0) {
          vt = vector_text_create();
          if (!vt) goto error;
          vector_append((vector_obj *) vt, (vector_obj *) vg);
          vector_text_settext(vt, "0");
          vector_text_setfont(vt, "Homerton.Medium");
          vector_text_setscale(vt, vector_makeord(3, vector_MM),
                               vector_makeord(3, vector_MM));
          vector_text_setinarea(vt, &box, vector_CENTRE | vector_TOP);
        } else {
          size_t i;
          for (i = 0; i < rulelen; i++) {
            sprintf(ruletext, "%" nonogram_PRIuSIZE, ruledata[i]);
            vt = vector_text_create();
            if (!vt) goto error;
            vector_append((vector_obj *) vt, (vector_obj *) vg);
            vector_text_settext(vt, ruletext);
            vector_text_setfont(vt, "Homerton.Medium");
            vector_text_setscale(vt, vector_makeord(3, vector_MM),
                                 vector_makeord(3, vector_MM));
            vector_text_setinarea(vt, &box, vector_CENTRE | vector_TOP);
            box.max.y -= vector_makeord(4, vector_MM);
          }
        }
      } /* if bottom flag */

      if (nonowin_savebox.top) {
        box.min.y = vector_makeord(5 + 4 * height, vector_MM);
        vg = vector_group_create();
        if (!vg) goto error;
        vector_append((vector_obj *) vg, (vector_obj *) vog);
        if (rulelen == 0) {
          vt = vector_text_create();
          if (!vt) goto error;
          vector_append((vector_obj *) vt, (vector_obj *) vg);
          vector_text_settext(vt, "0");
          vector_text_setfont(vt, "Homerton.Medium");
          vector_text_setscale(vt, vector_makeord(3, vector_MM),
                               vector_makeord(3, vector_MM));
          vector_text_setinarea(vt, &box, vector_CENTRE | vector_BOTTOM);
        } else {
          int i;
          for (i = rulelen; i > 0; i--) {
            sprintf(ruletext, "%" nonogram_PRIuSIZE, ruledata[i - 1]);
            vt = vector_text_create();
            if (!vt) goto error;
            vector_append((vector_obj *) vt, (vector_obj *) vg);
            vector_text_settext(vt, ruletext);
            vector_text_setfont(vt, "Homerton.Medium");
            vector_text_setscale(vt, vector_makeord(3, vector_MM),
                                 vector_makeord(3, vector_MM));
            vector_text_setinarea(vt, &box, vector_CENTRE | vector_BOTTOM);
            box.min.y += vector_makeord(4, vector_MM);
          }
        }
      } /* if top flag */
    }
  } /* per column separator */

  for (y = 0; y <= height; y++) {
    vector_path_move(vp, vector_makeord(4, vector_MM),
                     vector_makeord(4 + 4 * y, vector_MM));
    vector_path_draw(vp, vector_makeord(4 + 4 * width, vector_MM),
                     vector_makeord(4 + 4 * y, vector_MM));
    if (y < height) {
      const nonogram_sizetype *ruledata =
        nonogram_getrowdata(&w->puzzle, height - y - 1);
      size_t rulelen =
        nonogram_getrowlen(&w->puzzle, height - y - 1);
#if false
      nonogram_rule *rule = &w->puzzle.row[height - y - 1];
#endif
      box.min.y = vector_makeord(4 + 4 * y, vector_MM);
      box.max.y = vector_makeord(8 + 4 * y, vector_MM);

      /* generate text string to be printed */
      if (rulelen == 0) {
        strcpy(ruletext, "0");
      } else {
        size_t i;
        char *textptr = ruletext;
        *textptr = '\0';
        for (i = 0; i < rulelen; i++) {
          sprintf(textptr, "%s%" nonogram_PRIuSIZE,
                  (i == 0 ? "" : "."), ruledata[i]);
          while (*textptr)
            textptr++;
        }
      }

      if (nonowin_savebox.right) {
        box.min.x = vector_makeord(5 + 4 * width, vector_MM);
        vt = vector_text_create();
        if (!vt) goto error;
        vector_append((vector_obj *) vt, (vector_obj *) vog);
        vector_text_settext(vt, ruletext);
        vector_text_setfont(vt, "Homerton.Medium");
        vector_text_setscale(vt, vector_makeord(3, vector_MM),
                             vector_makeord(3, vector_MM));
        vector_text_setinarea(vt, &box, vector_LEFT | vector_MIDDLE);
      } /* if right flag */

      if (nonowin_savebox.left) {
        box.max.x = vector_makeord(3, vector_MM);
        vt = vector_text_create();
        if (!vt) goto error;
        vector_append((vector_obj *) vt, (vector_obj *) vog);
        vector_text_settext(vt, ruletext);
        vector_text_setfont(vt, "Homerton.Medium");
        vector_text_setscale(vt, vector_makeord(3, vector_MM),
                             vector_makeord(3, vector_MM));
        vector_text_setinarea(vt, &box, vector_RIGHT | vector_MIDDLE);
      } /* if left flag */
    }
  }

  if (height > 5 || width > 5) {
    vp = vector_path_create();
    if (!vp) goto error;
    vector_append((vector_obj *) vp, (vector_obj *) vog);
    vector_path_setlinecolour(vp, vdu_PALENTRY(0, 0, 0));
    vector_path_setfillcolour(vp, drawfile_TRANSPARENT);
    vector_path_setwidth(vp, vector_makeord(1.0, vector_POINT));
    vector_path_setstartcap(vp, vector_CBUTT);
    vector_path_setendcap(vp, vector_CBUTT);
    for (x = 5; x < width; x += 5) {
      vector_path_move(vp, vector_makeord(4 + 4 * x, vector_MM),
                       vector_makeord(4, vector_MM));
      vector_path_draw(vp, vector_makeord(4 + 4 * x, vector_MM),
                       vector_makeord(4 + 4 * height, vector_MM));
    }
    for (y = 5; y < height; y += 5) {
      vector_path_move(vp, vector_makeord(4, vector_MM),
                       vector_makeord(4 + 4 * y, vector_MM));
      vector_path_draw(vp, vector_makeord(4 + 4 * width, vector_MM),
                       vector_makeord(4 + 4 * y, vector_MM));
    }
  }

  if (nonowin_savebox.cells) {
#ifndef PATH_PER_SOLID
    edges_workcell *ws;
#endif

    /* draw elements */
    vg = vector_group_create();
    if (!vg) goto error;
    vector_append((vector_obj *) vg, (vector_obj *) vog);

#if 1

#ifndef PATH_PER_SOLID
    vp = vector_path_create();
    if (!vp) goto error;
    vector_append((vector_obj *) vp, (vector_obj *) vg);
    vector_path_setlinecolour(vp, drawfile_TRANSPARENT);
    vector_path_setfillcolour(vp, vdu_PALENTRY(0, 0, 0));
    vector_path_setwidth(vp, 0);
#endif

    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++)
        switch (w->grid[x + (height - y - 1) * width]) {
#ifdef PATH_PER_SOLID
        case nonogram_SOLID:
          vp = vector_path_create();
          if (!vp) goto error;
          vector_append((vector_obj *) vp, (vector_obj *) vg);
          vector_path_setlinecolour(vp, drawfile_TRANSPARENT);
          vector_path_setfillcolour(vp, vdu_PALENTRY(0, 0, 0));
          vector_path_setwidth(vp, 0);
          vector_path_move(vp, vector_makeord(4 + 4 * x, vector_MM),
                           vector_makeord(4 + 4 * y, vector_MM));
          vector_path_draw(vp, vector_makeord(4 + 4 * (x + 1), vector_MM),
                           vector_makeord(4 + 4 * y, vector_MM));
          vector_path_draw(vp, vector_makeord(4 + 4 * (x + 1), vector_MM),
                           vector_makeord(4 + 4 * (y + 1), vector_MM));
          vector_path_draw(vp, vector_makeord(4 + 4 * x, vector_MM),
                           vector_makeord(4 + 4 * (y + 1), vector_MM));
          vector_path_draw(vp, vector_makeord(4 + 4 * x, vector_MM),
                           vector_makeord(4 + 4 * y, vector_MM));
          vector_path_close(vp);
          break;
#endif

        case nonogram_DOT:
#ifdef PATH_PER_SOLID
          vp = vector_path_create();
          if (!vp) goto error;
          vector_append((vector_obj *) vp, (vector_obj *) vg);
          vector_path_setlinecolour(vp, drawfile_TRANSPARENT);
          vector_path_setfillcolour(vp, vdu_PALENTRY(0, 0, 0));
          vector_path_setwidth(vp, 0);
#endif
          vector_path_move(vp, vector_makeord(5.5 + 4 * x, vector_MM),
                           vector_makeord(6 + 4 * y, vector_MM));
          vector_path_arc(vp, vector_makeord(6 + 4 * x, vector_MM),
                          vector_makeord(6 + 4 * y, vector_MM),
                          vector_makeord(90, vector_DEGREE));
          vector_path_arc(vp, vector_makeord(6 + 4 * x, vector_MM),
                          vector_makeord(6 + 4 * y, vector_MM),
                          vector_makeord(90, vector_DEGREE));
          vector_path_arc(vp, vector_makeord(6 + 4 * x, vector_MM),
                          vector_makeord(6 + 4 * y, vector_MM),
                          vector_makeord(90, vector_DEGREE));
          vector_path_arc(vp, vector_makeord(6 + 4 * x, vector_MM),
                          vector_makeord(6 + 4 * y, vector_MM),
                          vector_makeord(90, vector_DEGREE));
          vector_path_close(vp);
          break;
        }

#endif

#if !defined PATH_PER_SOLID
    vp = vector_path_create();
    if (!vp) goto error;
    vector_append((vector_obj *) vp, (vector_obj *) vg);

    ws = malloc(edges_size(width, height) * sizeof *ws);
    if (ws) {
      struct pdisp pdisp;
      struct griddata gdat;

      vector_path_setlinecolour(vp, drawfile_TRANSPARENT);
      vector_path_setfillcolour(vp, vdu_PALENTRY(0, 0, 0));
      vector_path_setwidth(vp, 0);
      vector_path_setwind(vp, draw_FWINDEO);

      gdat.d = w->grid;
      gdat.w = width;
      gdat.h = height;
      pdisp.open = 0;
      pdisp.vp = vp;
      pdisp.height = height;
#if 1
      edges_plot(width, height, &testedge, &gdat, &lineplotter, &pdisp, 0, ws);
#elif 0
      nonogram_plotedges_grid(width, height, w->grid,
                              nonogram_SOLID, &lineplotter, &pdisp, 0, ws);
#else
      vector_path_move(vp, vector_makeord(10, vector_MM),
                       vector_makeord(10, vector_MM));
      vector_path_draw(vp, vector_makeord(10, vector_MM),
                       vector_makeord(0, vector_MM));
      vector_path_draw(vp, vector_makeord(0, vector_MM),
                       vector_makeord(10, vector_MM));
      vector_path_draw(vp, vector_makeord(0, vector_MM),
                       vector_makeord(0, vector_MM));
      vector_path_draw(vp, vector_makeord(10, vector_MM),
                       vector_makeord(10, vector_MM));
#endif
      free(ws);
      vector_path_close(vp);
    } else {
      task_error(false, "Out of memory for plotting efficient path.");
      goto tidy;
    }
#endif
  } /* if cells flag */

  vector_getbox((vector_obj *) vog, &box, 1);
  box.min.x -= vector_makeord(4, vector_MM);
  box.min.y -= vector_makeord(4, vector_MM);
  box.max.x += vector_makeord(4, vector_MM);
  box.max.y += vector_makeord(4, vector_MM);
  vp = vector_path_create();
  if (!vp) goto error;
  vector_prepend((vector_obj *) vp, (vector_obj *) vog);
  vector_path_setlinecolour(vp, drawfile_TRANSPARENT);
  vector_path_setfillcolour(vp, vdu_PALENTRY(255, 255, 255));
  vector_path_setwidth(vp, 0);
  vector_path_move(vp, box.min.x, box.min.y);
  vector_path_draw(vp, box.max.x, box.min.y);
  vector_path_draw(vp, box.max.x, box.max.y);
  vector_path_draw(vp, box.min.x, box.max.y);
  vector_path_draw(vp, box.min.x, box.min.y);
  vector_path_close(vp);

  /* align bottom-left to origin */
  box.min.x = box.min.y = 0;
  vector_align((vector_obj *) vog, &box, vector_LEFT | vector_BOTTOM);

  vector_file_print(vf, fp);

  vector_destroy((vector_obj *) vf);

  (void) wimp_endcommandwindow();

  (void) vdu_hourglassoff();
  return 0;

 error:
  task_error(false, "Out of memory generating Drawfile.");
 tidy:
  vector_destroy((vector_obj *) vf);
  return -1;
}

int nonowin_load(void *vw, wimp_iconhandle ih, const vdu_point *pos,
                 signed long sz, file_type type, const char *name)
{
  nonowin w = vw;
  return nonowin_loadpuzzlefile(w, type, name);
}
