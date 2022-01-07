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

extern bool _hflip; // state of user request for horizontal flip
extern bool _vflip; // state of user request for vertical flip
extern double _bright;
extern double _sharp;
extern double _contrast;
extern double _saturate;
extern double _evComp;

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

	for (unsigned int count = 0; ; count++)
	{
		LibcameraEncoder::Msg msg = _app->Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			return nullptr;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
			
        // check if state has changed
        if (stateChange)
        {
            _app->StopCamera();
			_app->StopEncoder();
            _app->Teardown();
		    VideoOptions *newopt = _app->GetOptions();

            // Horizontal and vertical flip implemented via transforms
            newopt->transform = libcamera::Transform::Identity;
            if (_vflip)
                newopt->transform = libcamera::Transform::VFlip * newopt->transform;
            if (_hflip)
                newopt->transform = libcamera::Transform::HFlip * newopt->transform;
                
            newopt->brightness = _bright;
            newopt->contrast   = _contrast;
            newopt->saturation = _saturate;
            newopt->sharpness  = _sharp;
            newopt->ev         = _evComp;
            
            _app->ConfigureVideo();
	        _app->StartEncoder();
            _app->StartCamera();
        }

		bool frameout = options->frames && count >= options->frames;
		if (frameout)
		{
			_app->StopCamera(); // stop complains if encoder very slow to close
			_app->StopEncoder();
			return nullptr;
		}

		if (!stateChange)
		{
			CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
			_app->EncodeBuffer(completed_request, _app->VideoStream());
			_app->ShowPreview(completed_request, _app->VideoStream());
		}
        
        stateChange = false; // TODO possibility of collision?
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

