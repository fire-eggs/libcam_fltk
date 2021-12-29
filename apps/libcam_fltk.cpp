/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_vid.cpp - libcamera video record app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"

#include "libcamera/logging.h"
#include <libcamera/transform.h>


using namespace std::placeholders;

#include <FL/Fl_Window.H>
#include <FL/Fl_Check_Button.H>

bool stateChange;

extern void fire_proc_thread(int argc, char ** argv);

bool _hflip;
bool _vflip;

void onHflip(Fl_Widget *w, void *)
{
    _hflip = ((Fl_Check_Button *)w)->value();
    stateChange = true;
}

void onVflip(Fl_Widget *w, void *)
{
    _vflip = ((Fl_Check_Button *)w)->value();
    stateChange = true;
}

int main(int argc, char *argv[])
{
	libcamera::logSetTarget(libcamera::LoggingTargetNone);

	Fl_Window *window = new Fl_Window (300, 180);
    Fl_Check_Button *btn = new Fl_Check_Button(20, 40, 200, 50, "Horizontal Flip");
    btn->callback(onHflip);
    Fl_Check_Button *btn2 = new Fl_Check_Button(20, 20, 200, 20, "Vertical Flip");
    btn2->callback(onVflip);

/*    
    Fl_Box *box = new Fl_Box (20, 40, 260, 100, "Hello World!");

  box->box (FL_UP_BOX);
  box->labelsize (36);
  box->labelfont (FL_BOLD+FL_ITALIC);
  box->labeltype (FL_SHADOW_LABEL);
*/
  window->end ();
  window->show (); 

  fire_proc_thread(argc, argv);

  return(Fl::run());

/*
	try
	{
		LibcameraEncoder app;
		VideoOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			//options->verbose = true;
			
			if (options->verbose)
				options->Print();

            options->keypress = true; // allows key handling: 'x' to exit; 'h' to horiz flip, 'v' to vert flip
            
			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
		return -1;
	}
	return 0;
    */ 
}
