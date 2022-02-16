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

static Fl_Menu_Item menu_cmbFPS[] =
{
{"15", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"24", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"25", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"30", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"Custom", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
};

static Fl_Menu_Item menu_cmbRunLenType[] =
{
{"Seconds", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
{"Minutes", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
{"Hours", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
{0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

class CalcWin : public Fl_Double_Window
{
private:
    Prefs *m_prefs;
    Fl_Input *m_resFrameCount;
    Fl_Input *m_resInterval;

    Fl_Spinner *_destFrameCount;
    Fl_Round_Button* _destRadio1;
    Fl_Round_Button* _destRadio2;
    Fl_Spinner *_destIntervalSpin;
    Fl_Choice *_destIntervalType;

    double m_intervalSeconds;
    int m_framecount;

public:
    CalcWin(int w, int h, const char *l, Prefs *prefs);
    void showCalc();
    void ControlsToUpdate(Fl_Spinner *, Fl_Round_Button*,Fl_Spinner*,Fl_Choice*,Fl_Round_Button*);
    void updateTLControls();

    Fl_Spinner *m_spinVidLen;
    Fl_Choice *m_cmbFPS;

    void recalc();

    Fl_Choice *m_cmbRunLenType;
    Fl_Spinner *m_spinRecLen;
};

#endif //LIBCAMFLTK_CALC_H
