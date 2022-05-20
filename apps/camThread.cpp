/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The camera state machine. This is executed in a separate thread from the GUI.
 */

#include <pthread.h>
#include <unistd.h>
#include <memory> // std::unique_ptr

#include <FL/Fl.H>

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "core/still_options.hpp"

#include "libcamera/logging.h"
#include "libcamera/transform.h"
#include "preview/preview.hpp"

#include "image/image.hpp" // jpeg_save

#include "mylog.h"

const int TIMELAPSE_COMPLETE = 1002; // TODO need a better hack
const int CAPTURE_FAIL = 1004;
const int CAPTURE_SUCCESS = 1003;
const int PREVIEW_LOC = 1005;

using namespace std::placeholders;
using libcamera::Stream;

// shared-memory inter-thread communication.
extern bool stateChange;
extern bool doCapture;
extern bool doTimelapse;
extern bool _previewOn;
extern int previewX;
extern int previewY;
extern int previewW;
extern int previewH;
extern bool timeToQuit;

// General camera settings
extern bool _hflip; // state of user request for horizontal flip
extern bool _vflip; // state of user request for vertical flip
extern double _bright;
extern double _sharp;
extern double _contrast;
extern double _saturate;
extern double _evComp;

// Zoom and pan
extern double _zoom;
extern double _panH;
extern double _panV;

// the camera "app"
LibcameraEncoder *_app;

// Capture settings
extern const char *_captureFolder;
extern bool _capturePNG;
extern int _captureH;
extern int _captureW;

// Timelapse settings
extern int _timelapseW;
extern int _timelapseH;
extern bool _timelapsePNG;
extern const char *_timelapseFolder;
extern unsigned long _timelapseStep;
//extern unsigned long _timelapseLimit; // not using: limit on count
extern unsigned int _timelapseCount;

// Local pointers, set appropriately on switchTo[Capture|Timelapse]
bool saveToPNG;
const char *saveToFolder;
bool previewIsOn;

extern void guiEvent(int);

// Advanced settings
extern int32_t _metering_index;
extern int32_t _exposure_index;
extern int32_t _awb_index;
extern bool    _AwbEnable;
extern float   _awb_gain_r;
extern float   _awb_gain_b;
extern float   _analogGain;
extern float   _shutter;

// playback limit
extern int frameSkip;

