//
// Created by kevin on 2/16/22.
//
#include <cmath>

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Output.H>
#include "calc.h"

class HackSpin : public Fl_Spinner
{
    // Modify the default callback logic of the Fl_Spinner so that the callback
    // will occur whenever the user types in the input field [not just waiting for
    // focus change].

public:
    HackSpin(int X, int Y, int W, int H, const char *L= nullptr)
    : Fl_Spinner(X,Y,W,H,L)
    {
        input_.when(FL_WHEN_CHANGED | FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
    }
};

static int FPS_VALS[] = {15, 24, 25, 30, 60};

static Fl_Menu_Item menu_cmbFPS[] =
{
{"15", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"24", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"25", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"30", 0, nullptr, nullptr, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

void CalcWin::recalcInterval()
{
    // vidlen, tllen, FPS changed: update interval, framecount

    int fpsChoice = m_cmbFPS->value();
    int fpsVal = FPS_VALS[fpsChoice];

    // At current FPS, video length -> how many frames?
    unsigned long vidlen = vidTime->getSeconds();
    int framecount = lround(vidlen * fpsVal);
    showFramecount(framecount);

    // At current frames, timelapse length -> resulting interval
    unsigned long tllen  = tlTime->getSeconds();
    double interval = (double)tllen / (double)framecount;
    spinTLInterval->value(interval);
}

static void onClose(Fl_Widget *, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->hide();
}

void CalcWin::onChange(Fl_Widget *, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->recalcInterval();
}

void CalcWin::showFramecount(int value)
{
    // Update the framecount display
    char buff[100];
    sprintf(buff, "%d", value);
    outFrameCount->value(buff);
}

void CalcWin::onChangeInterval(Fl_Widget *, void *d)
{
    // interval has been changed. Update vidTime

    CalcWin *cw = static_cast<CalcWin *>(d);

    double interval = cw->spinTLInterval->value();

    int fpsChoice = cw->m_cmbFPS->value();
    int fpsVal = FPS_VALS[fpsChoice];
    unsigned long tllen = cw->tlTime->getSeconds();

    int framecount = lround(tllen / interval );
    cw->showFramecount(framecount);

    unsigned long vidlen = lround(framecount / fpsVal);

    cw->vidTime->value(vidlen);
}

void CalcWin::onReset(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);

    cw->tlTime->value(1, 30, 0);
    cw->vidTime->value(0,2,0);
    cw->m_cmbFPS->value(0);
    cw->spinTLInterval->value(10);
    cw->recalcInterval();
}

static void onUse(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->updateTLControls();
}

void CalcWin::updateTLControls()
{
    // TODO callback and access methods
/*
    _destFrameCount->value(m_framecount);
    _destRadio1->value(true);
    _destRadio2->value(false);
    _destIntervalSpin->value(m_intervalSeconds);
    _destIntervalType->value(0);
*/
}

void CalcWin::ControlsToUpdate(Fl_Spinner *c1, Fl_Round_Button*c2, Fl_Spinner*c3, Fl_Choice*c4,Fl_Round_Button*c5)
{
    /*
    _destFrameCount = c1;
    _destRadio1 = c2;
    _destIntervalSpin = c3;
    _destIntervalType = c4;
    _destRadio2 = c5;
     */
}


CalcWin::CalcWin(int w, int h, const char *l, Prefs *prefs)
    : Fl_Double_Window(w, h, l)
{
#define CONTROLS_LEFT 200

    (new Fl_Box(5, 10, 125, 25, "Pre-Production"))->labelfont(FL_BOLD);

    tlTime = new TimeEntry(CONTROLS_LEFT, 45, 200, 30, "Length of the timelapse:");
    tlTime->value(1, 30, 0);
    tlTime->align(Fl_Align(FL_ALIGN_LEFT));
    tlTime->callback(onChange, this);

    {
        HackSpin *o = new HackSpin(CONTROLS_LEFT, 85, 80, 30);
        o->type(FL_FLOAT_INPUT);
        o->step(0.25);
        o->label("Timelapse interval:");
        o->align(Fl_Align(FL_ALIGN_LEFT));
        o->callback(onChangeInterval, this);
        spinTLInterval = o;
    }

    (new Fl_Box(CONTROLS_LEFT+85, 85, 70, 30, "Seconds"))->align(Fl_Align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT));

    {
        Fl_Box* o = new Fl_Box(15, 130, 350, 3);
        o->box(FL_BORDER_BOX);
        o->color(FL_FOREGROUND_COLOR);
    }

    {
        Fl_Box* o = new Fl_Box(5, 140, 150, 25, "Post-Production");
        o->labelfont(FL_BOLD);
        o->align(Fl_Align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT));
    }

    vidTime = new TimeEntry(CONTROLS_LEFT, 175, 200, 30);
    vidTime->label("Length of the final video:");
    vidTime->align(Fl_Align(FL_ALIGN_LEFT));
    vidTime->callback(onChange, this);

    m_cmbFPS = new Fl_Choice(CONTROLS_LEFT,215,75,30, "Video FPS:");
    m_cmbFPS->align(Fl_Align( FL_ALIGN_LEFT));
    m_cmbFPS->down_box(FL_BORDER_BOX);
    m_cmbFPS->menu(menu_cmbFPS);
    m_cmbFPS->callback(onChange, this);
    m_cmbFPS->when(FL_WHEN_CHANGED);

    outFrameCount = new Fl_Output(CONTROLS_LEFT, 255, 75, 30, "Frame Count:");
    outFrameCount->align(Fl_Align(FL_ALIGN_LEFT));

    {
        Fl_Box* o = new Fl_Box(15, 295, 350, 3);
        o->box(FL_BORDER_BOX);
        o->color(FL_FOREGROUND_COLOR);
    }

    Fl_Pack* pack1 = new Fl_Pack(50, 310, 350, 30);
    pack1->type(Fl_Pack::HORIZONTAL);
    pack1->spacing(10);
    {
        Fl_Button *btnUse = new Fl_Button(0, 0, 150, 0, "Use these settings");
        btnUse->callback(onUse, this);
        Fl_Button *btnReset = new Fl_Button(0, 0, 75, 0, "Reset");
        btnReset->callback(onReset, this);
        Fl_Button *btnClose = new Fl_Button(0, 0, 75, 0, "Close");
        btnClose->callback(onClose, this);
    }
    pack1->end();

    // TODO from prefs?
    onReset(nullptr, this);  // initialize
}

void CalcWin::showCalc()
{
    show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(nullptr);

    while (shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
}
