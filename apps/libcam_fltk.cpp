/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The main window. GUI definition, callback functions, and inter-thread communication.
 */

// TODO disable tab tooltips inside the tab [inner group with no tooltip?]

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#include "mainwin.h"
#include "prefs.h"
#include "settings.h"
#include "zoom.h"
#include "mylog.h"

// TODO must match in camThread.cpp; needs better definition mechanism
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
MainWin* _window;
bool _done; // shutdown status : used for final cleanup

extern pthread_t *fire_proc_thread(int argc, char ** argv);
bool OKTOSAVE; // initial loading of settings: is it OK to save these?
bool OKTOFIRE; // initial loading of settings: is it OK to send to the camera?

bool timeToQuit = false; // inter-thread flag
pthread_t *camThread; // co-ordinate shutdown

extern void do_about();
extern void do_advanced(int x, int y);
extern void init_advanced();

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

void MainWin::onTabChange(Fl_Widget *w, void *d)
{
    MainWin * mw = static_cast<MainWin*>(d);
    Fl_Tabs *tabs = dynamic_cast<Fl_Tabs*>(w);
    if (tabs->value() != mw->m_tabCap)
    {
        // user has tabbed away from the capture tab. insure last capture status is cleared
        _window->captureStatus(-1);
    }
}

MainWin::MainWin(int x, int y, int w, int h,const char *L) : Fl_Double_Window(x, y, w,h,L)
{
    int magicW = w - 20;
    int magicH = h - 50;

    Fl_Tabs *tabs = new Fl_Tabs(10, MAGIC_Y, magicW, magicH);
    tabs->callback(onTabChange, this);
    tabs->when(FL_WHEN_CHANGED);

    magicH -= 25;

    makeSettingsTab(magicW, magicH);
    makeZoomTab(magicW, magicH);
    m_tabCap = makeCaptureTab(magicW, magicH);
    m_tabTL = makeTimelapseTab(magicW, magicH);
    makeServoTab(magicW, magicH);
    tabs->end();

    resizable(this);
}

void MainWin::resize(int x, int y, int w, int h)
{
    int oldw = this->w();
    int oldh = this->h();
    Fl_Double_Window::resize(x, y, w, h);
    if (w == oldw && h == oldh)
        ;  // merely moving window
    else {
        _menu->size(w, 25);
    }
    _prefs->setWinRect("MainWin",x, y, w, h);
}

void cb_quit(Fl_Widget* , void* )
{
    _window->save_capture_settings();
    _window->save_timelapse_settings();

    // kill camera thread
    _done = false;
    timeToQuit = true;
    void *retval = nullptr;
    if (camThread)
        pthread_join(*camThread, &retval);

    _window->hide();
    // Wait for camthread shutdown callback
    while (!_done && camThread)
    {
        Fl::wait();
    }
    dolog("quit_cb");
    exit(0);
}

void cb_about(Fl_Widget*, void*)
{
    do_about();
}

void adv_cb(Fl_Widget *, void *)
{
    int x = _window->x();
    int y = _window->y();
    int h = _window->h();
    do_advanced(x, y+h);
}

void cb_PreviewSize(Fl_Widget *, void *d)
{
    auto val = (int)(long)(d);
    onPreviewSizeChange(val);
}

void cb_ShowPreview(Fl_Widget *w, void *)
{
    const Fl_Menu_Item *mi = ((Fl_Menu_ *)w)->mvalue();
    auto v = mi->checked();   
    togglePreview(v != 0);
}

extern int frameSkip;
void cb_PreviewSpeed(Fl_Widget *, void *d)
{
    auto val = (int)(long)(d);
    frameSkip = val;
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, nullptr, nullptr, FL_SUBMENU, 0, 0, 0, 0},
    {"Ad&vanced", 0, adv_cb, nullptr, 0, 0, 0, 0, 0},
    {"E&xit", 0, cb_quit, nullptr, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"&Preview", 0, nullptr, nullptr, FL_SUBMENU, 0,0,0,0},
    {"&Shown", 0, cb_ShowPreview, nullptr, FL_MENU_TOGGLE | FL_MENU_DIVIDER, 0,0,0,0},
    {"640 x 480", 0, cb_PreviewSize, (void *)0, FL_MENU_RADIO, 0, 0, 0, 0},
    {"800 x 600", 0, cb_PreviewSize, (void*)1, FL_MENU_RADIO, 0, 0, 0, 0},
    {"1024 x 768", 0, cb_PreviewSize, (void *)2, FL_MENU_RADIO | FL_MENU_DIVIDER, 0, 0, 0, 0},
    {"1:1", 0, cb_PreviewSpeed, (void *)1, FL_MENU_RADIO, 0, 0, 0, 0},
    {"1:2", 0, cb_PreviewSpeed, (void *)2, FL_MENU_RADIO, 0, 0, 0, 0},
    {"1:3", 0, cb_PreviewSpeed, (void *)3, FL_MENU_RADIO, 0, 0, 0, 0},
    {"1:4", 0, cb_PreviewSpeed, (void *)4, FL_MENU_RADIO, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"&About", 0, cb_about, nullptr, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
};

/* Events coming from camThread.
*/
int handleSpecial(int event)
{
    switch (event)
    {
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
        savePreviewLocation();
        _done = true;
        break;
        
    default:
        return 0;
    }
    return 1;
}

// Helper to fire an event. Invoked by camThread, but defined here
// so as to avoid including FLTK header(s) in camThread.
void guiEvent(int val)
{
    Fl::handle_(val, nullptr);
}

void initPreviewMenu()
{
    // TODO initialize the preview shown check state
    Fl_Menu_Item *mi = (Fl_Menu_Item *)(_menu->find_item(cb_ShowPreview));
    if (isPreviewShown())
        mi->set();
    else
        mi->clear();
    
    // TODO initialize the preview size radio state
    mi = (Fl_Menu_Item *)(_menu->find_item(cb_PreviewSize));
    int v = getPreviewSizeChoice();
    mi = mi->next(v);
    mi->set();
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

    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h, "libcam_fltk");
    window.callback(cb_quit);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);

    window.end();

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events

    window.show(argc, argv);

    // Need to initialize the preview state before starting the camera
    getPreviewData();
    initPreviewMenu();
    
    camThread = fire_proc_thread(argc, argv);
    OKTOSAVE = false;  // Note: hack to prevent save
    
    onReset(nullptr,_window); // init camera to defaults [hack: force no save]

    // Initialize the camera to last saved settings
    init_advanced();
    _window->loadSavedSettings(); // TODO can be combined with getPreviewData ???
    _window->loadZoomSettings();

    OKTOFIRE = true;
    onStateChange();
    OKTOSAVE = true; // TODO consider parameter

    dolog("fl_run");
    return Fl::run();   
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif

