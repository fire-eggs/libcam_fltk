/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "settings" tab: GUI definition, callback functions, and inter-thread communication.
 */

#include <iostream>
#include "prefs.h"
#include "mainwin.h"
#include "settings.h"
#include "mylog.h"
#include "zoom.h" // zoom settings for onStateChange

// Camera settings : implementation
bool _hflip;
bool _vflip;
double _bright;
double _saturate;
double _sharp;
double _contrast;
double _evComp;
int32_t _metering_index;
int32_t _exposure_index;
float   _analogGain;


// Inter-thread communication
bool _previewOn;
bool stateChange;
// preview window dimensions/location
int previewX;
int previewY;
int previewW;
int previewH;
int previewChoice;

extern Prefs *_prefs;
extern MainWin* _window;

extern bool OKTOFIRE;
extern bool OKTOSAVE;

static Fl_Menu_Item menu_cmbExposure[] =
{
    {"normal", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"short", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"long", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {0,     0, 0, 0, 0,                      0, 0,  0, 0}
};

static Fl_Menu_Item menu_cmbMetering[] =
{
    {"centre", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"spot", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {"matrix", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
    {0,     0, 0, 0, 0,                      0, 0,  0, 0}
};


// TODO move to own header
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



void onStateChange()
{
    if (!OKTOFIRE)
        return;

    dolog("STATE:bright[%g]contrast[%g]sharp[%g]evcomp[%g]saturate[%g]",
          _bright,_contrast,_sharp,_evComp,_saturate);
    dolog("STATE:hflip[%d]vflip[%d]zoom[%d]panh[%g]panv[%g]preview[%d]",
          _hflip,_vflip,_zoomChoice,_panH,_panV,_previewOn);
    dolog("PREVIEW: (%d,%d,%d,%d) ; choice %d",
          previewX, previewY, previewW, previewH, previewChoice);

    if (OKTOSAVE)
    {
        Prefs *setP = _prefs->getSubPrefs("camera");

        setP->set("bright", _bright);
        setP->set("contrast", _contrast);
        setP->set("sharp", _sharp);
        setP->set("evcomp", _evComp);
        setP->set("saturate", _saturate);
        setP->set("hflip", (int)_hflip);
        setP->set("vflip", (int)_vflip);

        setP->set("zoom", _zoomChoice);

        // HACK if 'tripod pan' is active, the values have been negated; undo for save
        setP->set("panh", _panH * (_lever ? -1.0 : 1.0));
        setP->set("panv", _panV * (_lever ? -1.0 : 1.0));

        setP->set("lever", (int)_lever);

        setP->set("previewChoice", previewChoice);
        
        // save meter,exp,gain
        setP->set("meterChoice",_metering_index);
        setP->set("exposureChoice", _exposure_index);
        setP->set("analogGain", _analogGain);

        savePreviewLocation();
    }
    stateChange = true;
}

// TODO goes in class?
static Fl_Value_Slider *makeSlider(int x, int y, const char *label, int min, int max, double def, bool vert=false)
{
    // TODO how to change the size of the "value" label?

    Fl_Value_Slider* o = new Fl_Value_Slider(x, y, vert ? 35 : 200, vert ? 200 : 25, label);
    o->type(vert ? FL_VERT_NICE_SLIDER : FL_HOR_NICE_SLIDER);
    o->box(FL_DOWN_BOX);
    o->bounds(min, max);
    o->value(def);
    o->labelsize(14);
    o->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    return o;
}

// TODO for the various settings callbacks, consider passing the address of the variable
// to change as the data parameter, and thus a couple of callback functions (one for 
// sliders, one for checkboxen).
static void onBright(Fl_Widget *w, void *)
{
    _bright = ((Fl_Value_Slider *)w)->value();
    onStateChange();
}

static void onContrast(Fl_Widget *w, void *)
{
    _contrast = ((Fl_Value_Slider *)w)->value();
    onStateChange();
}

static void onSharp(Fl_Widget *w, void *)
{
    _sharp = ((Fl_Value_Slider *)w)->value();
    onStateChange();
}

static void onSaturate(Fl_Widget *w, void *)
{
    _saturate = ((Fl_Value_Slider *)w)->value();
    onStateChange();
}

static void onEvComp(Fl_Widget *w, void *)
{
    _evComp = ((Fl_Value_Slider *)w)->value();
    onStateChange();
}

static void onMeter(Fl_Widget *w, void *)
{
    // metering choice change
    
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    const char *txt = menu_cmbMetering[val].text;
    
    std::cerr << "Metering:" << txt << std::endl;
    
    // TODO have camThread do the lookup
    //_metering_index = meter_tbl[val];
    _metering_index = val;
    onStateChange();
}

static void onExp(Fl_Widget *w, void *)
{
    // Exposure choice change
    
    Fl_Choice *o = dynamic_cast<Fl_Choice*>(w);
    int val = o->value();
    const char *txt = menu_cmbExposure[val].text;
            
    std::cerr << "Exposure:" << txt << std::endl;

    // TODO have camThread do the lookup
    //_exposure_index = exp_table[val];
    _exposure_index = val;
    onStateChange();
}

static void onGain(Fl_Widget *w, void *)
{
    // Gain value change
    Fl_Spinner *o = dynamic_cast<Fl_Spinner*>(w);
    _analogGain = o->value();
    onStateChange();
}

// User has selected a preview size via the main menu bar
void onPreviewSizeChange(int choice)
{
    static int previewWVals[] = {640, 800,1024, 1280, 1600};
    static int previewHVals[] = {480, 600, 768, 960, 1200};
    
    previewChoice = choice;
    previewW = previewWVals[previewChoice];
    previewH = previewHVals[previewChoice];
    onStateChange();
}

void onReset(Fl_Widget *, void *d)
{
    // TODO constants for defaults

    MainWin *mw = (MainWin *)d;
    mw->m_slBright->value(0.0);
    mw->m_slContrast->value(1.0);
    mw->m_slSaturate->value(1.0);
    mw->m_slSharp->value(1.0);
    mw->m_slevComp->value(0.0);
        
    _bright = 0.0;
    _contrast = 1.0;
    _sharp = 1.0;
    _saturate = 1.0;
    _evComp = 0.0;

    // reset meter,exp,gain
    _metering_index = 0;
    _exposure_index = 0;
    _analogGain     = 8.0;
    mw->m_cmbMeter->value(_metering_index);
    mw->m_cmbExp  ->value(_exposure_index);
    mw->m_spinGain->value(_analogGain);
    
    onStateChange();
}

void getPreviewData()
{
    Prefs *setP = _prefs->getSubPrefs("preview");
    _previewOn = setP->get("on", true);

    _prefs->getWinRect("Preview", previewX, previewY, previewW, previewH);
    if (previewW == 500 || previewW > 1024 || previewW < 0 ||
        previewH == 500 || previewH > 768  || previewH < 0)
    { // first time / error defaults
        previewW = 1024;
        previewH = 768;
    }
}

void savePreviewLocation()
{
    // both values at -1 indicates preview window not open at shutdown
    if (previewX != -1 && previewY != -1)
        _prefs->setWinRect("Preview", previewX, previewY, previewW, previewH);
}

// User has toggled the "preview shown" menu
void togglePreview(bool on)
{
    // NOTE: magic! camThread is monitoring this location and handles it.
    _previewOn = on;
    
    Prefs *setP = _prefs->getSubPrefs("preview");
    setP->set("on", _previewOn);
}

void MainWin::loadSavedSettings()
{
    // The purpose of this function is to initialize the camera to the last
    // saved state.
    // TODO does this mean that loading values in makeSettingsTab is unnecessary?

    Prefs *setP = _prefs->getSubPrefs("camera");

    // TODO range check
    double brightVal  = setP->get("bright",   0.0);
    double saturateVal= setP->get("saturate", 1.0);
    double sharpVal   = setP->get("sharp",    1.0);
    double contrastVal= setP->get("contrast", 1.0);
    double evCompVal  = setP->get("evcomp",   0.0);
    bool hflipval     = setP->get("hflip",    false);
    bool vflipval     = setP->get("vflip",    false);

    // load meter,exp,gain
    _metering_index = setP->get("meterChoice", 0);
    _exposure_index = setP->get("exposureChoice", 0);
    _analogGain     = setP->get("analogGain", 8.0);
    m_cmbMeter->value(_metering_index);
    m_cmbExp  ->value(_exposure_index);
    m_spinGain->value(_analogGain);
    
    m_chkHflip->value(hflipval);
    m_chkVflip->value(vflipval);
    m_slBright->value(brightVal);
    m_slContrast->value(contrastVal);
    m_slSaturate->value(saturateVal);
    m_slSharp->value(sharpVal);
    m_slevComp->value(evCompVal);

    _hflip    = hflipval;
    _vflip    = vflipval;
    _bright   = brightVal;
    _saturate = saturateVal;
    _sharp    = sharpVal;
    _contrast = contrastVal;
    _evComp   = evCompVal;

    // TODO ???    
    previewChoice = setP->get("previewChoice", 2);
}

Fl_Group *MainWin::makeSettingsTab(int w, int h)
{
    Prefs *setP = _prefs->getSubPrefs("camera");

    // TODO range check
    double brightVal  = setP->get("bright",   0.0);
    double saturateVal= setP->get("saturate", 1.0);
    double sharpVal   = setP->get("sharp",    1.0);
    double contrastVal= setP->get("contrast", 1.0);
    double evCompVal  = setP->get("evcomp",   0.0);

    // load meter,exp,gain
    _metering_index = setP->get("meterChoice", 0);
    _exposure_index = setP->get("exposureChoice", 0);
    _analogGain     = setP->get("analogGain", 8.0);
    
    Fl_Group *setGroup = new Fl_Group(10,MAGIC_Y+25,w,h, "Settings");
    setGroup->tooltip("Configure camera settings");
    
    int slidX = 40;
    int slidY = MAGIC_Y + 60;
    int yStep = 60;
    m_slBright = makeSlider(slidX, slidY, "Brightness", -1, 1, brightVal);
    m_slBright->callback(onBright);
    m_slBright->value(brightVal);
    m_slBright->tooltip("-1.0: black; 1.0: white; 0.0: standard");
    m_slBright->when(FL_WHEN_RELEASE);  // no change until slider stops
    slidY += yStep;
    m_slContrast = makeSlider(slidX, slidY, "Contrast", 0, 5, contrastVal);
    m_slContrast->callback(onContrast);
    m_slContrast->value(contrastVal);
    m_slContrast->tooltip("0: minimum; 1: default; 1+: extra contrast");
    m_slContrast->when(FL_WHEN_RELEASE);
    slidY += yStep;
    m_slSaturate = makeSlider(slidX, slidY, "Saturation", 0, 5, saturateVal);
    m_slSaturate->callback(onSaturate);
    m_slSaturate->value(saturateVal);
    m_slSaturate->tooltip("0: minimum; 1: default; 1+: extra color saturation");
    m_slSaturate->when(FL_WHEN_RELEASE);
    slidY += yStep;
    m_slSharp = makeSlider(slidX, slidY, "Sharpness", 0, 5, sharpVal);
    m_slSharp->callback(onSharp);
    m_slSharp->value(sharpVal);
    m_slSharp->tooltip("0: minimum; 1: default; 1+: extra sharp");
    m_slSharp->when(FL_WHEN_RELEASE);
    slidY += yStep;
    m_slevComp = makeSlider(slidX, slidY, "EV Compensation", -10, 10, evCompVal);
    m_slevComp->callback(onEvComp);
    m_slevComp->value(evCompVal);
    m_slevComp->tooltip("-10 to 10 stops");
    m_slevComp->when(FL_WHEN_RELEASE);
    slidY += yStep;

    // Position reset button below all other controls
    Fl_Button *bReset = new Fl_Button(270, slidY-20, 100, 25, "Reset");
    bReset->tooltip("Reset all settings to default");
    bReset->callback(onReset, this);
    

    // Add metering, exposure and analog gains as "common" settings
    int Y = MAGIC_Y + 60;
    
    m_cmbMeter = new Fl_Choice(275, Y, 150, 25, "Metering");
    m_cmbMeter->down_box(FL_BORDER_BOX);
    m_cmbMeter->copy(menu_cmbMetering);
    m_cmbMeter->value(_metering_index);
    m_cmbMeter->callback(onMeter);
    m_cmbMeter->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    m_cmbMeter->tooltip("Select the camera metering mode");

    Y += 60;   
    
    m_cmbExp = new Fl_Choice(275, Y, 150, 25, "Exposure");
    m_cmbExp->down_box(FL_BORDER_BOX);
    m_cmbExp->copy(menu_cmbExposure);
    m_cmbExp->value(_exposure_index);
    m_cmbExp->callback(onExp);
    m_cmbExp->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
    m_cmbExp->tooltip("Select the camera exposure mode");
    
    Y += 60;   
    
    {
        auto o = new HackSpin(275, Y, 80, 30);
        o->type(FL_FLOAT_INPUT);
        o->minimum(0.25); // Ted suggests the minimum can be 0.25
        o->maximum(50);   // arbitrary
        o->value(_analogGain);
        o->step(0.25);
        o->wrap(false);   // TODO check calculator: spinners to not wrap?
        o->label("Analog Gain");
        o->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_TOP));
        o->callback(onGain);
        o->tooltip("Specify the camera analogue gain value");
        m_spinGain = o;
    }
    
    
    setGroup->end();
    //Fl_Group::current()->resizable(o);
    return setGroup;
}

int getPreviewSizeChoice()
{
    Prefs *setP = _prefs->getSubPrefs("camera");
    int previewChoice = setP->get("previewChoice", 2);
    return previewChoice;
}

bool isPreviewShown()
{
    Prefs *setP = _prefs->getSubPrefs("preview");
    int v = setP->get("on", 1);
    return v != 0;    
}
