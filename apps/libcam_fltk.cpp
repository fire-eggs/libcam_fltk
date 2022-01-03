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
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>

#include "SliderInput.h"

bool stateChange;

extern void fire_proc_thread(int argc, char ** argv);

bool _hflip;
bool _vflip;
float bright;
float saturate;
float sharp;
float contrast;
float evComp;


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

void onSharp(Fl_Widget *w, void *)
{
    sharp = ((Fl_Value_Slider *)w)->value();
    
	std::cerr << "sharp: " << sharp << std::endl;
    
}

void onContrast(Fl_Widget *w, void *)
{
    contrast = ((Fl_Value_Slider *)w)->value();
    
	std::cerr << "contrast: " << contrast << std::endl;
    
}

void onSaturate(Fl_Widget *w, void *)
{
    saturate = ((Fl_Value_Slider *)w)->value();
    
	std::cerr << "saturation: " << saturate << std::endl;
    
}

void onBright(Fl_Widget *w, void *)
{
    bright = ((Fl_Value_Slider *)w)->value();
    
	std::cerr << "brightness: " << bright << std::endl;
    
}
void onEvComp(Fl_Widget *w, void *)
{
    evComp = ((Fl_Value_Slider *)w)->value();
    
	std::cerr << "ev comp: " << evComp << std::endl;
    
}


Fl_Widget *makeSlider(int x, int y, const char *label, int min, int max, int def)
{
    Fl_Value_Slider* o = new Fl_Value_Slider(x, y, 200, 20, label);
    o->type(5);
    o->box(FL_DOWN_BOX);
    o->bounds(min, max);
    o->value(def);
    o->labelsize(12);
    o->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    return o;
}

int main(int argc, char *argv[])
{
	libcamera::logSetTarget(libcamera::LoggingTargetNone);

	Fl_Window *window = new Fl_Window (300, 300);
    Fl_Check_Button *btn = new Fl_Check_Button(20, 20, 200, 20, "Horizontal Flip");
    btn->box(FL_DOWN_BOX);
    btn->callback(onHflip);
    Fl_Check_Button *btn2 = new Fl_Check_Button(20, 40, 200, 20, "Vertical Flip");
    btn2->box(FL_DOWN_BOX);
    btn2->callback(onVflip);
    
    auto s = makeSlider(20, 80, "Sharpness", 0, 5, 1);
    s->callback(onSharp);
    s = makeSlider(20, 120, "Saturation", 0, 5, 1);
    s->callback(onSaturate);
    s = makeSlider(20, 160, "Contrast", 0, 5, 1);
    s->callback(onContrast);
    s = makeSlider(20, 200, "Brightness", -1, 1, 0);
    s->callback(onBright);
    s = makeSlider(20, 240, "EV Compensation", -10, 10, 0);
    s->callback(onEvComp);
       
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
