/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * libcamera_control.cpp
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "core/json.hpp"
#include "core/control.hpp"

using namespace std::placeholders;
using json = nlohmann::json;

FILE *Control::pipe;
int Control::mode;
int Control::frames;
bool Control::enableBuffer;
std::string Control::timestampsFile;
json parameters;
int global_argc;
char **global_argv;
bool captureReady;
bool capturing;
int stillCapturedCount;
int signal_received;
int control_signal_received;
std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create());

static void default_signal_handler(int signal_number)
{
	if (!capturing) return;
	signal_received = signal_number;
	std::cerr << "Received signal " << signal_number << std::endl;
}

static void control_signal_handler(int signal_number)
{
	if (capturing) return;
	control_signal_received = signal_number;
	std::cerr << "Control received signal " << signal_number << std::endl;
}

static void capture() {
	capturing = true;
	LibcameraEncoder app;
	VideoOptions *options = app.GetOptions();
	options->Parse(global_argc, global_argv);
	Control::mode = parameters.at("mode");
	std::cerr << "CAPTURE MODE: " << Control::mode << std::endl;
	Control::frames = parameters.at("frames");
	Control::timestampsFile = parameters.at("timestamps_file");
	options->shutter = parameters.at("shutter");
	options->mode = parameters.at("sensor_mode"); // BROKEN CURRENTLY WITH "4056:3040:12:P" / PASS "" FOR NOW
	options->width = parameters.at("width");
	options->height = parameters.at("height");
	options->framerate = parameters.at("framerate");
	options->sharpness = parameters.at("sharpness");
	options->saturation = parameters.at("saturation");
	options->contrast = parameters.at("contrast");
	options->brightness = parameters.at("brightness");
	options->gain = parameters.at("gain");
	options->awb = "incandescent"; //parameters.at("wbmode"); // BROKEN
	options->denoise = parameters.at("denoise");
	options->nopreview = true;
	output->Reset();
	std::cerr << "AWB: " << parameters.at("wbmode") << std::endl;
  	std::cerr << "CAPTURE READY - MODE: " << Control::mode << std::endl;
	switch(Control::mode) {
		case 0:
			Control::enableBuffer = false;
			// options->quality = parameters.at("quality");
			// options->codec = "mjpeg";
			options->codec = "yuv420";
			options->timeout = 0;
			break;
		case 1:
			Control::enableBuffer = false;
			// options->quality = parameters.at("quality");
			// options->codec = "mjpeg";
			options->codec = "yuv420";
			options->frames = 1;
			break;
		case 2:
			Control::enableBuffer = true;
			options->frames = parameters.at("frames");
			options->codec = "yuv420";
			break;
		case 3:
			Control::enableBuffer = false;
			options->codec = "yuv420";
			options->frames = 1;
			break;
  	}
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4)); // MIGHT BE ABLE TO LOOP THIS RATHER THAN WHOLE METHOD
	app.StartEncoder();
	app.OpenCamera();
	app.ConfigureVideo();
	app.StartCamera();
	for (unsigned int count = 0; ; count++)
	{
		if (Control::mode == 1 || Control::mode == 3) {
			stillCapturedCount++;
			std::cerr << "STILL CAPTURE COUNT: " << stillCapturedCount << std::endl;
		}
		LibcameraEncoder::Msg msg = app.Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			break;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		bool frameout = options->frames && count >= options->frames;
		if (frameout || signal_received == SIGUSR2)
		{
			if (Control::mode == 0 || Control::mode == 2)
				capturing = false;
			app.StopCamera();
			app.StopEncoder();
			break;
		}
		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBuffer(completed_request, app.VideoStream());
	}
	output->WriteOut();
}

int main(int argc, char *argv[])
{
	try
	{
		global_argc = argc;
		global_argv = argv;
		int interval;
		signal(SIGHUP, control_signal_handler);  // START NEW CAPTURE (SIGUSR2 MUST ALWAYS PRECEED SIGHUP)
		signal(SIGUSR1, default_signal_handler); // TRIGGER CAPTURE
		signal(SIGUSR2, default_signal_handler); // END CAPTURE
		std::cerr << "BUFFER ALLOCATED AND READY TO CAPTURE" << std::endl;
		while (true) 
		{
			if (!capturing && control_signal_received == 1) {
				std::cerr << "READING PARAMETERS" << std::endl;
				std::ifstream ifs("/home/pi/parameters.json");
				std::string content((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
				parameters = json::parse(content);
				std::cerr << std::setw(4) << parameters << std::endl;
				// interval = static_cast<int>(std::max(parameters.at("shutter")/1000, static_cast<float>(100))); // IN MILLISECONDS
				interval = 100;
				stillCapturedCount = 0;
				std::cerr << "CAPTURE START" << std::endl;
				capture();
				std::cerr << "CAPTURE END" << std::endl;
				control_signal_received = 0;
			} else if (capturing && Control::mode == 1) {
				if (signal_received != SIGUSR2) {
					std::cerr << "LOOPING1" << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					capture();
				} else if (signal_received == SIGUSR2) {
					signal_received = 0;
					capturing = false;
				} 
			} else if (capturing && Control::mode == 3) {
				if (signal_received == SIGUSR1 && stillCapturedCount < Control::frames) {
					std::cerr << "LOOPING2" << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(33));
					signal_received = 0;
					capture();
				} else if (stillCapturedCount == Control::frames) {
					signal_received = 0;
					capturing = false;
				}
			} 
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
		return -1;
	}
	return 0;
}