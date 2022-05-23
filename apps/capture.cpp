/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "capture" tab: GUI definition, callback functions, and inter-thread communication.
 */

#include "prefs.h"
#include "mainwin.h"

const int CAPTURE_FAIL = 1004; // TODO Hack
const int CAPTURE_SUCCESS = 1003; // TODO Hack

// Inter-process communication
bool doCapture;

// Settings settings
// TODO consolidate
extern bool _hflip;
extern bool _vflip;
extern void onStateChange();

// capture settings
int _captureW;
int _captureH;
bool _capturePNG;
const char *_captureFolder;

extern Prefs *_prefs;
extern MainWin* _window;

// HACK made static for access via callback
static Fl_Output* inpFileNameDisplay = nullptr;

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
    _captureFolder = inpFileNameDisplay->value();
}

static int captureWVals[] = {640, 1024, 1280, 1920, 2272, 3072, 4056};
static int captureHVals[]  = {480, 768, 960, 1080, 1704, 2304, 3040};


void MainWin::captureStatus(int val)
{
    if (val == CAPTURE_SUCCESS)
    {
        m_lblCapStatus->label("Capture success");
        m_lblCapStatus->labelcolor(FL_DARK_GREEN);
        m_lblCapStatus->labelfont(FL_BOLD);
    }
    else if (val == CAPTURE_FAIL)
    {
        m_lblCapStatus->label("Capture fail");
        m_lblCapStatus->labelcolor(FL_DARK_RED);
        m_lblCapStatus->labelfont(FL_BOLD);
    }
    else
        m_lblCapStatus->label("");
   Fl::flush();       
}

static void onCapture(Fl_Widget *w, void *)
{
    // TODO pass mainwin as data
    _capturePNG = _window->cmbFormat->value() == 1;
    int sizeVal = _window->cmbSize->value();
    _captureH = captureHVals[sizeVal];
    _captureW = captureWVals[sizeVal];
    _captureFolder = inpFileNameDisplay->value();
   _window->captureStatus(0);
    doCapture = true;
}

static void onHFlip(Fl_Widget*w, void *)
{
    _hflip = ((Fl_Check_Button *)w)->value();
    onStateChange();
}

static void onVFlip(Fl_Widget*w, void *)
{
    _vflip = ((Fl_Check_Button *)w)->value();
    onStateChange();
}

Fl_Group *MainWin::makeCaptureTab(int w, int h)
{
    Prefs *capP = _prefs->getSubPrefs("capture");
    int sizeChoice = capP->get("size", 1);
    int formChoice = capP->get("format", 0);
    char * foldBuffer;
    capP->_prefs->get("folder", foldBuffer, "/home/pi/Pictures");
    // TODO check for valid folder


    Fl_Group *capGroup = new Fl_Group(10,MAGIC_Y+25,w,h, "Capture");
    capGroup->tooltip("Image Capture");

    Fl_Button *bStill = new Fl_Button(50, MAGIC_Y+60, 150, 30, "Still Capture");
    bStill->tooltip("Single still image");
    bStill->callback(onCapture);

    /* POST-V0.5
    Fl_Button *bBurst = new Fl_Button(50, MAGIC_Y+100, 150, 30, "Burst Capture");
    bBurst->tooltip("Capture multiple still images");
    bBurst->deactivate();
    */

    m_lblCapStatus = new Fl_Box(50, MAGIC_Y+310, 200, 25);
    m_lblCapStatus->label("");
    m_lblCapStatus->box(FL_NO_BOX);

    Fl_Group *sGroup = new Fl_Group(220, MAGIC_Y+60, 270, 200, "Settings");
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
    cmbFormat->copy(menu_cmbFormat);
    cmbFormat->value(formChoice);

    // TODO Quality? (for jpg)

    Fl_Output* inp= new Fl_Output(230, MAGIC_Y+200, 200, 25, "Folder:");
    inp->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    inp->value(foldBuffer);
    if (foldBuffer) free(foldBuffer);
    inpFileNameDisplay = inp;
    _captureFolder = inp->value();

    Fl_Button* btn = new Fl_Button(435, MAGIC_Y+200, 50, 25, "Pick");
    btn->callback(capFolderPick);

    sGroup->end();
    
    // Flip options moved from settings tab to capture tab
    Prefs *setP = _prefs->getSubPrefs("camera");   
    bool hflipval = setP->get("hflip", false);
    bool vflipval = setP->get("vflip", false);   
    
    m_chkHflip = new Fl_Check_Button(230, MAGIC_Y+275, 150, 20, "Horizontal Flip");
    m_chkHflip->callback(onHFlip);
    m_chkHflip->value(hflipval);
    m_chkHflip->tooltip("Flip camera image horizontally");
    m_chkVflip = new Fl_Check_Button(230, MAGIC_Y+305, 150, 20, "Vertical Flip");
    m_chkVflip->callback(onVFlip);
    m_chkVflip->value(vflipval);
    m_chkVflip->tooltip("Flip camera image vertically");
    
    capGroup->end();
    // TODO why crash? current()->resizable(o);
    return capGroup;
}

void MainWin::save_capture_settings()
{
    Prefs *setP = _prefs->getSubPrefs("capture");
    setP->set("size", cmbSize->value());
    setP->set("format", cmbFormat->value());
    const char *val = inpFileNameDisplay->value();
    setP->_prefs->set("folder", val);
    setP->_prefs->flush();
    
    // NOTE not saving hflip, vflip here; relying on onStateChange
}
