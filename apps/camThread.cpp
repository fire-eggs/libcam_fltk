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
#include "core/still_options.hpp"

#include "libcamera/logging.h"
#include "libcamera/transform.h"

#include "image/image.hpp" // jpeg_save

using namespace std::placeholders;
using libcamera::Stream;

// brute-force inter-process communication.
extern bool stateChange;
extern bool doCapture;

extern bool _hflip; // state of user request for horizontal flip
extern bool _vflip; // state of user request for vertical flip
extern double _bright;
extern double _sharp;
extern double _contrast;
extern double _saturate;
extern double _evComp;

extern double _zoom;
extern double _panH;
extern double _panV;

// the camera "app"
LibcameraEncoder *_app;

extern const char *_captureFolder;
extern bool _capturePNG;
extern int _captureH;
extern int _captureW;

static std::string generate_filename(Options const *options)
{
	char filename[256];
	//std::string folder = options->output; // sometimes "output" is used as a folder name
	std::string folder = _captureFolder;
	if (!folder.empty() && folder.back() != '/')
		folder += "/";
    
//	if (options->datetime)
	{
		std::time_t raw_time;
		std::time(&raw_time);
		char time_string[32];
		std::tm *time_info = std::localtime(&raw_time);
		std::strftime(time_string, sizeof(time_string), "%m%d%H%M%S", time_info);
		snprintf(filename, sizeof(filename), "%s%s.%s", folder.c_str(), time_string, _capturePNG ? "png" : "jpg"); // options->encoding.c_str());
	}
/*	
	else if (options->timestamp)
		snprintf(filename, sizeof(filename), "%s%u.%s", folder.c_str(), (unsigned)time(NULL),
				 options->encoding.c_str());
	else
	snprintf(filename, sizeof(filename), options->output.c_str(), options->framestart);
*/        
	filename[sizeof(filename) - 1] = 0;
	return std::string(filename);
}

static void save_image(CompletedRequestPtr &payload, Stream *stream,
					   std::string const &filename)
{
	Options const *options = _app->GetOptions();
	StreamInfo info = _app->GetStreamInfo(stream);
	const std::vector<libcamera::Span<uint8_t>> mem = _app->Mmap(payload->buffers[stream]);
    
    if (_capturePNG)
		png_save(mem, info, filename, options);
    else        
		jpeg_save(mem, info, payload->metadata, filename, _app->CameraId(), options);
//	if (stream == _app->RawStream())
//		dng_save(mem, info, payload->metadata, filename, app.CameraId(), options);
//	else if (options->encoding == "jpg")
//		jpeg_save(mem, info, payload->metadata, filename, _app->CameraId(), options);
//	else if (options->encoding == "png")
//		png_save(mem, info, filename, options);
//	else if (options->encoding == "bmp")
//		bmp_save(mem, info, filename, options);
//	else
//		yuv_save(mem, info, filename, options);
//	if (options->verbose)
    std::cerr << "Saved image " << info.width << " x " << info.height << " to file " << filename << std::endl;
}

static void save_images(CompletedRequestPtr &payload)
{
	Options *options = _app->GetOptions();
	std::string filename = generate_filename(options);
	save_image(payload, _app->StillStream(), filename);
	//update_latest_link(filename, options);
/*    
	if (options->raw)
	{
		filename = filename.substr(0, filename.rfind('.')) + ".dng";
		save_image(app, payload, app.RawStream(), filename);
	}
	options->framestart++;
	if (options->wrap)
		options->framestart %= options->wrap;
*/    
}


void* proc_func(void *p)
{
	VideoOptions *options = _app->GetOptions();
	
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	_app->SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));

    options->output = "/home/pi/Pictures/";
    
	_app->OpenCamera();
	_app->ConfigureVideo();
	_app->StartEncoder();
	_app->StartCamera();

    //bool inCapture = false;
    
	for (unsigned int count = 0; ; count++)
	{
		LibcameraEncoder::Msg msg = _app->Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			return nullptr;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
			
        if (_app->VideoStream())
        {
            if (doCapture) // user has requested still capture
            {
                _app->StopCamera();
                _app->StopEncoder();
                _app->Teardown();
                
                options->width  = _captureW;
                options->height = _captureH;
                
                _app->ConfigureStill( _capturePNG ? LibcameraApp::FLAG_STILL_BGR : LibcameraApp::FLAG_STILL_NONE ); // save to PNG needs BGR
                
//                _app->configuration_->at(0).size.width = _captureW;
//                _app->configuration_->at(0).size.height = _captureH;
                
                _app->StartCamera();
            }
            else if (stateChange) // user has made settings change
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
                
                newopt->roi_x = _panH + (1.0-_zoom) / 2.0;  // pan values are relative to 'center'
                newopt->roi_y = _panV + (1.0-_zoom) / 2.0;
                newopt->roi_height = _zoom;
                newopt->roi_width = _zoom;
                
                _app->ConfigureVideo();
                _app->StartEncoder();
                _app->StartCamera();
                
                stateChange = false;
            }
            else
            {
                // normal video stream processing
                CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
                _app->EncodeBuffer(completed_request, _app->VideoStream());
                _app->ShowPreview(completed_request, _app->VideoStream());
            }
        }
        else if (_app->StillStream()) // still capture complete
        {
            doCapture = false;
            _app->StopCamera();
            save_images(std::get<CompletedRequestPtr>(msg.payload));
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
/*        
        
        // 1) user has made settings change
        // 2) request for frame capture has completed
        if (stateChange || inCapture)
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
            
            inCapture = false;
        }

        if (doCapture)
        {
            _app->StopCamera();
            //_app->StopEncoder();
            _app->Teardown();
            
            inCapture = true;
            
            //StillOptions *newopt = _app->GetCaptureOptions();
            
            // TODO modify options
            // TODO how are these options provided to ConfigureStill ?
            
            _app->ConfigureStill();
            
            _app->StartCamera();
        }
        
		bool frameout = options->frames && count >= options->frames;
		if (frameout)
		{
			_app->StopCamera(); // stop complains if encoder very slow to close
			_app->StopEncoder();
			return nullptr;
		}

		if (!stateChange && !inCapture)
		{
			CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
			_app->EncodeBuffer(completed_request, _app->VideoStream());
			_app->ShowPreview(completed_request, _app->VideoStream());
		}
        
        stateChange = false; // TODO possibility of collision?
        doCapture = false;
*/        
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

