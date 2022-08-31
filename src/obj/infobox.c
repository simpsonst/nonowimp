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

#include <yacitros/desktop/win.h>
#include <yacitros/desktop/event.h>
#include <yacitros/desktop/task.h>

void nonowin_updatetitle(nonowin w)
{
  if (nonowin_infobox.cur != w) return;
  win_setfield(nonowin_infobox.wh, 4, w->filename);
}

void nonowin_updatestatus(nonowin w)
{
  if (nonowin_infobox.cur != w) return;
  if (w->finished)
    win_setfield(nonowin_infobox.wh, 2, "Finished");
  else if (w->ph)
    win_setfield(nonowin_infobox.wh, 2, "Running");
  else
    win_setfield(nonowin_infobox.wh, 2, "Paused");
}

void nonowin_updatesolutions(nonowin w)
{
  if (nonowin_infobox.cur != w) return;
  win_setfield_int(nonowin_infobox.wh, 0, w->solutions);
}

event_rspcode nonowin_openinfo(void *vc, const wimp_winloc *loc)
{
  nonowin_infoctxt *c = vc;
  if (win_openat(c->wh, loc) < 0)
    task_complain(task_err());
  return event_CLAIM;
}

event_rspcode nonowin_closeinfo(void *vc)
{
  nonowin_infoctxt *c = vc;
  win_close(c->wh);
  return event_CLAIM;
}
