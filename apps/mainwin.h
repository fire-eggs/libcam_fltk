//
// Created by kevin on 1/31/22.
//

#ifndef LIBCAMFLTK_MAINWIN_H
#define LIBCAMFLTK_MAINWIN_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Output.H>

#include <ctime>

#include "prefs.h"
#include "FL_Flex/FL_Flex.H"
#include "timeentry.h"

class MainWin : public Fl_Double_Window
{
public:
    static const int MAGIC_Y = 30;

public:
    // Camera settings
    Fl_Value_Slider *m_slBright;
    Fl_Value_Slider *m_slContrast;
    Fl_Value_Slider *m_slSaturate;
    Fl_Value_Slider *m_slSharp;
    Fl_Value_Slider *m_slevComp;
    Fl_Check_Button *m_chkHflip;
    Fl_Check_Button *m_chkVflip;

    // Capture settings
    Fl_Choice *cmbSize = nullptr;
    Fl_Choice *cmbFormat = nullptr;
    Fl_Box *m_lblCapStatus = nullptr;

    // Zoom settings
    Fl_Roller *m_rlPanH;
    Fl_Roller *m_rlPanV;
    Fl_Choice *cmbZoom = nullptr;
    Fl_Check_Button *m_chkLever;

    // Timelapse settings
    Fl_Spinner *m_spTLDblVal;          // timelapse interval step
    Fl_Spinner *m_spTLFrameCount;      // timelapse length frame count
    Fl_Choice *m_cmbTLTimeType;        // timelapse interval "type"
    Fl_Round_Button *m_rdTLFrameCount; // is timelapse length frames or time?
    Fl_Round_Button *m_rdTLLength;     // ditto
    //Fl_Choice *m_cmbTLLenType;         // timelapse length "type"
    Fl_Group *m_tabTL;                 // the timelapse tab itself
    //Fl_Spinner *m_spTLLenVal;          // timelapse length value [not framecount]
    TimeEntry *m_TLLengthOfTime;
    Fl_Choice *m_cmbTLSize;
    Fl_Choice *m_cmbTLFormat;
    Fl_Light_Button *m_btnDoTimelapse;

    Fl_Output* inpTLFileNameDisplay;

    std::time_t m_TLStart;
    std::time_t m_TLEnd;
    Fl_Box *m_lblCountdown;

    static void onCalcResults(Fl_Widget *, void *);

public:
    MainWin(int x, int y, int w, int h, const char *L=0); // : Fl_Double_Window(x, y, w,h);

    Fl_Group *makeSettingsTab(int w, int h);
    Fl_Group *makeCaptureTab(int w, int h);
    Fl_Group *makeVideoTab(int w, int h);
    Fl_Group *makeTimelapseTab(int w, int h);
    void leftTimelapse(Fl_Flex *col);
    void rightTimelapse(Fl_Flex *col);
    void timelapseEnded();
    void captureStatus(int event);
    void doCalc();

    Fl_Group *makeZoomTab(int w, int h);

    void resize(int, int, int, int) override;

    void loadSavedSettings();
    void loadZoomSettings();
    void save_capture_settings();
    void save_timelapse_settings();

    void updateCountdown(bool repeat=true);

public:
    int *m_captureWVals;
    int *m_captureHVals;
};

void folderPick(Fl_Output *);

#endif //LIBCAMFLTK_MAINWIN_H
