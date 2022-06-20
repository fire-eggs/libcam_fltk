/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "advanced" dialog box. GUI definition, callback functions, and inter-thread communication.
 */

#include <iostream>

#include <libcamera/control_ids.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Box.H>

#include "prefs.h"
extern Prefs *_prefs;

extern bool stateChange;

// The AWB settings for camThread
int32_t _awb_index;
float   _awb_gain_r;
float   _awb_gain_b;
//TODO float   _shutter;

Fl_Double_Window *advanced;

static void cbClose(Fl_Widget *, void *);

int awb_table[] = {
    
    libcamera::controls::AwbAuto, 
    libcamera::controls::AwbCloudy, 
    libcamera::controls::AwbDaylight,
    libcamera::controls::AwbFluorescent,
    libcamera::controls::AwbIncandescent,
    libcamera::controls::AwbIndoor,
    libcamera::controls::AwbTungsten

};

static Fl_Menu_Item menu_cmbFormat[] =
{
    {"auto", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"cloudy", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"daylight", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"fluorescent", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"incandescent", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"indoor", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"tungsten", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

static void onAWBMode(Fl_Widget *w, void *)
{
    // AWB Mode choice change
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    
    Prefs *setP = _prefs->getSubPrefs("advanced");
    setP->set("awbIndex", val);
    
    _awb_index = awb_table[val];
    stateChange = true;
}

static void cb_GainsB(Fl_Widget *w, void *)
{
    // AWB Gains Blue
    Fl_Slider *o = dynamic_cast<Fl_Slider *>(w);
    double val = o->value();

    Prefs *setP = _prefs->getSubPrefs("advanced");
    setP->set("awbGainB", val);
    
    _awb_gain_b = val;
    stateChange = true;
}

static void cb_GainsR(Fl_Widget *w, void *)
{
    // AWB Gains Red
    Fl_Slider *o = dynamic_cast<Fl_Slider *>(w);
    double val = o->value();

    Prefs *setP = _prefs->getSubPrefs("advanced");
    setP->set("awbGainR", val);
    
    _awb_gain_r = val;
    stateChange = true;    
}

Fl_Double_Window *make_advanced(int _x, int _y)
{
    // TODO copy-pasta see init_advanced
    Prefs *advP = _prefs->getSubPrefs("advanced");
    
    int awbDex = advP->get("awbIndex", 0);
    float awbgr = advP->get("awbGainR", 0.0f);
    float awbgb = advP->get("awbGainB", 0.0f);
    
    
    auto panel = new Fl_Double_Window(_x, _y, 345, 350, "Advanced Settings");

    int Y = 25;
    
    auto awbCmb = new Fl_Choice(10, Y, 150, 25, "AWB Mode");
    awbCmb->down_box(FL_BORDER_BOX);
    awbCmb->copy(menu_cmbFormat);
    awbCmb->value(awbDex);
    awbCmb->callback(onAWBMode);
    awbCmb->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    
    Y += 55;   
    
    {
        auto o = new Fl_Value_Slider(30, Y, 200, 25, "AWB Gains Blue");
        o->type(FL_HOR_NICE_SLIDER);
        o->box(FL_DOWN_BOX);
        o->bounds(0.0, 10.0);
        o->value(awbgb);
        o->when(FL_WHEN_RELEASE);
        o->callback(cb_GainsB);
        o->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    }
    Y += 45;   
    {
        auto o = new Fl_Value_Slider(30, Y, 200, 25, "AWB Gains Red");
        o->type(FL_HOR_NICE_SLIDER);
        o->box(FL_DOWN_BOX);
        o->bounds(0.0, 10.0);
        o->value(awbgr);
        o->when(FL_WHEN_RELEASE);
        o->callback(cb_GainsR);
        o->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    }

    Y += 35;   
    
    new Fl_Box(10, Y, 250, 25, "NOTE: Both blue and red values must");
    Y += 20;   
    new Fl_Box(10, Y, 250, 25, "be non-zero to override AWB Mode!");
    
    Fl_Return_Button *o = new Fl_Return_Button(250, 270, 83, 25, "Close");
    o->callback(cbClose);
    
    panel->end();
    return panel;    
}

static void cbClose(Fl_Widget *, void *)
{
    advanced->hide();
}

void do_advanced(int x, int y)
{
   // Prevent multiple invocations, but don't want it modal
    if (advanced)
        return;
    
    advanced = make_advanced(x, y);
    
    advanced->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(nullptr);

    while (advanced->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
    
    delete advanced;
    advanced = nullptr;
}

void init_advanced()
{
    Prefs *advP = _prefs->getSubPrefs("advanced");
    
    int val = advP->get("awbIndex", 0);
    _awb_index = awb_table[val];

    float awbg = advP->get("awbGainR", 0.0f);
    _awb_gain_r = awbg;
    awbg = advP->get("awbGainB", 0.0f);
    _awb_gain_b = awbg;
}

const char *getAWBString()
{
    Prefs *advP = _prefs->getSubPrefs("advanced");
    int val = advP->get("awbIndex", 0);
    return menu_cmbFormat[val].text;
}
