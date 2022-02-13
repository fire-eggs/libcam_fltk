// TODO disable tab tooltips inside the tab [inner group with no tooltip?]

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#include "mainwin.h"

#include "prefs.h"

#include <iostream>

#define KBR_UPD_PREVIEW 1001
const int TIMELAPSE_COMPLETE = 1002;

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "libcamera/logging.h"
#include <libcamera/transform.h>

Prefs *_prefs; // TODO HACK
Fl_Menu_Bar *_menu;
char *_loadfile;
MainWin* _window;

// Camera settings
bool _hflip;
bool _vflip;
double _bright;
double _saturate;
double _sharp;
double _contrast;
double _evComp;

// Zoom settings
double _zoom;
double _panH;
double _panV;
int _zoomChoice; // needed for settings save
bool _lever;     // needed for settings save

// Inter-thread communication
bool stateChange;
bool doCapture;
bool _previewOn;

// capture settings
int _captureW;
int _captureH;
bool _capturePNG;
const char *_captureFolder;

extern void fire_proc_thread(int argc, char ** argv);
bool OKTOSAVE;

// pre-declare those callbacks which reference MainWin members
//static void onReset(Fl_Widget *, void *);
static void onZoomReset(Fl_Widget *, void *);
static void onZoomChange(Fl_Widget *w, void *d);
static void onPanH(Fl_Widget *w, void *d);
static void onPanV(Fl_Widget *w, void *d);

class MainWin;
static void commonZoom(MainWin *mw, int zoomdex);

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

static void onStateChange()
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

        setP->set("zoom", _zoomChoice);

        // HACK if 'tripod pan' is active, the values have been negated; undo for save
        setP->set("panh", _panH * (_lever ? -1.0 : 1.0));
        setP->set("panv", _panV * (_lever ? -1.0 : 1.0));

        setP->set("lever", (int)_lever);
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

static void onReset(Fl_Widget *, void *d)
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

// HACK made static for access via callback
static Fl_Input* inpFileNameDisplay = nullptr;

void folderPick(Fl_Input *inp)
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

static void capFolderPick(Fl_Widget*, void *)
{
    // TODO pass as userdata
    folderPick(inpFileNameDisplay);
}

static int captureWVals[] = {640,1024,1280,1920,2272,3072,4056};
static int captureHVals[]  = {480,768,960,1080,1704,2304,3040};
static double zoomVals[] = {1.0, 0.8, 2.0/3.0, 0.5, 0.4, 1.0/3.0, 0.25, 0.2 };

