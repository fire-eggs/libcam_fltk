/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "about" dialog box. GUI definition, callback functions.
 */
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/filename.H>

Fl_Double_Window *about;

void cbClose(Fl_Widget *w, void *d)
{
    about->hide();
    delete about;
    about = nullptr;
}

void fireLink(Fl_Widget *w, void *d)
{
    w->labelcolor(FL_DARK_MAGENTA);
    fl_open_uri((char *)d);
}

Fl_Double_Window *make_about()
{
    auto panel = new Fl_Double_Window(345, 300, "About libcam_fltk");

    new Fl_Box(10, 5, 330, 150,
"libcam_fltk : an interactive GUI\nfor libcamera-apps on Raspberry Pi\n"
"\nVersion 0.5 (Alpha)\n\nProgrammer: Kevin Routley\nProduct Manager: Ted Barnard");

    int Y = 150;
    int STEP_Y = 30;

    auto text3 = new Fl_Button(10, Y, 200, STEP_Y, "libcam_fltk Github");
    text3->labelcolor(FL_DARK_BLUE);
    text3->callback(fireLink, (void *)"https://github.com/fire-eggs/libcam_fltk");
    Y += STEP_Y + 5;

    auto text5 = new Fl_Button(10, Y, 200, 30, "libcamera-apps");
    text5->labelcolor(FL_DARK_BLUE);
    text5->callback(fireLink, (void *)"https://github.com/raspberrypi/libcamera-apps");
    Y += STEP_Y + 5;

    auto text2 = new Fl_Button(10,Y, 200, STEP_Y, "GUI by FLTK");
    text2->labelcolor(FL_DARK_BLUE);
    text2->callback(fireLink, (void *)"https://www.fltk.org");
    Y += STEP_Y + 5;

    auto text4 = new Fl_Button(10, Y, 200, 30, "Logging by spdlog");
    text4->labelcolor(FL_DARK_BLUE);
    text4->callback(fireLink, (void *)"https://github.com/gabime/spdlog");
    Y += STEP_Y + 5;

    Fl_Return_Button *o = new Fl_Return_Button(250, 170, 83, 25, "Close");
    o->callback(cbClose);

    panel->end();
    return panel;
}

void do_about()
{
    about = make_about();
    about->set_modal();
    about->show();
}
