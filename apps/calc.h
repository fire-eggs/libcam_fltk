//
// Created by kevin on 2/16/22.
//

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
    Prefs *m_prefs;

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

public:
    CalcWin(int w, int h, const char *l, Prefs *prefs);
    void showCalc();
    void ControlsToUpdate(Fl_Spinner *, Fl_Round_Button*,Fl_Spinner*,Fl_Choice*,Fl_Round_Button*);
    void updateTLControls();

    //Fl_Spinner *m_spinVidLen;


};

#endif //LIBCAMFLTK_CALC_H
