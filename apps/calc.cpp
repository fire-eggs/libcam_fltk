//
// Created by kevin on 2/16/22.
//

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Choice.H>
#include <cmath>
#include "calc.h"

static int FPS_VALS[] = {15, 24, 25, 30, 60};

// TODO duplicated here and timelapse.cpp
static double intervalToMilliseconds(int intervalType)
{
    // TODO turn into an array like zoomVals ?
    // TODO need manifest constants for the 'type'

    switch (intervalType)
    {
        /* 20220211 milliseconds no longer an option
        case 0:
            return 1;
            */
        default:
        case 0:
            return 1000; // seconds
        case 1:
            return 60 * 1000; // minutes
        case 2:
            return 60 * 60 * 1000; // hours
    }
    return -1;
}

void CalcWin::recalc()
{
    double vidlen = m_spinVidLen->value();
    int fpsChoice = m_cmbFPS->value();
    int fpsVal = FPS_VALS[fpsChoice];

    m_framecount = lround(vidlen * fpsVal);
    char buff[100];
    sprintf(buff, "%ld", m_framecount);
    m_resFrameCount->value(buff);

    int lenType = m_cmbRunLenType->value();
    double lenval = m_spinRecLen->value();
    double recLimit = lenval * intervalToMilliseconds(lenType);

    long interval = lround(recLimit / m_framecount);
    sprintf(buff, "%ld", interval);
    m_resInterval->value(buff);

    m_intervalSeconds = recLimit / m_framecount / 1000.0;
}

static void onClose(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->hide();
}

static void onChange(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->recalc();
}

static void onReset(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);

    // TODO change to an invocation of cw->reset()
    cw->m_spinVidLen->value(60);
    cw->m_cmbFPS->value(0);
    cw->m_spinRecLen->value(1);
    cw->m_cmbRunLenType->value(2);
    cw->recalc();
}

static void onUse(Fl_Widget *w, void *d)
{
    CalcWin *cw = static_cast<CalcWin *>(d);
    cw->updateTLControls();
}

void CalcWin::updateTLControls()
{
    _destFrameCount->value(m_framecount);
    _destRadio1->value(true);
    _destRadio2->value(false);
    _destIntervalSpin->value(m_intervalSeconds);
    _destIntervalType->value(0);
}

void CalcWin::ControlsToUpdate(Fl_Spinner *c1, Fl_Round_Button*c2, Fl_Spinner*c3, Fl_Choice*c4,Fl_Round_Button*c5)
{
    _destFrameCount = c1;
    _destRadio1 = c2;
    _destIntervalSpin = c3;
    _destIntervalType = c4;
    _destRadio2 = c5;
}


