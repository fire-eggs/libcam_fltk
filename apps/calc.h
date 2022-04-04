/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "timelapse calculator" dialog box. GUI definition, callback functions.
 */

#ifndef LIBCAMFLTK_CALC_H
#define LIBCAMFLTK_CALC_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Round_Button.H>

#include "prefs.h"
#include "FL_Flex/FL_Flex.H"
#include "timeentry.h"

class CalcWin : public Fl_Double_Window
{
private:
    void *_callbackData;
    Fl_Callback *_callback;

    TimeEntry *tlTime;  // length of timelapse
    TimeEntry *vidTime; // length of video
    Fl_Choice *m_cmbFPS; // choice of FPS
    Fl_Spinner *spinTLInterval; // interval in seconds
    Fl_Output *outFrameCount; // display of framecount

    static void onChange(Fl_Widget *, void *);
    static void onChangeInterval(Fl_Widget *, void *);
    static void onReset(Fl_Widget *, void *);
    void recalcInterval();
    void showFramecount(int value);

    static void onUse(Fl_Widget*, void *);
public:
    CalcWin(int w, int h, const char *l, Prefs *prefs);
    void showCalc();

    unsigned int getFrameCount();
    unsigned int getInterval();
    unsigned int getTimelapseLength();

    void onUseData(Fl_Callback *cb, void *d) {_callback = cb; _callbackData = d;}
};

#endif //LIBCAMFLTK_CALC_H
