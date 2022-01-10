// TODO disable tab tooltips inside the tab [inner group with no tooltip?]

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Slider.H>
#include "prefs.h"

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "libcamera/logging.h"
#include <libcamera/transform.h>

Prefs *_prefs;
Fl_Menu_Bar *_menu;
char *_loadfile;

// Camera settings
bool _hflip;
bool _vflip;
double _bright;
double _saturate;
double _sharp;
double _contrast;
double _evComp;

// Inter-thread communication
bool stateChange;
bool doCapture;

extern void fire_proc_thread(int argc, char ** argv);
bool OKTOSAVE;

static void onReset(Fl_Widget *, void *);

static Fl_Value_Slider *makeSlider(int x, int y, const char *label, int min, int max, int def)
{
    // TODO how to change the size of the "value" label?

    Fl_Value_Slider* o = new Fl_Value_Slider(x, y, 200, 20, label);
    o->type(5);
    o->box(FL_DOWN_BOX);
    o->bounds(min, max);
    o->value(def);
    o->labelsize(14);
    o->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    return o;
}

static void onStateChange(bool save=true)
{
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
    }
    stateChange = true;
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

#define KBR_UPD_PREVIEW 1001

static void onCapture(Fl_Widget *w, void *)
{
    doCapture = true;
}

class MainWin : public Fl_Double_Window
{
public:

    Fl_Value_Slider *m_slBright;
    Fl_Value_Slider *m_slContrast;
    Fl_Value_Slider *m_slSaturate;
    Fl_Value_Slider *m_slSharp;
    Fl_Value_Slider *m_slevComp;
    Fl_Check_Button *m_chkHflip;
    Fl_Check_Button *m_chkVflip;

    static const int MAGIC_Y = 30;
    // TODO "25" is font height? label height?

    MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w,h)
    {
        //grp = new Fl_Group(0, 25, w, h-25);

        int magicW = w - 20;
        int magicH = h - 50;

//        Fl_Box *box1 = new Fl_Box(50, 50, magicW, magicH, "Box 1");
//        box1->type(FL_BORDER_BOX);
//        box1->labelfont(1);

        Fl_Tabs *tabs = new Fl_Tabs(10, MAGIC_Y, magicW, magicH);
        magicH -= 25;
        //Fl_Group *t1 = 
        makeSettingsTab(magicW, magicH);
        //Fl_Group *t5 = 
        makeZoomTab(magicW, magicH);
        //Fl_Group *t2 = 
        makeCaptureTab(magicW, magicH);
        //Fl_Group *t3 = 
        makeVideoTab(magicW, magicH);
        //Fl_Group *t4 = 
        makeTimelapseTab(magicW, magicH);
        tabs->end();
        //Fl_Group::current()->resizable(tabs);

/*
        _crop = new cropBox(_img, 0, 25, w/2, h-25);
        _preview = new Fl_Box(w/2, 25, w/2, h-25);
        _preview->box(FL_DOWN_FRAME);
*/
        //grp->end();
        resizable(this);
    }

    void loadSavedSettings()
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
    

    Fl_Group *makeSettingsTab(int w, int h)
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
        int yStep = 50;
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

        o->end();
        //Fl_Group::current()->resizable(o);
        return o;
    }

    Fl_Group *makeCaptureTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Capture");
        o->tooltip("Image Capture");

        Fl_Button *bStill = new Fl_Button(50, MAGIC_Y+60, 150, 30, "Still Capture");
        bStill->tooltip("Single still image");
        bStill->callback(onCapture);
        Fl_Button *bBurst = new Fl_Button(50, MAGIC_Y+100, 150, 30, "Burst Capture");
        bBurst->tooltip("Capture multiple still images");

        o->end();
        Fl_Group::current()->resizable(o);
        return o;
    }

    Fl_Group *makeVideoTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Video");
        o->tooltip("Video Capture");

        o->end();
        return o;
    }

    Fl_Group *makeTimelapseTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "TimeLapse");
        o->tooltip("Make timelapse");

        o->end();
        return o;
    }

    Fl_Group *makeZoomTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Zoom");
        o->tooltip("Adjust dynamic zoom");

        o->end();
        return o;
    }

    void resize(int, int, int, int) override;
};

static void onReset(Fl_Widget *w, void *d)
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
    
    onStateChange(OKTOSAVE); // hack force not save
}

MainWin* _window;

void MainWin::resize(int x, int y, int w, int h)
{
    int oldw = this->w();
    int oldh = this->h();
    Fl_Double_Window::resize(x, y, w, h);
    if (w == oldw && h == oldh)
        ; //return; // merely moving window
    else {
        _menu->size(w, 25);
    }
    //grp->size(w, h-25);
    //_crop->size(w/2,h-25);
    //_preview->size(w/2,h-25);
    _prefs->setWinRect("MainWin",x, y, w, h);
}

static void popup(Fl_File_Chooser* filechooser) // TODO stolen from FLTK source... genericize?
{
    filechooser->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(0);

    while (filechooser->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
}

void load_cb(Fl_Widget* , void* )
{
    char filename[1024] = "";
    Fl_File_Chooser* choose = new Fl_File_Chooser(filename, "*.*", 0, "Open image file");
    choose->preview(false); // force preview off
    popup(choose);
    _loadfile = (char*)choose->value();
    if (!_loadfile)
        return;

  //Fl_Image *i = loadFile(_loadfile, nullptr);
  //_window->setImage(i);
}

void quit_cb(Fl_Widget* , void* )
{
    // TODO how to kill camera thread?
    // TODO grab preview window size/pos
    _window->hide();
    exit(0);
}

void save_cb(Fl_Widget*, void*) {
//    _window->savecrop();
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, 0, 0, FL_SUBMENU, 0, 0, 0, 0},
    {"&Load...", 0, load_cb, 0, 0, 0, 0, 0, 0},
    {"Save", 0, save_cb, 0, 0, 0, 0, 0, 0},
    {"E&xit", 0, quit_cb, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
};

int handleSpecial(int event)
{
    switch (event)
    {
    case KBR_UPD_PREVIEW:
//        _window->updatePreview();
        return 1;
    default:
        return 0;
    }
    return 1;
}

int main(int argc, char** argv)
{

    libcamera::logSetTarget(libcamera::LoggingTargetNone);


    Fl::lock();

    // Use remembered main window size/pos
    int x, y, w, h;
    _prefs = new Prefs();
    _prefs->getWinRect("MainWin", x, y, w, h);

    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);

    window.end();

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events

    window.show();


    fire_proc_thread(argc, argv);
    
    OKTOSAVE = false;
    
//    sleep(1); // wait for camera to 'settle'
    
    onReset(nullptr,_window); // init camera to defaults [hack: force no save]
    
    _window->loadSavedSettings();
    onStateChange();

    OKTOSAVE = true;
//    sleep(1); // wait for camera to 'settle'
        
    return Fl::run();   
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif

// options->preview_x
// options->preview_y
// options->preview_width
// options->preview_height
// May need to query actual physical window to get position at shutdown

