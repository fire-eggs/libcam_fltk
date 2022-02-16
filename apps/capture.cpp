//
// Created by kevin on 2/13/22.
//

#include <iostream>
#include "prefs.h"
#include "mainwin.h"
#include "capture.h"

// Inter-process communication
bool doCapture;

// capture settings
int _captureW;
int _captureH;
bool _capturePNG;
const char *_captureFolder;

extern Prefs *_prefs;
void folderPick(Fl_Input *);
extern MainWin* _window;

// HACK made static for access via callback
static Fl_Input* inpFileNameDisplay = nullptr;

// TODO duplicated in timelapse.cpp
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

// TODO duplicated in timelapse.cpp
static Fl_Menu_Item menu_cmbFormat[] =
        {
                {"JPG", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"PNG", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };

static void capFolderPick(Fl_Widget*, void *)
{
    // TODO pass as userdata
    folderPick(inpFileNameDisplay);
}

static int captureWVals[] = {640, 1024, 1280, 1920, 2272, 3072, 4056};
static int captureHVals[]  = {480, 768, 960, 1080, 1704, 2304, 3040};

static void onCapture(Fl_Widget *w, void *)
{
    // TODO pass mainwin as data
    _capturePNG = _window->cmbFormat->value() == 1;
    int sizeVal = _window->cmbSize->value();
    _captureH = captureHVals[sizeVal];
    _captureW = captureWVals[sizeVal];
    _captureFolder = inpFileNameDisplay->value();

    doCapture = true;
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
    // TODO why crash? current()->resizable(o);
    return o;
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
