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

#ifndef NONOWIN_H
#define NONOWIN_H

#include <stdbool.h>

#include <stdio.h>

#include <riscos/types.h>

#include <yacitros/desktop/types.h>

#include <nonogram.h>

typedef struct nonowin_win *nonowin;
typedef struct nonowin_save nonowin_savectxt;
typedef struct nonowin_info nonowin_infoctxt;

void nonowin_init(label_bundle *);
void nonowin_term(void);

nonowin nonowin_create(void);
void nonowin_destroy(nonowin w);

int nonowin_loadpuzzle(nonowin w, FILE *fp);
void nonowin_discard(nonowin w);
int nonowin_savegrid_text(nonowin w, FILE *fp);
int nonowin_savegrid_draw(nonowin w, FILE *fp);
void nonowin_idle(void *);
void nonowin_restart(nonowin w);
void nonowin_halt(nonowin w);
void nonowin_clear(nonowin w);
void nonowin_setfilename(nonowin w, const char *);
int nonowin_loadpuzzlefile(nonowin w, file_type type, const char *name);

#endif
