// TODO disable tab tooltips inside the tab [inner group with no tooltip?]

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#include "mainwin.h"

#include "prefs.h"
#include "capture.h"
#include "settings.h"
#include "zoom.h"
#include "mylog.h"

#include <iostream>

#define KBR_UPD_PREVIEW 1001
const int TIMELAPSE_COMPLETE = 1002;
const int CAPTURE_FAIL = 1004;
const int CAPTURE_SUCCESS = 1003;
const int PREVIEW_LOC = 1005;

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "libcamera/logging.h"
#include <libcamera/transform.h>

Prefs *_prefs; // TODO HACK
Fl_Menu_Bar *_menu;
char *_loadfile;
MainWin* _window;

extern void fire_proc_thread(int argc, char ** argv);
bool OKTOSAVE;
bool OKTOFIRE;
extern bool _previewOn;
extern int previewX;
extern int previewY;

static void popup(Fl_File_Chooser* filechooser)
{
    filechooser->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(nullptr);

    while (filechooser->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
}

void folderPick(Fl_Output *inp)
{
    // TODO native dialog as an option

    // show folder picker dialog
    Fl_File_Chooser* choose = new Fl_File_Chooser(nullptr,nullptr,
                                                  Fl_File_Chooser::DIRECTORY | Fl_File_Chooser::CREATE,
                                                  "Choose a folder");
    choose->preview(false); // force preview off

    // try to initialize to existing folder
    char *val = const_cast<char *>(inp->value());
    if (!val || strlen(val) < 1)
        val = const_cast<char *>("/home");
    choose->directory(val);

    popup(choose);
    if (!choose->count())  // no selection
        return;

    char *loaddir = (char*)choose->value();

    // update input field with picked folder
    inp->value(loaddir);
}

void onStateChange()
{
    if (!OKTOFIRE)
        return;

    dolog("STATE:bright[%g]contrast[%g]sharp[%g]evcomp[%g]saturate[%g]",
          _bright,_contrast,_sharp,_evComp,_saturate);
    dolog("STATE:hflip[%d]vflip[%d]zoom[%d]panh[%g]panv[%g]preview[%d]",
          _hflip,_vflip,_zoomChoice,_panH,_panV,_previewOn);

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
    }
    stateChange = true;
}

MainWin::MainWin(int x, int y, int w, int h,const char *L) : Fl_Double_Window(x, y, w,h,L)
    {
        int magicW = w - 20;
        int magicH = h - 50;

        Fl_Tabs *tabs = new Fl_Tabs(10, MAGIC_Y, magicW, magicH);
        magicH -= 25;

        makeSettingsTab(magicW, magicH);
        makeZoomTab(magicW, magicH);
        makeCaptureTab(magicW, magicH);
        m_tabTL = makeTimelapseTab(magicW, magicH);
        makeVideoTab(magicW, magicH);
        tabs->end();

        resizable(this);

        //m_captureHVals = captureHVals;  // TODO hack
        //m_captureWVals = captureWVals;  // TODO hack
    }

Fl_Group *MainWin::makeVideoTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Video");
        o->tooltip("Video Capture");

        o->end();
        o->deactivate();
        return o;
    }

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

void load_cb(Fl_Widget*, void*) {
}

void quit_cb(Fl_Widget* , void* )
{
    _window->save_capture_settings();
    _window->save_timelapse_settings();

    // TODO how to kill camera thread?
    // TODO grab preview window size/pos
    _window->hide();

    dolog("quit_cb");
    exit(0);
}

void save_cb(Fl_Widget*, void*) {
}

extern void do_about();
void about_cb(Fl_Widget*, void*)
{
    do_about();
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, nullptr, nullptr, FL_SUBMENU, 0, 0, 0, 0},
/*
    {"&Load...", 0, load_cb, 0, 0, 0, 0, 0, 0},
    {"Save", 0, save_cb, 0, 0, 0, 0, 0, 0},
*/
    {"E&xit", 0, quit_cb, nullptr, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"&About", 0, about_cb, nullptr, 0, 0, 0, 0, 0},
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

    case TIMELAPSE_COMPLETE:
        dolog("TL-end from camthread");
        _window->timelapseEnded();
        break;

    case CAPTURE_SUCCESS:
        dolog("Cap-success from camthread");
        _window->captureStatus(event);
        break;

    case CAPTURE_FAIL:
        dolog("Cap-fail from camthread");
        _window->captureStatus(event);
        break;
    
    case PREVIEW_LOC:
		std::cerr << "Preview Location: (" << previewX << "," << previewY << ")" << std::endl;
        _prefs->setWinRect("Preview", previewX, previewY, 640, 480);
        break;
        
    default:
        return 0;
    }
    return 1;
}

void guiEvent(int val)
{
    Fl::handle_(val, nullptr);
}

int main(int argc, char** argv)
{
    OKTOFIRE = false; // hack: don't activate statechange until settings all loaded
    initlog();

    libcamera::logSetTarget(libcamera::LoggingTargetNone);

    Fl::lock();

    // Use remembered main window size/pos
    int x, y, w, h;
    _prefs = new Prefs();
    _prefs->getWinRect("MainWin", x, y, w, h);

    // Use remembered preview window size/pos
    int pw, ph;
    _prefs->getWinRect("Preview", previewX, previewY, pw, ph);
    
    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h, "libcam_fltk");
    window.callback(quit_cb);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);

    window.end();

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events

    window.show();

    // Initialize the preview flag before starting the camera
    Prefs *setP = _prefs->getSubPrefs("preview");
    _previewOn = setP->get("on", true);
    
    fire_proc_thread(argc, argv);
    
    OKTOSAVE = false;
    
    onReset(nullptr,_window); // init camera to defaults [hack: force no save]
    
    _window->loadSavedSettings(); // TODO consider moving to settings tab

    OKTOFIRE = true;
    onStateChange();

    OKTOSAVE = true;
        
    dolog("fl_run");
    return Fl::run();   
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif

