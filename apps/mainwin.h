/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The main window. GUI definition, callback functions, and inter-thread communication.
 */

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

    // Expanded camera settings
    Fl_Spinner *m_spinGain;
    Fl_Choice  *m_cmbMeter;
    Fl_Choice  *m_cmbExp;
    
    // Capture settings
    Fl_Choice *cmbSize = nullptr;
    Fl_Choice *cmbFormat = nullptr;
    Fl_Box *m_lblCapStatus = nullptr;
    Fl_Group *m_tabCap; // the capture tab itself

    // Zoom settings
    Fl_Roller *m_rlPanH;
    Fl_Roller *m_rlPanV;
    Fl_Choice *cmbZoom = nullptr;
    Fl_Check_Button *m_chkLever;

    // Timelapse settings and controls [left side]
    Fl_Spinner *m_spTLDblVal;          // timelapse interval step
    Fl_Spinner *m_spTLFrameCount;      // timelapse length frame count
    Fl_Choice *m_cmbTLTimeType;        // timelapse interval "type"
    Fl_Round_Button *m_rdTLFrameCount; // is timelapse length: frames, time or indefinite?
    Fl_Round_Button *m_rdTLLength;     // ditto
    Fl_Round_Button *m_rdTLIndefinite; // ditto
    Fl_Group *m_tabTL;                 // the timelapse tab itself
    TimeEntry *m_TLLengthOfTime;       // timelapse length of time limit

    // Timelapse settings and controls [right side]
    Fl_Choice *m_cmbTLSize;            // timelapse image size
    Fl_Choice *m_cmbTLFormat;          // timelapse image format
    Fl_Output* inpTLFileNameDisplay;   // timelapse folder display

    Fl_Light_Button *m_btnDoTimelapse;

    // Display the timelapse countdown
    std::time_t m_TLEnd;
    Fl_Box *m_lblCountdown;

    static void onCalcResults(Fl_Widget *, void *);
    static void onTabChange(Fl_Widget *, void *);

public:
    MainWin(int x, int y, int w, int h, const char *L=nullptr);

    Fl_Group *makeSettingsTab(int w, int h);
    Fl_Group *makeCaptureTab(int w, int h);
    Fl_Group *makeTimelapseTab(int w, int h);
    Fl_Group *makeServoTab(int w, int h);
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
};

void folderPick(Fl_Output *);

#endif //LIBCAMFLTK_MAINWIN_H