CalcWin::CalcWin(int w, int h, const char *l, Prefs *prefs)
    : Fl_Double_Window(w, h, l)
{
    // container
    Fl_Flex *col0 = new Fl_Flex(10,5,w-15, h-10,Fl_Flex::COLUMN);
    {
        Fl_Box* pad1 = new Fl_Box(0, 0, 0, 0, "");

        Fl_Flex *row1 = new Fl_Flex(Fl_Flex::ROW);
        {
            Fl_Box *lbl2 = new Fl_Box(0,0,0,0,"1. Length of final video:");

            m_spinVidLen = new Fl_Spinner(0, 0, 0, 0);
            m_spinVidLen->type(1); // FL_FLOAT
            m_spinVidLen->minimum(0.5);
            m_spinVidLen->maximum(500);
            m_spinVidLen->step(0.5);
            m_spinVidLen->value(60); // TODO from prefs
            m_spinVidLen->callback(onChange, this);
            m_spinVidLen->when(FL_WHEN_CHANGED);

            Fl_Box *lbl3 = new Fl_Box(0, 0, 0, 0, "Seconds");
            lbl3->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_LEFT);
            Fl_Box *lbl4 = new Fl_Box(0, 0, 0, 0);

            row1->setSize(m_spinVidLen, 100);

            row1->end();
        }

        Fl_Box* pad2 = new Fl_Box(0, 0, 0, 0, "");

        Fl_Flex *row2 = new Fl_Flex(Fl_Flex::ROW);
        {
            Fl_Box *b = new Fl_Box(0,0,0,0,"2. Select Video FPS:");

            m_cmbFPS = new Fl_Choice(0,0,0,0);
            m_cmbFPS->align(Fl_Align(FL_ALIGN_LEFT));
            m_cmbFPS->down_box(FL_BORDER_BOX);
            m_cmbFPS->menu(menu_cmbFPS);
            m_cmbFPS->value(0); // TODO from prefs
            m_cmbFPS->callback(onChange, this);
            m_cmbFPS->when(FL_WHEN_CHANGED);

            Fl_Input *b2 = new Fl_Input(0,0,0,0);
            b2->value("Custom");

            row2->end();
            row1->setSize(b2, 100);
        }

        Fl_Box* pad3 = new Fl_Box(0, 0, 0, 0, "");

        Fl_Flex *row3 = new Fl_Flex(Fl_Flex::ROW);
        Fl_Box *lbl1 = new Fl_Box(0,0,0,0, "Resulting Frame Count:");
        m_resFrameCount = new Fl_Input(0, 0, 0, 0);
        m_resFrameCount->value("filler");
        m_resFrameCount->readonly(true);
        row3->end();

        Fl_Box* line1 = new Fl_Box(0,0,0,0);
        line1->box(FL_BORDER_BOX);

        Fl_Box* pad4 = new Fl_Box(0, 0, 0, 0, "");

        col0->setSize(pad1, 5);
        col0->setSize(row1, 30);
        col0->setSize(pad2, 5);
        col0->setSize(row2, 30);
        col0->setSize(pad3, 10);
        col0->setSize(row3, 30);
        col0->setSize(line1, 3);
        col0->setSize(pad4, 30);


        Fl_Flex *row4 = new Fl_Flex(Fl_Flex::ROW);

        m_spinRecLen = new Fl_Spinner(0, 0, 0, 0);
        m_spinRecLen->label("3. How long will it take to record the timelapse:");
        m_spinRecLen->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        m_spinRecLen->type(1); // FL_FLOAT
        m_spinRecLen->minimum(0.5);
        m_spinRecLen->maximum(500);
        m_spinRecLen->step(0.5);
        m_spinRecLen->value(1); // TODO from prefs
        m_spinRecLen->callback(onChange, this);
        m_spinRecLen->when(FL_WHEN_CHANGED);

        m_cmbRunLenType = new Fl_Choice(0,0,0,0);
        m_cmbRunLenType->down_box(FL_BORDER_BOX);
        m_cmbRunLenType->menu(menu_cmbRunLenType);
        m_cmbRunLenType->value(2); // TODO from prefs
        m_cmbRunLenType->callback(onChange, this);
        m_cmbRunLenType->when(FL_WHEN_CHANGED);

        row4->end();

        Fl_Box* pad5 = new Fl_Box(0, 0, 0, 0, "");

        Fl_Flex *row5 = new Fl_Flex(Fl_Flex::ROW);
        Fl_Box *lbl5 = new Fl_Box(0,0,0,0, "Resulting Interval:");
        m_resInterval = new Fl_Input(0, 0, 0, 0);
        m_resInterval->value("filler");
        m_resInterval->readonly(true);
        Fl_Box *lbl7 = new Fl_Box(0,0,0,0,"Milliseconds");
        row3->end();

        Fl_Box* line2 = new Fl_Box(0,0,0,0);
        line2->box(FL_BORDER_BOX);

        Fl_Box* pad6 = new Fl_Box(0, 0, 0, 0, "");

        Fl_Flex *row6 = new Fl_Flex(Fl_Flex::ROW);

        Fl_Button *btnUse = new Fl_Button(0,0,0,0,"Use these settings");
        btnUse->callback(onUse, this);
        Fl_Button *btnReset = new Fl_Button(0,0,0,0,"Reset");
        btnReset->callback(onReset, this);
        Fl_Button *btnClose = new Fl_Button(0,0,0,0,"Close");
        btnClose->callback(onClose, this);

        row6->end();

        col0->setSize(row4, 30);
        col0->setSize(pad5, 20);
        col0->setSize(row5, 30);
        col0->setSize(line2, 3);
        col0->setSize(pad6, 20);
        col0->setSize(row6, 30);

        col0->end();
    }

    resizable(col0);
    end();

    recalc();
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