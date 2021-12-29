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
#include <FL/Fl_Box.H>

bool stateChange;

// Some keypress/signal handling.
/*
static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	std::cerr << "Received signal " << signal_number << std::endl;
}

static int get_key_or_signal(VideoOptions const *options, pollfd p[1])
{
	int key = 0;
	if (options->keypress)
	{
		poll(p, 1, 0);
		if (p[0].revents & POLLIN)
		{
			char *user_string = nullptr;
			size_t len;
			[[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
			key = user_string[0];
		}
	}
	if (options->signal)
	{
		if (signal_received == SIGUSR1)
			key = '\n';
		else if (signal_received == SIGUSR2)
			key = 'x';
		signal_received = 0;
	}
	return key;
}
*/

// The main even loop for the application.
/*
static void event_loop(LibcameraEncoder &app)
{
	VideoOptions const *options = app.GetOptions();
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));

	app.OpenCamera();
	app.ConfigureVideo();
	app.StartEncoder();
	app.StartCamera();
	
	//auto start_time = std::chrono::high_resolution_clock::now();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };


	for (unsigned int count = 0; ; count++)
	{
		bool flipper = false;
		
		LibcameraEncoder::Msg msg = app.Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			return;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		int key = get_key_or_signal(options, p);
		if (key == '\n')
			output->Signal();

        if (key == 'v' || key == 'V')
        {
            std::cerr << "VERTICAL FLIP" << std::endl;
            app.StopCamera();
			app.StopEncoder();
            app.Teardown();
		    VideoOptions *newopt = app.GetOptions();
            newopt->vflip_ = !newopt->vflip_;
            newopt->transform = libcamera::Transform::Identity;
            if (newopt->vflip_)
                newopt->transform = libcamera::Transform::VFlip * newopt->transform;
            app.ConfigureVideo();
	        app.StartEncoder();
            app.StartCamera();
            flipper = true;
		}
        if (key == 'h' || key == 'H')
            std::cerr << "HORIZONTAL FLIP" << std::endl;

		//if (options->verbose)
		//	std::cerr << "Viewfinder frame " << count << std::endl;
			
		//auto now = std::chrono::high_resolution_clock::now();
		//bool timeout = !options->frames && options->timeout &&
		//			   (now - start_time > std::chrono::milliseconds(options->timeout));
					   
		bool timeout = false; // KBR run forever
					   
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout || key == 'x' || key == 'X')
		{
			app.StopCamera(); // stop complains if encoder very slow to close
			app.StopEncoder();
			return;
		}

        if (!flipper)
        {
			CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
			app.EncodeBuffer(completed_request, app.VideoStream());
			app.ShowPreview(completed_request, app.VideoStream());
		}
	}
}
*/
extern void fire_proc_thread(int argc, char ** argv);

int main(int argc, char *argv[])
{
	libcamera::logSetTarget(libcamera::LoggingTargetNone);

	Fl_Window *window = new Fl_Window (300, 180);
    Fl_Box *box = new Fl_Box (20, 40, 260, 100, "Hello World!");

  box->box (FL_UP_BOX);
  box->labelsize (36);
  box->labelfont (FL_BOLD+FL_ITALIC);
  box->labeltype (FL_SHADOW_LABEL);
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
