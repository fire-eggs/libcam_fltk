/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The time-entry widget. A custom FLTK widget to enter time in HR:MIN:SEC.
 */

#include "timeentry.h"

TimeEntry::TimeEntry(int x, int y, int w, int h, const char *L) :
        Fl_Pack(x,y,w,h,L)
{
    type(Fl_Pack::HORIZONTAL);
    spacing(2);

    hs = new Fl_Spinner(0,0,45,25);
    hs->type(FL_INT_INPUT);
    hs->range(0,99);
    hs->wrap(1);
    hs->format("%02.f");
    hs->value(0);
    hs->callback(te_cb, this);

    Fl_Box *b1 = new Fl_Box(0,0,3,25);
    b1->labelfont(FL_BOLD);
    b1->label(":");

    ms = new Fl_Spinner(0,0,45,0);
    ms->type(FL_INT_INPUT);
    ms->range(0,60);
    ms->wrap(1);   // TODO on wrap, increment hours?
    ms->format("%02.f");
    ms->value(1);
    ms->callback(te_cb, this);

    Fl_Box *b2 = new Fl_Box(0,0,3,0);
    b2->labelfont(FL_BOLD);
    b2->label(":");

    ss = new Fl_Spinner(0,0,45,0);
    ss->type(FL_INT_INPUT);
    ss->range(0,59);
    ss->wrap(1);  // TODO on wrap, increment minutes?
    ss->format("%02.f");
    ss->value(0);
    ss->callback((Fl_Callback *)te_cb, this);

    end();
}

void TimeEntry::value(int hr, int min, int sec)
{
    hs->value(hr);
    ms->value(min);
    ss->value(sec);
}

void TimeEntry::getValue(int &hr, int &min, int& sec)
{
    hr = static_cast<int>(hs->value());
    min = static_cast<int>(ms->value());
    sec = static_cast<int>(ss->value());
}

unsigned long TimeEntry::getSeconds()
{
    int h,m,s;
    getValue(h,m,s);
    return h * 3600 + m * 60 + s;
}

void TimeEntry::value(unsigned long seconds)
{
    int hr = seconds / 3600;
    seconds -= hr * 3600;
    int min = seconds / 60;
    seconds -= min * 60;
    value(hr, min, seconds);
}

void TimeEntry::te_cb(Fl_Widget *, void *d)
{
    auto te = static_cast<TimeEntry *>(d);
    te->set_changed();
    te->do_callback();
}