static std::string generate_filename(Options const *options)
{
	char filename[256];
	std::string folder = saveToFolder;
	if (!folder.empty() && folder.back() != '/')
		folder += "/";
    
//	if (options->datetime) // TODO other filename options
	{
		std::time_t raw_time;
		std::time(&raw_time);
		char time_string[32];
		std::tm *time_info = std::localtime(&raw_time);
		std::strftime(time_string, sizeof(time_string), "%m%d%H%M%S", time_info);
		snprintf(filename, sizeof(filename), "%s%s.%s", folder.c_str(), time_string, saveToPNG ? "png" : "jpg"); // options->encoding.c_str());
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
    
  if (saveToPNG)
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
    dolog("CT:save_image: w:%d h:%d file:%s", info.width, info.height, filename.c_str());
}

static void save_images(CompletedRequestPtr &payload)
{
	Options *options = _app->GetOptions();
	std::string filename = generate_filename(options);
    options->output=filename; // KBR for exception
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

static void previewLocation()
{
    previewX = previewY = -1;
    if (previewIsOn)
        _app->getPreview()->getWindowPos(previewX, previewY);
    guiEvent(PREVIEW_LOC); // app won't shut down without this ...
}

static void captureImage(LibcameraEncoder::Msg *msg)
{
  try
  {
  _app->StopCamera();
  save_images(std::get<CompletedRequestPtr>(msg->payload));
  guiEvent(CAPTURE_SUCCESS);
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
  
  // TODO is pan/zoom reset?  if NOT, even necessary to re-establish settings?
  newopt->width = previewW;
  newopt->height= previewH;
  
  _app->ConfigureVideo();
  _app->StartEncoder();
  _app->StartCamera();
  }
  catch (std::runtime_error& e)
  {
    dolog("CT:captureImage: exception |%s|", e.what());
    guiEvent(CAPTURE_FAIL);
  }
}

static void switchToCapture(VideoOptions *options)
{
  _app->StopCamera();
  _app->StopEncoder();
  _app->Teardown();

  options->width  = _captureW;
  options->height = _captureH;

  _app->ConfigureStill( _capturePNG ? LibcameraApp::FLAG_STILL_BGR : LibcameraApp::FLAG_STILL_NONE ); // save to PNG needs BGR

  saveToPNG = _capturePNG;
  saveToFolder = _captureFolder;

  _app->StartCamera();
}

static void switchToTimelapse(VideoOptions *options)
{
  _app->StopCamera();
  _app->StopEncoder();
  _app->Teardown();

  options->width  = _timelapseW;
  options->height = _timelapseH;

  _app->ConfigureStill( _timelapsePNG ? LibcameraApp::FLAG_STILL_BGR : 
                                        LibcameraApp::FLAG_STILL_NONE ); // save to PNG needs BGR

  saveToPNG = _timelapsePNG;
  saveToFolder = _timelapseFolder;
  
  _app->StartCamera();
}

int exp_table[] = {
    libcamera::controls::ExposureNormal,
    libcamera::controls::ExposureShort,
    libcamera::controls::ExposureLong,
};

int meter_tbl[] = {
    libcamera::controls::MeteringCentreWeighted,
    libcamera::controls::MeteringSpot,
    libcamera::controls::MeteringMatrix
};

static void changeSettings()
{

  try
  {
      previewLocation();

      // Is the preview window size being changed? If so, need to 
      // close/open the camera so the preview window is re-created.
      VideoOptions *newopt = _app->GetOptions();
      bool restart = newopt->preview_height != (unsigned int)previewH;
      
      _app->StopCamera();
      _app->StopEncoder();
      _app->Teardown();
      if (restart)
          _app->CloseCamera();

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
    
      newopt->preview_width = previewW;
      newopt->preview_height = previewH;
      
      // GUI values are merely indices to libcamera::controls values
      newopt->metering_index = meter_tbl[_metering_index];
      newopt->exposure_index = exp_table[_exposure_index];
      
      newopt->awb_index = _awb_index;
      
      // TODO if not zero, need to disable AWB Mode ?
      newopt->awb_gain_b = _awb_gain_b;
      newopt->awb_gain_r = _awb_gain_r;
      newopt->gain = _analogGain;
      // TODO newopt->shutter = _shutter;

      if (restart)
        _app->OpenCamera();  // preview window is created as a side-effect here
      
      _app->ConfigureVideo();
      _app->StartEncoder();
      _app->StartCamera();   
  }
  catch (std::runtime_error& e)
  {
    dolog("CT:changeSettings: exception |%s|", e.what());
    // TODO notify GUI
  }
}

static void changePreview()
{
    // preview window is turned off/on
    
  try
  {
    previewLocation();
    _app->StopCamera();
    _app->StopEncoder();
    _app->Teardown();
    _app->CloseCamera();
    
    VideoOptions *newopt = _app->GetOptions();
    newopt->width = previewW;
    newopt->height= previewH;
    
    newopt->nopreview = !_previewOn;
    previewIsOn = _previewOn;
    
    _app->OpenCamera();  // preview window is created as a side-effect here
    _app->ConfigureVideo();
    _app->StartEncoder();
    _app->StartCamera();   
  }
  catch (std::runtime_error& e)
  {
    dolog("CT:changePreview: exception |%s|", e.what());
    // TODO notify GUI
  }
}

int frameCount = 1;

// This is the "master loop" function.
void* proc_func(void *p)
{
	VideoOptions *options = _app->GetOptions();
	
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	_app->SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));

  options->output = "/home/pi/Pictures/";  // TODO necessary?

    // Initialize preview window [created as side-effect in OpenCamera]
    previewIsOn = _previewOn;
    options->nopreview = !previewIsOn;
    
    options->preview_width = previewW;
    options->preview_height = previewH;
    options->preview_x = previewX > 0 ? previewX : 750;
    // TODO magic: needs to be adjusted by the height of the titlebar for some reason?
    options->preview_y = previewY > 0 ? previewY - 25 :  25;
    
	_app->OpenCamera();
	_app->ConfigureVideo();   // TODO should this be ConfigurePreview instead?
	_app->StartEncoder();
	_app->StartCamera();

    bool activeTimelapse = false;
    auto timelapseStart = std::chrono::high_resolution_clock::now();
    unsigned int timelapseFrameCount = 0;
    std::chrono::milliseconds timelapseStep;
    
    for (;;)
    {
        if (timeToQuit)
        {
            // GUI thread has indicated it's time to shut down
            previewLocation();
            _app->StopCamera();
            _app->StopEncoder();
            _app->Teardown();
            return nullptr;
        }
        
        LibcameraEncoder::Msg msg = _app->Wait();
        if (msg.type == LibcameraEncoder::MsgType::Quit)
            return nullptr;
        else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
            throw std::runtime_error("unrecognised message!");    // TODO error recovery

        if (doTimelapse && !activeTimelapse)
        {
            dolog("CT:activate timelapse");
            
            // first recognition of timelapse starting
            timelapseStart = std::chrono::high_resolution_clock::now();
            timelapseStep  = std::chrono::milliseconds(_timelapseStep);
            activeTimelapse = true;
            timelapseFrameCount = 0;
        }
        
        // Don't continue the timelapse if the limit has been reached,
        // as indicated by flag state
        bool timelapseTrigger = false;
        if (activeTimelapse)
        {
            // have we hit a timelapse trigger?
            auto now = std::chrono::high_resolution_clock::now();
            timelapseTrigger = activeTimelapse && (now - timelapseStart) > timelapseStep;

            // User has turned off the timelapse in the GUI
            if (!doTimelapse)
            {
                dolog("CT:timelapse stopped in GUI");
                activeTimelapse = false;
                timelapseTrigger = false;
                doTimelapse = false;
                guiEvent(TIMELAPSE_COMPLETE);
            }

            // Limit reached ["run indefinitely" indicated by _timelapseCount
            // being effectively infinite]
            else if ((_timelapseCount-1) < timelapseFrameCount)
            {
                dolog("CT:timelapse frame count reached; ended");
                activeTimelapse = false;
                timelapseTrigger = false;
                doTimelapse = false;
                guiEvent(TIMELAPSE_COMPLETE);
            }

        }

        if (_app->VideoStream())
        {
            if (doCapture) // user has requested still capture
            {
                dolog("CT:docapture");
                switchToCapture(options);
            }
            else if (stateChange) // user has made settings change
            {
                dolog("CT:changesettings");
                changeSettings();
                stateChange = false;
            }
            else if (timelapseTrigger)
            {
                dolog("CT:timelapse triggered");
                timelapseStart = std::chrono::high_resolution_clock::now(); // for next timelapse interval
                switchToTimelapse(options);
            }
            else
            {
                // normal video stream processing
                CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
                //if ((frameCount % frameSkip) == 1 || frameSkip == 1)
                if (frameCount == frameSkip)
                {
                    _app->EncodeBuffer(completed_request, _app->VideoStream());
                    _app->ShowPreview(completed_request, _app->VideoStream());
                    frameCount = 0;
                }
                frameCount++;
            }
        }
        else if (_app->StillStream()) // still capture complete
        {
            timelapseFrameCount++;
            dolog("CT:capture image - %d", timelapseFrameCount);
            doCapture = false;
            timelapseTrigger = false;
            captureImage(&msg);
        }
        
        if (previewIsOn != _previewOn)
        {
            dolog("CT: change preview");
            // switching state of the preview window
            changePreview();
        }
	}
    return nullptr;
}

pthread_t proc_thread;

pthread_t *fire_proc_thread(int argc, char ** argv)
{

    // spawn the camera loop in a separate thread.
    // the GUI thread needs to be primary, and the camera thread secondary.
    
    _app = new LibcameraEncoder();
    VideoOptions *options = _app->GetOptions();
    if (!options->Parse(argc, argv))
        return nullptr;  // TODO some sort of error indication
        
    pthread_create(&proc_thread, 0, proc_func, nullptr);
    pthread_detach(proc_thread);
    return &proc_thread;
}

