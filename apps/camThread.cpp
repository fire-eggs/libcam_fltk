#ifndef _WIN32
#define HAVE_PTHREAD
#define HAVE_PTHREAD_H
#endif

// TODO not an official part of FLTK?
//#include "threads.h"
#include <pthread.h>

#ifdef _WIN32
#include "stdlib.h"
#define sleep _sleep
#else
#include <unistd.h>
#endif

#include <FL/Fl.H>
#include <memory> // std::unique_ptr

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"

#include "libcamera/logging.h"
#include <libcamera/transform.h>

using namespace std::placeholders;

// brute-force inter-process communication.
extern bool stateChange;

// the camera "app"
LibcameraEncoder *_app;

void* proc_func(void *p)
{
	VideoOptions const *options = _app->GetOptions();
	
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	_app->SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));

	_app->OpenCamera();
	_app->ConfigureVideo();
	_app->StartEncoder();
	_app->StartCamera();

	//auto start_time = std::chrono::high_resolution_clock::now();

	for (unsigned int count = 0; ; count++)
	{
		bool flipper = false;

		LibcameraEncoder::Msg msg = _app->Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			return nullptr;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
			
		bool timeout = false; // KBR run forever

		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout) // || key == 'x' || key == 'X')
		{
			_app->StopCamera(); // stop complains if encoder very slow to close
			_app->StopEncoder();
			return nullptr;
		}

		if (!flipper)
		{
			CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
			_app->EncodeBuffer(completed_request, _app->VideoStream());
			_app->ShowPreview(completed_request, _app->VideoStream());
		}
	}

    return nullptr;
}

pthread_t proc_thread;

void fire_proc_thread(int argc, char ** argv)
{

    // spawn the camera loop in a separate thread.
    // the GUI thread needs to be primary, and the camera thread secondary.
    
    _app = new LibcameraEncoder();
    VideoOptions *options = _app->GetOptions();
    if (!options->Parse(argc, argv))
	return;  // TODO some sort of error indication
    
    pthread_create(&proc_thread, 0, proc_func, nullptr);
    pthread_detach(proc_thread);
}

