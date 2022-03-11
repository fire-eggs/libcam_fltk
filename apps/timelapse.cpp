//
// Created by kevin on 1/31/22.
//

#include <iostream>
#include <ctime>
#include <cmath>
#include "mainwin.h"
#include "calc.h"
#include "mylog.h"
#include <FL/Fl.H>

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#define TL_ACTIVE_COLOR FL_RED

// Inter-process communication
bool doTimelapse;

// Timelapse settings
int _timelapseW;
int _timelapseH;
bool _timelapsePNG;
const char *_timelapseFolder;
unsigned long _timelapseStep;
unsigned long _timelapseLimit;
unsigned int _timelapseCount;

//extern bool _previewOn;

extern Prefs *_prefs;
void folderPick(Fl_Input *);

Fl_Box *lblStart;
Fl_Box *lblEnd;

static unsigned long intervalToMilliseconds(int intervalType)
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

Fl_Menu_Item menu_cmbTLType[] =
        {
//{"Milliseconds", 0, nullptr, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
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

static int captureWVals[] = {640,1024,1280,1920,2272,3072,4056};
static int captureHVals[]  = {480,768,960,1080,1704,2304,3040};

static void cbTLcountdown(void *d)
{
    MainWin *mw = static_cast<MainWin *>(d);
    mw->updateCountdown();
}

#define LONGTICK 15
#define SHORTTICK 5

void MainWin::updateCountdown(bool repeat)
{
    // current time
    std::time_t raw_time;
    std::time(&raw_time);

    double t = difftime(m_TLEnd, raw_time);

    if (t < 0)
        return; // shouldn't happen but just in case
        
    int h = (t / 3600);
    int m = (long)((t / 60.0)) % 60;
    int s = (long)t % 60;

    char buff[64];
    snprintf(buff, sizeof(buff), "%02d:%02d:%02d remaining", h, m, s);
    m_lblCountdown->copy_label(buff);
    
    Fl::flush();

    double tick = t > 3600 ? LONGTICK : t > 100 ? SHORTTICK : 1;
    if (repeat)
        Fl::repeat_timeout(tick, cbTLcountdown, this);
}

static void cbTimelapse(Fl_Widget *w, void *d)
{
    dolog("cbTimelapse");
    MainWin *mw = static_cast<MainWin *>(d);
    Fl_Light_Button *btn = static_cast<Fl_Light_Button *>(w);

    if (!btn->value())
    {
        mw->timelapseEnded();
        return;
    }

    mw->m_tabTL->selection_color(TL_ACTIVE_COLOR);

    int intervalType = mw->m_cmbTLTimeType->value();
    double intervalStep = mw->m_spTLDblVal->value();

    /* 20220211 milliseconds no longer an option
    // don't attempt fractions of milliseconds
    if (intervalType == 0)
        intervalStep = (int)intervalStep;
    */

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

    /*
    std::cerr << "Timelapse step: " << intervalStep << " " << mw->m_cmbTLTimeType->text() << std::endl;
    if (framecount > 0)
        std::cerr << "   Run for " << framecount << " frames" << std::endl;
    else
        std::cerr << "   Run for " << lenval << " " << mw->m_cmbTLLenType->text() << std::endl;
*/
    
    dolog("cbTimelapse: step:%g %s frames: %d", intervalStep, mw->m_cmbTLTimeType->text(), framecount);
    
    // Convert interval, length into milliseconds
    _timelapseStep = intervalStep * intervalToMilliseconds(intervalType);
    if (framecount > 0)
        _timelapseCount = framecount;
    else
    {
        _timelapseLimit = lenval * intervalToMilliseconds(lenType);
        _timelapseCount = _timelapseLimit / _timelapseStep;
    }

    dolog("cbTimelapse2: step:%d frames: %d [limit:%d]", _timelapseStep, _timelapseCount, _timelapseLimit);
    
    // get file settings
    _timelapsePNG = mw->m_cmbTLFormat->value() == 1;
    int sizeVal = mw->m_cmbTLSize->value();
    _timelapseH = captureHVals[sizeVal];
    _timelapseW = captureWVals[sizeVal];
    _timelapseFolder = mw->inpTLFileNameDisplay->value();

    // TODO check if _timelapseStep or _timelapseLimit are negative
    // TODO check for all file settings being valid

    mw->save_timelapse_settings();

    {
        std::time_t raw_time;
        std::time(&raw_time);
        mw->m_TLStart = raw_time;

        char time_string[64];
        std::tm *time_info = std::localtime(&raw_time);
        std::strftime(time_string, sizeof(time_string), "Timelapse started at: %H:%M:%S %b %d", time_info);
        lblStart->copy_label(time_string);

        // effective timelapse length -> seconds
        int tLseconds = lround(_timelapseLimit / 1000.0);
        if (_timelapseCount)
        {
            tLseconds = lround(_timelapseCount * _timelapseStep / 1000.0);
        }

        // raw_time + seconds
        std::time_t end_time = raw_time + tLseconds; // test: 37 minutes
        time_info = std::localtime(&end_time);
        std::strftime(time_string, sizeof(time_string), "Timelapse will end at: %H:%M:%S %b %d (est.)", time_info);
        lblEnd->copy_label(time_string);

        mw->m_TLEnd = end_time;
        // TODO to function
        int countdownSeconds = tLseconds > 3600 ? LONGTICK : tLseconds > 100 ? SHORTTICK : 1;
        Fl::add_timeout(countdownSeconds, cbTLcountdown, mw);
        mw->updateCountdown(false);

    }

    // activate timelapse flag
    doTimelapse = true;
}

void MainWin::timelapseEnded()
{
    // timelapse has been ended
    doTimelapse = false;
    m_tabTL->selection_color(FL_BACKGROUND_COLOR);
    m_btnDoTimelapse->value(0);
    lblStart->copy_label("");
    lblEnd->copy_label("");
    m_lblCountdown->copy_label("");
    Fl::remove_timeout(cbTLcountdown);
    Fl::flush();
}

static void onCalc(Fl_Widget *w, void *d)
{
    MainWin *mw = static_cast<MainWin*>(d);
    mw->doCalc();
}

void MainWin::doCalc()
{
    CalcWin *calc = new CalcWin(500, 500, "Calculator", _prefs);
    calc->ControlsToUpdate(m_spTLFrameCount,m_rdTLFrameCount,m_spTLDblVal,m_cmbTLTimeType,m_rdTLLength);
    calc->showCalc();
}

void MainWin::leftTimelapse(Fl_Flex *col)
{
    Prefs *setP = _prefs->getSubPrefs("timelapse");

    int intervalChoice = setP->get("intervalType", 0);
    int lengthChoice = setP->get("lengthType", 1);
    int frameCount = setP->get("frameCount", 1);
    int limitOnFrames = setP->get("frameLimit", 1);
    double intervalVal = setP->get("interval", 1.0);
    double lengthVal   = setP->get("length",   1.0);   

    Fl_Flex *row0 = new Fl_Flex(Fl_Flex::ROW);
    {
        Fl_Box *b = new Fl_Box(0, 0, 0, 0, "Length Settings");
        b->align(FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
        b->labelfont(FL_BOLD);

        Fl_Button *bCalc = new Fl_Button(0, 0, 0, 0, "Calculator");
        bCalc->callback(onCalc, this);

        row0->end();
        row0->setSize(bCalc, 100);
    }

    Fl_Box* pad1 = new Fl_Box(0, 0, 0, 0, "");

    Fl_Flex *row1 = new Fl_Flex(Fl_Flex::ROW);
    {
        Fl_Spinner *spinDblVal = new Fl_Spinner(0, 0, 0, 0);
        spinDblVal->label("Interval:");
        spinDblVal->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        spinDblVal->type(1); // FL_FLOAT
        spinDblVal->minimum(0.5);
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
    
    col->setSize(row0, 25);
    col->setSize(pad1, 25);
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

        // horizontal padding between columns
        Fl_Box *pad = new Fl_Box(0,0,0,0);
        row0->setSize(pad,15);

        Fl_Flex *colRight = new Fl_Flex(Fl_Flex::COLUMN);
        rightTimelapse(colRight);
        colRight->end();

        row0->end();
    }

    // TODO how to prevent getting too narrow?
    Fl_Light_Button *btnDoit = new Fl_Light_Button(35, MAGIC_Y+325, 150, 25, "Run Timelapse");
    btnDoit->selection_color(TL_ACTIVE_COLOR);
    btnDoit->callback(cbTimelapse, this);
    m_btnDoTimelapse = btnDoit;

    lblStart = new Fl_Box(35, MAGIC_Y+360, 350, 25);
    //lblStart->label("Timelapse started at: hh:mm:ss MMM dd");
    lblStart->align(Fl_Align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT));
    //lblStart->deactivate();

    lblEnd = new Fl_Box(35, MAGIC_Y+390, 350, 25);
    //lblEnd->label("Timelapse will end at: hh:mm:ss MMM dd (est.)");
    lblEnd->align(Fl_Align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT));
    //lblEnd->deactivate();

    m_lblCountdown = new Fl_Box(35, MAGIC_Y+420, 350, 25);
    m_lblCountdown->align(Fl_Align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT));

    /*
    Fl_Box *lblFc = new Fl_Box(35, MAGIC_Y+450, 350, 25);
    lblFc->label("Frames captured: xxxx of nnnn");
    lblFc->align(Fl_Align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT));
    lblFc->deactivate();
    */

    /*
    Fl_Check_Button *chkHidePreview = new Fl_Check_Button(35, MAGIC_Y + 375, 200, 25, "Turn off preview window");
    chkHidePreview->callback(cbHidePreview, this);
    {
        Prefs *setP = _prefs->getSubPrefs("preview");
        int val = setP->get("on", true);
        chkHidePreview->value(!val);    
    }
    m_chkHidePreviewTL = chkHidePreview;
    */
    
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
