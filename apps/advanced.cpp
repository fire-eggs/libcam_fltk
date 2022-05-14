/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "advanced" dialog box. GUI definition, callback functions.
 */

#include <iostream>

#include <libcamera/control_ids.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Spinner.H>

extern bool stateChange;

extern int32_t _metering_index;
extern int32_t _exposure_index;
int32_t _awb_index;
bool    _AwbEnable;
float   _awb_gain_r;
float   _awb_gain_b;
extern float _analogGain;
float   _shutter;


Fl_Double_Window *advanced;


// Modifies behavior of AWB Mode, AWB Gains
Fl_Check_Button *overrideAWB;

static void cbClose(Fl_Widget *, void *);

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

/*
	std::map<std::string, int> awb_table =
		{ { "auto", libcamera::controls::AwbAuto },
			{ "normal", libcamera::controls::AwbAuto },
			{ "incandescent", libcamera::controls::AwbIncandescent },
			{ "tungsten", libcamera::controls::AwbTungsten },
			{ "fluorescent", libcamera::controls::AwbFluorescent },
			{ "indoor", libcamera::controls::AwbIndoor },
			{ "daylight", libcamera::controls::AwbDaylight },
			{ "cloudy", libcamera::controls::AwbCloudy },
			{ "custom", libcamera::controls::AwbCustom } };
*/


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

int exp_table[] = {

    libcamera::controls::ExposureNormal,
    libcamera::controls::ExposureShort,
    libcamera::controls::ExposureLong,
    
};

static Fl_Menu_Item menu_cmbExposure[] =
{
    {"normal", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"short", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"long", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

int meter_tbl[] = {
libcamera::controls::MeteringCentreWeighted,
libcamera::controls::MeteringSpot,
libcamera::controls::MeteringMatrix
};

static Fl_Menu_Item menu_cmbMetering[] =
{
    {"centre", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"spot", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"matrix", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

static void onAWBMode(Fl_Widget *w, void *)
{
    // AWB Mode choice change
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    
    // examine the state of overrideAWB
    int overV = overrideAWB->value();
    
    //const char *txt = menu_cmbFormat[val].text;
    //std::cerr << "AWB Mode:" << txt << std::endl;
    //std::cerr << "Override:" << (overV == 0 ? "OFF" : "ON") << std::endl;

    _AwbEnable = overV == 0 ? false : true;    
    _awb_index = awb_table[val];
    stateChange = true;
}

static void cb_GainsB(Fl_Widget *w, void *)
{
    // AWB Gains Blue
    Fl_Slider *o = dynamic_cast<Fl_Slider *>(w);
    double val = o->value();
    
    // examine the state of overrideAWB
    int overV = overrideAWB->value();
    //std::cerr << "AWB Gains (Blue):" << val << std::endl;
    //std::cerr << "Override:" << (overV == 0 ? "OFF" : "ON") << std::endl;    
    
    _awb_gain_b = val;
    _AwbEnable = overV == 0 ? false : true;
    stateChange = true;
}

static void cb_GainsR(Fl_Widget *w, void *)
{
    // AWB Gains Red
    Fl_Slider *o = dynamic_cast<Fl_Slider *>(w);
    double val = o->value();
    
    // examine the state of overrideAWB
    int overV = overrideAWB->value();
    //std::cerr << "AWB Gains (Red):" << val << std::endl;
    //std::cerr << "Override:" << (overV == 0 ? "OFF" : "ON") << std::endl;    
    
    _awb_gain_r = val;
    _AwbEnable = overV == 0 ? false : true;
    stateChange = true;
    
}

static void onMeter(Fl_Widget *w, void *)
{
    // metering choice change
    
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    
//    const char *txt = menu_cmbMetering[val].text;  
    //std::cerr << "Metering:" << txt << std::endl;
    
    _metering_index = meter_tbl[val];
    stateChange = true;
}

static void onExp(Fl_Widget *w, void *)
{
    // Exposure choice change
    
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    
    //const char *txt = menu_cmbExposure[val].text;            
    //std::cerr << "Exposure:" << txt << std::endl;

    _exposure_index = exp_table[val];
    stateChange = true;
}

static void onGain(Fl_Widget *w, void *)
{
    // Gain value change
    Fl_Spinner *o = dynamic_cast<Fl_Spinner*>(w);
    _analogGain = o->value();
    stateChange = true;
}

Fl_Double_Window *make_advanced(int _x, int _y)
{
    auto panel = new Fl_Double_Window(_x, _y, 345, 350, "Advanced Settings");

    int Y = 25;
    
    auto awbCmb = new Fl_Choice(10, Y, 150, 25, "AWB Mode");
    awbCmb->down_box(FL_BORDER_BOX);
    awbCmb->copy(menu_cmbFormat);
    awbCmb->callback(onAWBMode);
    awbCmb->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    //awbCmb->when(FL_WHEN_CHANGED);
    
    Y += 30;

    overrideAWB = new Fl_Check_Button(10, Y, 150, 25, "Override AWB Mode");
    overrideAWB->value(0);

    Y += 45;   
    
    {
        auto o = new Fl_Value_Slider(30, Y, 200, 25, "AWB Gains Blue");
        o->type(FL_HOR_NICE_SLIDER);
        o->box(FL_DOWN_BOX);
        o->bounds(0.0, 10.0);
        o->value(0.0);
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
        o->value(0.0);
        o->when(FL_WHEN_RELEASE);
        o->callback(cb_GainsR);
        o->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    }
    
    Y += 50;   

    auto cmbMeter = new Fl_Choice(10, Y, 150, 25, "Metering");
    cmbMeter->down_box(FL_BORDER_BOX);
    cmbMeter->copy(menu_cmbMetering);
    cmbMeter->callback(onMeter);
    cmbMeter->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));

    Y += 50;   
    
    auto cmbExp = new Fl_Choice(10, Y, 150, 25, "Exposure");
    cmbExp->down_box(FL_BORDER_BOX);
    cmbExp->copy(menu_cmbExposure);
    cmbExp->callback(onExp);
    cmbExp->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));

    Y += 50;   
    
    {
        HackSpin *o = new HackSpin(10, Y, 80, 30);
        o->type(FL_FLOAT_INPUT);
        o->step(0.25);
        o->label("Analog Gain");
        o->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
        o->callback(onGain);
    }

    Y += 30;   
    
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
    _metering_index = libcamera::controls::MeteringCentreWeighted;
    _exposure_index =     libcamera::controls::ExposureNormal;
    _awb_index =     libcamera::controls::AwbAuto;

    _AwbEnable = true;
    _awb_gain_r = 0.0;
    _awb_gain_b = 0.0;
    _analogGain = 1;   
}
