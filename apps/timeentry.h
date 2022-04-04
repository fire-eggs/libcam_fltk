/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The time-entry widget.
 */

#ifndef _TIMEENTRY_H
#define _TIMEENTRY_H

#include <FL/Fl_Pack.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Box.H>

class TimeEntry : public Fl_Pack
{
public:
    TimeEntry(int x, int y, int w, int h, const char *L=nullptr);
    void value(int hr, int min, int sec);
    void value(unsigned long seconds);
    void getValue(int &hr, int &min, int& sec);
    unsigned long getSeconds();

private:
    Fl_Spinner *hs; // TODO consider using HackSpin instead?
    Fl_Spinner *ms;
    Fl_Spinner *ss;

    static void te_cb(Fl_Widget *, void *); // internal callback
};


#endif //_TIMEENTRY_H
