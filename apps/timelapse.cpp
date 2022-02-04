//
// Created by kevin on 1/31/22.
//

#include <iostream>
#include "mainwin.h"

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

bool doTimelapse;

int _timelapseW;
int _timelapseH;
bool _timelapsePNG;
const char *_timelapseFolder;
unsigned long _timelapseStep;
unsigned long _timelapseLimit;
unsigned int _timelapseCount;

extern Prefs *_prefs;
Fl_Menu_Item menu_cmbTLType[] =
        {
                {"Milliseconds", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"Seconds", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"Minutes", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"Hours", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };

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

void folderPick(Fl_Input *);
unsigned long intervalToMilliseconds(int intervalType);

static void capTLFolderPick(Fl_Widget* w, void *d)
{
    MainWin *mw = static_cast<MainWin *>(d);
    folderPick(mw->inpTLFileNameDisplay);
}

static void cbRadio(Fl_Widget *w, void *d)
{
    // Fl_Flex uses FL_Group to contain rows/columns. This breaks radio button
    // behavior when they are in different rows/columns.

    MainWin *mw = static_cast<MainWin *>(d);
    if (w == mw->m_rdTLLength)
        mw->m_rdTLFrameCount->value(0);
    else
        mw->m_rdTLLength->value(0);
}

void MainWin::timelapseEnded()
{
    doTimelapse = false;
    m_tabTL->selection_color(FL_BACKGROUND_COLOR);
    m_btnDoTimelapse->value(0);
    m_tabTL->redraw_label();
    redraw();
    Fl::flush();
}

static void cbTimelapse(Fl_Widget *w, void *d)
{
    MainWin *mw = static_cast<MainWin *>(d);
    Fl_Light_Button *btn = static_cast<Fl_Light_Button *>(w);

    if (!btn->value())
    {
        // deactivate timelapse flag
        doTimelapse = false;
        mw->m_tabTL->selection_color(FL_BACKGROUND_COLOR);
        return;
    }

    mw->m_tabTL->selection_color(FL_YELLOW);

    int intervalType = mw->m_cmbTLTimeType->value();
    double intervalStep = mw->m_spTLDblVal->value();

    // don't attempt fractions of milliseconds
    if (intervalType == 0)
        intervalStep = (int)intervalStep;

    int framecount = -1;
    int lenType = -1;
    double lenval = -1.0;
    if (mw->m_rdTLFrameCount->value())
        framecount = mw->m_spTLFrameCount->value();
    else
    {
        lenType = mw->m_cmbTLLenType->value();
        lenval = mw->m_spTLLenVal->value();
    }

    std::cerr << "Timelapse step: " << intervalStep << " " << mw->m_cmbTLTimeType->text() << std::endl;
    if (framecount > 0)
        std::cerr << "   Run for " << framecount << " frames" << std::endl;
    else
        std::cerr << "   Run for " << lenval << " " << mw->m_cmbTLLenType->text() << std::endl;

    // Convert interval, length into milliseconds
    _timelapseStep = intervalStep * intervalToMilliseconds(intervalType);
    if (framecount > 0)
        _timelapseCount = framecount;
    else
    {
        _timelapseCount = 0;
        _timelapseLimit = lenval * intervalToMilliseconds(lenType);
    }

    // get file settings
    _timelapsePNG = mw->m_cmbTLFormat->value() == 1;
    int sizeVal = mw->m_cmbTLSize->value();
    _timelapseH = mw->m_captureHVals[sizeVal];
    _timelapseW = mw->m_captureWVals[sizeVal];
    _timelapseFolder = mw->inpTLFileNameDisplay->value();

    // TODO check if _timelapseStep or _timelapseLimit are negative
    // TODO check for all file settings being valid

    mw->save_timelapse_settings();
    
    // activate timelapse flag
    doTimelapse = true;
}

void MainWin::leftTimelapse(Fl_Flex *col)
{
    Prefs *setP = _prefs->getSubPrefs("timelapse");

    int intervalChoice = setP->get("intervalType", 1);
    int lengthChoice = setP->get("lengthType", 1);
    int frameCount = setP->get("frameCount", 1);
    int limitOnFrames = setP->get("frameLimit", 1);
    double intervalVal = setP->get("interval", 1.0);
    double lengthVal   = setP->get("length",   1.0);   

    Fl_Box *b = new Fl_Box(0, 0, 0, 0, "Length Settings");
    b->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
    b->labelfont(FL_BOLD);

    Fl_Box* pad1 = new Fl_Box(0, 0, 0, 0, "");

    Fl_Flex *row1 = new Fl_Flex(Fl_Flex::ROW);
    {
        Fl_Spinner *spinDblVal = new Fl_Spinner(0, 0, 0, 0);
        spinDblVal->label("Interval:");
        spinDblVal->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        spinDblVal->type(1); // FL_FLOAT
        spinDblVal->minimum(1);
        spinDblVal->maximum(500);
        spinDblVal->step(0.5);
        spinDblVal->value(intervalVal);
        m_spTLDblVal = spinDblVal;

        m_cmbTLTimeType = new Fl_Choice(0, 0, 0, 0);
        m_cmbTLTimeType->down_box(FL_BORDER_BOX);
        m_cmbTLTimeType->copy(menu_cmbTLType);
        m_cmbTLTimeType->value(intervalChoice);
        row1->end();
        row1->setSize(spinDblVal, 85);
    }

    Fl_Flex *row2 = new Fl_Flex(Fl_Flex::ROW);
    Fl_Round_Button *rdFrameCount = new Fl_Round_Button(0,0,0,0, "Number of frames");
    rdFrameCount->type(102);
    rdFrameCount->down_box(FL_ROUND_DOWN_BOX);
    rdFrameCount->callback(cbRadio, this);
    if (limitOnFrames) rdFrameCount->value(1);
    m_rdTLFrameCount = rdFrameCount;
    row2->end();

    Fl_Flex *row3 = new Fl_Flex(Fl_Flex::ROW);
    {
        Fl_Box *pad = new Fl_Box(0, 0, 0, 0, "");

        Fl_Spinner *spinFrameCount = new Fl_Spinner(0, 0, 0, 0);
        spinFrameCount->minimum(1);
        spinFrameCount->maximum(10000);
        spinFrameCount->value(frameCount);
        spinFrameCount->step(1);
        m_spTLFrameCount = spinFrameCount;
        row3->end();
        row3->setSize(pad, 25);
        row3->setSize(spinFrameCount, 85);
    }

    Fl_Flex *row4 = new Fl_Flex(Fl_Flex::ROW);
    Fl_Round_Button *rdMaxTime = new Fl_Round_Button(0,0,0,0, "Length of time");
    rdMaxTime->type(102);
    rdMaxTime->down_box(FL_ROUND_DOWN_BOX);
    if (!limitOnFrames) rdMaxTime->value(1);
    rdMaxTime->callback(cbRadio, this);
    m_rdTLLength = rdMaxTime;
    row4->end();

    Fl_Flex *row5 = new Fl_Flex(Fl_Flex::ROW);
    {
        Fl_Box *pad = new Fl_Box(0, 0, 0, 0, "");

        Fl_Spinner *spinLengthVal = new Fl_Spinner(0, 0, 0, 0);
        spinLengthVal->type(1); // FL_FLOAT
        spinLengthVal->minimum(1);
        spinLengthVal->maximum(500);
        spinLengthVal->step(0.5);
        spinLengthVal->value(lengthVal);
        m_spTLLenVal = spinLengthVal;

        m_cmbTLLenType = new Fl_Choice(0, 0, 0, 0);
        m_cmbTLLenType->down_box(FL_BORDER_BOX);
        m_cmbTLLenType->menu(menu_cmbTLType);
        m_cmbTLLenType->value(lengthChoice);
        row5->end();
        row5->setSize(pad, 25);
        row5->setSize(spinLengthVal, 85);
    }

    col->setSize(b, 35);
    col->setSize(pad1, 15);
    col->setSize(row1, 30);
    col->setSize(row2, 30);
    col->setSize(row3, 30);
    col->setSize(row4, 30);
    col->setSize(row5, 30);
}

void MainWin::rightTimelapse(Fl_Flex *col)
{
    Prefs *setP = _prefs->getSubPrefs("timelapse");

    int sizeChoice = setP->get("size", 1);
    int formChoice = setP->get("format", 0);

    // TODO check for valid folder
    char * foldBuffer;
    setP->_prefs->get("folder", foldBuffer, "/home/pi/Pictures");


    Fl_Box *b = new Fl_Box(0, 0, 0, 0, "Image Settings");
    b->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
    b->labelfont(FL_BOLD);

    Fl_Box* pad1 = new Fl_Box(0, 0, 0, 0, "");

    Fl_Flex *row2 = new Fl_Flex(Fl_Flex::ROW);
    Fl_Choice *cmbTLSize;
    cmbTLSize = new Fl_Choice(0,0,0,0, "Image Size:");
    cmbTLSize->down_box(FL_BORDER_BOX);
    cmbTLSize->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    cmbTLSize->copy(menu_cmbSize);
    cmbTLSize->value(sizeChoice);
    m_cmbTLSize = cmbTLSize;
    row2->end();

    Fl_Box* pad2 = new Fl_Box(0, 0, 0, 0, "");

    Fl_Flex *row3 = new Fl_Flex(Fl_Flex::ROW);
    Fl_Choice *cmbTLFormat;
    cmbTLFormat = new Fl_Choice(0,0,0,0, "File Format:");
    cmbTLFormat->down_box(FL_BORDER_BOX);
    cmbTLFormat->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    cmbTLFormat->copy(menu_cmbFormat);
    cmbTLFormat->value(formChoice);
    m_cmbTLFormat = cmbTLFormat;
    row3->end();

    Fl_Box* pad3 = new Fl_Box(0, 0, 0, 0, "");

    Fl_Flex *row4 = new Fl_Flex(Fl_Flex::ROW);
    {
        inpTLFileNameDisplay = new Fl_Input(0,0,0,0, "Folder:");
        inpTLFileNameDisplay->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        inpTLFileNameDisplay->value(foldBuffer);
        if (foldBuffer) free(foldBuffer);
        inpTLFileNameDisplay->readonly(true);

        Fl_Button *btn = new Fl_Button(0,0,0,0, "Pick");
        btn->callback(capTLFolderPick, this);
        row4->end();
        row4->setSize(btn,50);
    }
    col->setSize(b, 35);
    col->setSize(pad1, 15);
    col->setSize(row2, 30);
    col->setSize(pad2, 15);
    col->setSize(row3, 30);
    col->setSize(pad3, 15);
    col->setSize(row4, 30);
}

Fl_Group *MainWin::makeTimelapseTab(int w, int h)
{
    Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "TimeLapse");
    o->tooltip("Make timelapse");

    Fl_Flex *row0 = new Fl_Flex(30, MAGIC_Y + 50, w-60, h-100,Fl_Flex::ROW);
    {
        Fl_Flex *colLeft = new Fl_Flex(Fl_Flex::COLUMN);
        leftTimelapse(colLeft);
        colLeft->end();
        Fl_Box *pad = new Fl_Box(0,0,0,0);
        Fl_Flex *colRight = new Fl_Flex(Fl_Flex::COLUMN);
        rightTimelapse(colRight);
        colRight->end();
        row0->setSize(pad,10);
        row0->end();
    }

    Fl_Light_Button *btnDoit = new Fl_Light_Button(35, MAGIC_Y+325, 150, 25, "Run Timelapse");
    btnDoit->callback(cbTimelapse, this);
    m_btnDoTimelapse = btnDoit;

    o->end();

    return o;
}

void MainWin::save_timelapse_settings()
{
    Prefs *setP = _prefs->getSubPrefs("timelapse");
    
    setP->set("size", m_cmbTLSize->value());
    setP->set("format", m_cmbTLFormat->value());
    const char *val = inpTLFileNameDisplay->value();
    setP->_prefs->set("folder", val);
    
    setP->set("frameCount", m_spTLFrameCount->value());
    setP->set("intervalType", m_cmbTLTimeType->value());
    setP->set("lengthType", m_cmbTLLenType->value());
    setP->set("frameLimit", m_rdTLFrameCount->value());

    setP->set("length",   m_spTLLenVal->value());
    setP->set("interval", m_spTLDblVal->value());
    
    setP->_prefs->flush();
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif
