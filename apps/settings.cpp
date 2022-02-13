//
// Created by kevin on 2/13/22.
//

#include <iostream>
#include "capture.h"
#include "prefs.h"
#include "mainwin.h"
#include "settings.h"

// Camera settings : implementation
bool _hflip;
bool _vflip;
double _bright;
double _saturate;
double _sharp;
double _contrast;
double _evComp;

// Inter-thread communication
bool _previewOn;
bool stateChange;

extern Prefs *_prefs;
void folderPick(Fl_Input *);
extern MainWin* _window;

extern void onStateChange();

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

static void onBright(Fl_Widget *w, void *)
{
    _bright = ((Fl_Value_Slider *)w)->value();
#ifdef NOISY
    std::cerr << "brightness: " << _bright << std::endl;
#endif
    onStateChange();
}

static void onContrast(Fl_Widget *w, void *)
{
    _contrast = ((Fl_Value_Slider *)w)->value();
#ifdef NOISY
    std::cerr << "contrast: " << _contrast << std::endl;
#endif
    onStateChange();
}

static void onSharp(Fl_Widget *w, void *)
{
    _sharp = ((Fl_Value_Slider *)w)->value();
#ifdef NOISY
    std::cerr << "sharpness: " << _sharp << std::endl;
#endif
    onStateChange();
}

static void onSaturate(Fl_Widget *w, void *)
{
    _saturate = ((Fl_Value_Slider *)w)->value();
#ifdef NOISY
    std::cerr << "saturation: " << _saturate << std::endl;
#endif
    onStateChange();
}

static void onEvComp(Fl_Widget *w, void *)
{
    _evComp = ((Fl_Value_Slider *)w)->value();
#ifdef NOISY
    std::cerr << "Ev comp: " << _evComp << std::endl;
#endif
    onStateChange();
}

static void onHFlip(Fl_Widget*w, void *)
{
    _hflip = ((Fl_Check_Button *)w)->value();
#ifdef NOISY
    std::cerr << "H-Flip: " << _hflip << std::endl;
#endif
    onStateChange();
}

static void onVFlip(Fl_Widget*w, void *)
{
    _vflip = ((Fl_Check_Button *)w)->value();
#ifdef NOISY
    std::cerr << "V-Flip: " << _vflip << std::endl;
#endif
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
    mw->m_chkHflip->value(false);
    mw->m_chkVflip->value(false);

    _bright = 0.0;
    _contrast = 1.0;
    _sharp = 1.0;
    _saturate = 1.0;
    _evComp = 0.0;
    _hflip = false;
    _vflip = false;

    onStateChange(); // hack force not save
}

static void cbHidePreview(Fl_Widget *w, void *)
{
    Fl_Check_Button* btn = dynamic_cast<Fl_Check_Button*>(w);
    _previewOn = !btn->value();

    Prefs *setP = _prefs->getSubPrefs("preview");
    setP->set("on", _previewOn);
}

void MainWin::loadSavedSettings()
{
    Prefs *setP = _prefs->getSubPrefs("camera");

    // TODO range check
    double brightVal  = setP->get("bright",   0.0);
    double saturateVal= setP->get("saturate", 1.0);
    double sharpVal   = setP->get("sharp",    1.0);
    double contrastVal= setP->get("contrast", 1.0);
    double evCompVal  = setP->get("evcomp",   0.0);
    bool hflipval     = setP->get("hflip",    false);
    bool vflipval     = setP->get("vflip",    false);

    m_chkHflip->value(hflipval);
    m_chkVflip->value(vflipval);
    m_slBright->value(brightVal);
    m_slContrast->value(contrastVal);
    m_slSaturate->value(saturateVal);
    m_slSharp->value(sharpVal);
    m_slevComp->value(evCompVal);

    _hflip = hflipval;
    _vflip = vflipval;
    _bright= brightVal;
    _saturate = saturateVal;
    _sharp = sharpVal;
    _contrast = contrastVal;
    _evComp = evCompVal;
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
    bool hflipval     = setP->get("hflip",    false);
    bool vflipval     = setP->get("vflip",    false);

    Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Settings");
    o->tooltip("Configure camera settings");

    m_chkHflip = new Fl_Check_Button(270, MAGIC_Y+60, 150, 20, "Horizontal Flip");
    m_chkHflip->callback(onHFlip);
    m_chkHflip->value(hflipval);
    m_chkHflip->tooltip("Flip camera image horizontally");
    m_chkVflip = new Fl_Check_Button(270, MAGIC_Y+90, 150, 20, "Vertical Flip");
    m_chkVflip->callback(onVFlip);
    m_chkVflip->value(vflipval);
    m_chkVflip->tooltip("Flip camera image vertically");

    Fl_Button *bReset = new Fl_Button(270, MAGIC_Y+200, 100, 25, "Reset");
    bReset->tooltip("Reset all settings to default");
    bReset->callback(onReset, this);

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


    Fl_Check_Button *chkHidePreview = new Fl_Check_Button(slidX, slidY, 200, 25, "Turn off preview window");
    chkHidePreview->callback(cbHidePreview, this);
    {
        Prefs *setP = _prefs->getSubPrefs("preview");
        chkHidePreview->value(!setP->get("on", true));
    }

    o->end();
    //Fl_Group::current()->resizable(o);
    return o;
}