static Fl_Menu_Item menu_cmbSize[] =
        {
                {" 640 x  480", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"1024 x  768", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"1280 x  960", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"1920 x 1080", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"2272 x 1704", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"3072 x 2304", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"4056 x 3040", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };

static Fl_Menu_Item menu_cmbFormat[] =
        {
                {"JPG", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"PNG", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };

Fl_Menu_Item menu_cmbZoom[] =
        {
                {"100%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"125%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"150%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"200%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"250%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"300%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"400%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"500%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };


static void onCapture(Fl_Widget *,void*); // forward decl
        

MainWin::MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w,h)
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

        m_captureHVals = captureHVals;  // TODO hack
        m_captureWVals = captureWVals;  // TODO hack
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

        _zoomChoice = setP->get("zoom", 0);
        _lever      = (bool)setP->get("lever", (int)false);
        _panH       = setP->get("panh", 0.0);
        _panV       = setP->get("panv", 0.0);
        
        m_chkHflip->value(hflipval);
        m_chkVflip->value(vflipval);
        m_slBright->value(brightVal);
        m_slContrast->value(contrastVal);
        m_slSaturate->value(saturateVal);
        m_slSharp->value(sharpVal);
        m_slevComp->value(evCompVal);

        commonZoom(this, _zoomChoice);
        
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
    
    Fl_Group *MainWin::makeCaptureTab(int w, int h)
    {
        Prefs *setP = _prefs->getSubPrefs("capture");
        int sizeChoice = setP->get("size", 1);
        int formChoice = setP->get("format", 0);
        char * foldBuffer;
        setP->_prefs->get("folder", foldBuffer, "/home/pi/Pictures");
        // TODO check for valid folder


        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Capture");
        o->tooltip("Image Capture");

        Fl_Button *bStill = new Fl_Button(50, MAGIC_Y+60, 150, 30, "Still Capture");
        bStill->tooltip("Single still image");
        bStill->callback(onCapture);
        Fl_Button *bBurst = new Fl_Button(50, MAGIC_Y+100, 150, 30, "Burst Capture");
        bBurst->tooltip("Capture multiple still images");
        bBurst->deactivate();


        Fl_Group *sGroup = new Fl_Group(220, MAGIC_Y+60, 270, 250, "Settings");
        sGroup->box(FL_ENGRAVED_FRAME);
        sGroup->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        
        cmbSize = new Fl_Choice(230, MAGIC_Y+85, 150, 25, "Image Size:");
        cmbSize->down_box(FL_BORDER_BOX);
        cmbSize->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        cmbSize->copy(menu_cmbSize);
        cmbSize->value(sizeChoice);

        cmbFormat = new Fl_Choice(230, MAGIC_Y+140, 150, 25, "File Format:");
        cmbFormat->down_box(FL_BORDER_BOX);
        cmbFormat->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        cmbFormat->menu(menu_cmbFormat);
        cmbFormat->value(formChoice);

        // TODO Quality? (for jpg)

        Fl_Input* inp= new Fl_Input(230, MAGIC_Y+200, 200, 25, "Folder:");
        inp->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        inp->value(foldBuffer);
        if (foldBuffer) free(foldBuffer);
        inp->readonly(true);
        inpFileNameDisplay = inp;

        Fl_Button* btn = new Fl_Button(435, MAGIC_Y+200, 50, 25, "Pick");
        btn->callback(capFolderPick);

        o->end();
        Fl_Group::current()->resizable(o);
        return o;
    }

    Fl_Group *MainWin::makeVideoTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Video");
        o->tooltip("Video Capture");

        o->end();
        o->deactivate();
        return o;
    }

    Fl_Group *MainWin::makeZoomTab(int w, int h)
    {
        // Restore saved values
        Prefs *setP = _prefs->getSubPrefs("camera");
        _zoomChoice  = setP->get("zoom", 0);
        double panHval  = setP->get("panh",   0.0);
        double panVval  = setP->get("panv",   0.0);
        _lever = (bool)setP->get("lever", false);

        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Zoom");
        o->tooltip("Adjust dynamic zoom");

        int slidX = 40;
        int slidY = MAGIC_Y+75;

        m_rlPanH = new Fl_Roller(slidX, slidY, 200, 25, "Pan horizontally");
        m_rlPanH->when(FL_WHEN_CHANGED);
        m_rlPanH->callback(onPanH, this);
        m_rlPanH->type(FL_HORIZONTAL);
        m_rlPanH->tooltip("Move the zoom center left or right");

        m_rlPanV = new Fl_Roller(slidX + 220, slidY, 25, 200, "Pan vertically");
        m_rlPanV->when(FL_WHEN_CHANGED);
        m_rlPanV->callback(onPanV, this);
        m_rlPanV->tooltip("Move the zoom center up or down");
        slidY += 70;

        cmbZoom = new Fl_Choice(slidX, slidY, 150, 25, "Zoom:");
        cmbZoom->down_box(FL_BORDER_BOX);
        cmbZoom->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        cmbZoom->menu(menu_cmbZoom);
        cmbZoom->value(_zoomChoice); // NOTE: do before establish callback, don't fire it
        cmbZoom->callback(onZoomChange, this);
        slidY += 50;

        m_chkLever = new Fl_Check_Button(slidX, slidY, 150, 25, "Tripod panning");
        m_chkLever->tooltip("Pan as if using a tripod lever to move the camera");
        m_chkLever->value(_lever);
        slidY += 50;

        Fl_Button *bReset = new Fl_Button(slidX, slidY, 100, 25, "Reset");
        bReset->tooltip("Reset all zoom settings to default");
        bReset->callback(onZoomReset, this);
        
        slidY += 50;
        
        _panH = panHval;
        m_rlPanH->value(_panH); // TODO confirm is not corrected
        _panV = panVval;
        m_rlPanV->value(_panV); // TODO confirm is not corrected

        _zoom = zoomVals[_zoomChoice];
        commonZoom(this, _zoomChoice);

        // TODO fine control?

        o->end();
        return o;
    }

static void onZoomReset(Fl_Widget *, void *d)
{
        MainWin *mw = (MainWin *)d;
        _zoom = 1.0;
        _zoomChoice = 0;
        _panH = +0.0;
        _panV = +0.0;
        _lever = false;

        mw->m_chkLever->value(_lever);
        mw->cmbZoom->value(_zoomChoice);
        mw->m_rlPanH->value(_panH);
        mw->m_rlPanV->value(_panV);
        mw->m_rlPanH ->deactivate();
        mw->m_rlPanV ->deactivate();

        onStateChange();
}

/* Update zoom controls on zoom choice change.
 * Used during init for saved value. Used when user changes zoom level.
 */
static void commonZoom(MainWin *mw, int zoomdex)
{
    _zoom = zoomVals[zoomdex];

    bool oldLever = _lever; // TODO onZoomReset clobbers

    if (zoomdex == 0)
        onZoomReset(nullptr, mw);
    else
    {
        double pHval = mw->m_rlPanH->value();
        double pVval = mw->m_rlPanV->value();
        
        double range = (1.0 - _zoom) / 2.0;

        mw->m_rlPanH->bounds(-range, range);
        mw->m_rlPanV->bounds(-range, range);

        _panV = mw->m_rlPanV->clamp(pVval);
        mw->m_rlPanV->value(_panV);
        _panH = mw->m_rlPanH->clamp(pHval);
        mw->m_rlPanH->value(_panH);
        
        mw->m_rlPanH->activate();
        mw->m_rlPanV->activate();
    }

    mw->m_chkLever->value(oldLever);
    _lever = oldLever;
}

static void onZoomChange(Fl_Widget *w, void *d)
{
    MainWin *mw = static_cast<MainWin *>(d);
    _zoomChoice = ((Fl_Choice *)w)->value();
    commonZoom(mw, _zoomChoice);

    if (!_zoomChoice && mw->m_chkLever->value())
    {
        _panV = -_panV;
        _panH = -_panH;
    }

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently

    onStateChange();
}

static void onPanH(Fl_Widget *w, void *d)
{
    _panH = ((Fl_Roller *)w)->value();
    
#ifdef NOISY       
    std::cerr << "panH: " << _panH << std::endl;
#endif
    
    MainWin *mw = (MainWin *)d;
    mw->m_rlPanH->value(_panH);

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently
    if (_lever)
    {
        _panH = -_panH;
    }

    onStateChange();
}

static void onPanV(Fl_Widget *w, void *d)
{
    _panV = ((Fl_Roller *)w)->value();
    
#ifdef NOISY       
    std::cerr << "panV: " << _panV << std::endl;
#endif
        
    MainWin *mw = (MainWin *)d;
    mw->m_rlPanV->value(_panV);

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently
    if (_lever)
    {
        _panV = -_panV;
    }

    onStateChange();
}

unsigned long intervalToMilliseconds(int intervalType)
{
    // TODO turn into an array like zoomVals ?
    // TODO need manifest constants for the 'type'

    switch (intervalType)
    {
        /* 20220211 milliseconds no longer an option
        case 0:
            return 1;
            */
        case 0:
            return 1000; // seconds
        case 1:
            return 60 * 1000; // minutes
        case 2:
            return 60 * 60 * 1000; // hours
    }
    return -1;
}

static void onCapture(Fl_Widget *w, void *)
{
    _capturePNG = _window->cmbFormat->value() == 1;
    int sizeVal = _window->cmbSize->value();
    _captureH = captureHVals[sizeVal];
    _captureW = captureWVals[sizeVal];
    _captureFolder = inpFileNameDisplay->value();
    
    doCapture = true;
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

void MainWin::save_capture_settings()
{
    Prefs *setP = _prefs->getSubPrefs("capture");
    setP->set("size", cmbSize->value());
    setP->set("format", cmbFormat->value());
    const char *val = inpFileNameDisplay->value();
    setP->_prefs->set("folder", val);
    setP->_prefs->flush();
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
    exit(0);
}

void save_cb(Fl_Widget*, void*) {
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
        std::cerr << "Timelapse ended" << std::endl;
        _window->timelapseEnded();
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

    libcamera::logSetTarget(libcamera::LoggingTargetNone);

    Fl::lock();

    // Use remembered main window size/pos
    int x, y, w, h;
    _prefs = new Prefs();
    _prefs->getWinRect("MainWin", x, y, w, h);

    // TODO : use actual size when building controls?
    MainWin window(x, y, w, h);
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
    
    _window->loadSavedSettings(); // TODO is this necessary? or preferred over each tab doing it?
    onStateChange();

    OKTOSAVE = true;
        
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

