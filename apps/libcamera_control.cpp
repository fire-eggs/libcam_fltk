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

int Control::mode;
int Control::frames;
bool Control::enableBuffer;
std::string Control::timestampsFile;
// SHOULD CONSIDER PUTTING SOME OR ALL OF THESE IN CONTROL.HPP
json parameters;
int pid;
int global_argc;
char** global_argv;
bool capturing;
int stillCapturedCount;
int signal_received;
std::string awbgains;
std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create());

static void signal_handler(int signal_number)
{
	signal_received = signal_number;
	std::cerr << "LIBCAMERA: Received signal " << signal_number << std::endl;
	if (!capturing && signal_received == 12) {
		std::system("pkill -f -SIGHUP camera_server.py");
		std::cerr << "LIBCAMERA: SENDING FIRST SIGHUP, CAPTUREREADY" << std::endl;
	}
}

static void configure() {
	std::vector<std::string> args = {"/home/pi/GitHub/libcamera-apps/build/libcamera-control"};
	switch(Control::mode) {
		case 0:
			args.push_back(std::string("--timeout"));
			args.push_back(parameters.at("timeout").get<std::string>());
			break;
		case 3:
			args.push_back(std::string("--frames"));
			args.push_back(std::string("1"));
			break;
		default:
			args.push_back(std::string("--frames"));
			args.push_back(parameters.at("frames").get<std::string>());	
	}
	args.push_back(std::string("--shutter"));
	args.push_back(parameters.at("shutter").get<std::string>());
	args.push_back(std::string("--codec"));
	args.push_back(parameters.at("codec").get<std::string>());
	args.push_back(std::string("--quality"));
	args.push_back(parameters.at("quality").get<std::string>());
	// args.push_back(std::string("--mode"));
	// args.push_back(parameters.at("sensor_mode").get<std::string>()); // BROKEN CURRENTLY
	args.push_back(std::string("--width"));
	args.push_back(parameters.at("width").get<std::string>());
	args.push_back(std::string("--height"));
	args.push_back(parameters.at("height").get<std::string>());
	args.push_back(std::string("--framerate"));
	args.push_back(parameters.at("framerate").get<std::string>());
	args.push_back(std::string("--sharpness"));
	args.push_back(parameters.at("sharpness").get<std::string>());
	args.push_back(std::string("--saturation"));
	args.push_back(parameters.at("saturation").get<std::string>());
	args.push_back(std::string("--contrast"));
	args.push_back(parameters.at("contrast").get<std::string>());
	args.push_back(std::string("--brightness"));
	args.push_back(parameters.at("brightness").get<std::string>());
	args.push_back(std::string("--gain"));
	args.push_back(parameters.at("gain").get<std::string>());
	args.push_back(std::string("--awb"));
	args.push_back(parameters.at("awb").get<std::string>());
	args.push_back(std::string("--awbgains"));
	args.push_back(awbgains);
	args.push_back(std::string("--denoise"));
	args.push_back(parameters.at("denoise").get<std::string>());
	args.push_back(std::string("--nopreview"));
	args.push_back(std::string("on"));
	std::vector<char *> argv;
	for(std::string &s: args) argv.push_back(&s[0]);
	argv.push_back(NULL);
	global_argc = args.size();
	global_argv = argv.data();
	for (int i = 0; i < global_argc; i++) {
		printf("%s ", global_argv[i]);
	}
} 

static void capture() {
	LibcameraEncoder app;
	configure();
	VideoOptions *options = app.GetOptions();
	options->Parse(global_argc, global_argv);
	output->Initialize();
	switch(Control::mode) {
		case 0:
			options->timeout = std::stoi(parameters.at("timeout").get<std::string>()); // THIS SHOULDN'T BE NECESSARY - HACK
			Control::enableBuffer = false;
			break;
		case 1:
			options->frames = std::stoi(parameters.at("frames").get<std::string>()); // THIS SHOULDN'T BE NECESSARY - HACK
			Control::enableBuffer = false;
			break;
		case 2:
			output->ConfigTimestamp();
			Control::enableBuffer = true;
			break;
		case 3:
			options->frames = 1;
			Control::enableBuffer = true;
			break;
	}
	std::cerr << "LIBCAMERA: FORCE FRAMES: " << options->frames << " FORCE TIMEOUT: " << options->timeout << std::endl;
  	std::cerr << "LIBCAMERA: CAPTURE READY - MODE: " << Control::mode << std::endl;
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
	app.OpenCamera();
	app.ConfigureVideo();
	app.StartEncoder();
	app.StartCamera();
	std::cerr << "LIBCAMERA: CAPTURE START" << std::endl;
	capturing = true;
	for (unsigned int count = 0; ; count++)
	{
		LibcameraEncoder::Msg msg = app.Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			break;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		bool frameout = options->frames && count >= options->frames;
		std::cerr << "LIBCAMERA: options->frames: " << options->frames << ", count: " << count << " frameout: " << frameout << std::endl;
		if (frameout || signal_received == SIGUSR2)
		{
			if (Control::mode == 0 || Control::mode == 2)
				capturing = false;
			std::cerr << "LIBCAMERA: FRAMEOUT or SIGUSR2 received,  CAPTURE MODE: " << Control::mode << ", CAPTURING: " << capturing << std::endl;
			app.StopCamera();
			app.StopEncoder();
			break;
		}
		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBuffer(completed_request, app.VideoStream());
	}
	switch(Control::mode) {
		case 0:
			output->WriteOut();
			output->Reset();
			std::cerr << "LIBCAMERA: CAPTURE END" << ", CAPTURE MODE: " << Control::mode << ", VIDEO CAPTURE COUNT: " << Control::frames << std::endl;
			std::system("pkill -f -SIGHUP camera_server.py");
			std::cerr << "LIBCAMERA: SENDING SIGHUP, CAPTUREREADY" << std::endl;
			break;
		case 1:
			output->WriteOut();
			output->Reset();
			stillCapturedCount++;
			// awbgains = std::to_string(options->awb_gain_r) + "," + std::to_string(options->awb_gain_b); // DOESN'T WORK!!! - NEED TO DIG INTO LIBCAMERA - SET AWBGAINS ON EACH PREVIEW FRAME SO IT'S AS ACCURATE AS POSSIBLE FOR WHEN IT SWITCHES TO MODE 3
			std::cerr << "LIBCAMERA: options->awb_gain_r: " << options->awb_gain_r << std::endl;
			std::cerr << "LIBCAMERA: options->awb_gain_b: " << options->awb_gain_b << std::endl;
			std::cerr << "LIBCAMERA: CAPTURE END" << ", CAPTURE MODE: " << Control::mode << " AWBGAINS: " << awbgains << ", STILL CAPTURE COUNT: " << stillCapturedCount << ", TOTAL FRAMES REQUESTED: " << Control::frames << std::endl;
			std::system("pkill -f -SIGHUP camera_server.py");
			std::cerr << "LIBCAMERA: SENDING SIGHUP, CAPTUREREADY" << std::endl;
			break;
  		case 2:
  			output->WriteOut();
  			output->Reset();
			std::cerr << "LIBCAMERA: CAPTURE END" << ", CAPTURE MODE: " << Control::mode << ", VIDEO CAPTURE COUNT: " << Control::frames << std::endl;
			break;
		case 3:
			stillCapturedCount++;
			if (stillCapturedCount == Control::frames) {
				output->WriteOut();
				output->Reset();
			}
			std::cerr << "LIBCAMERA: CAPTURE END" << ", CAPTURE MODE: " << Control::mode << " AWBGAINS: " << awbgains << ", STILL CAPTURE COUNT: " << stillCapturedCount << ", TOTAL FRAMES REQUESTED: " << Control::frames << std::endl;
			std::system("pkill -f -SIGUSR1 camera_server.py");
			break;
	}
}

int main(int argc, char *argv[])
{
	try
	{
		std::system("pkill -f -SIGHUP camera_server.py");
		int interval;
		signal(SIGHUP, signal_handler);  // START NEW CAPTURE (SIGUSR2 MUST ALWAYS PRECEED SIGHUP)
		signal(SIGUSR1, signal_handler); // TRIGGER CAPTURE
		signal(SIGUSR2, signal_handler); // END CAPTURE
		std::cerr << "LIBCAMERA: BUFFER ALLOCATED AND READY TO CAPTURE" << std::endl;
		while (true) 
		{
			if (!capturing && signal_received == SIGHUP) {
				std::cerr << "LIBCAMERA: READING PARAMETERS" << std::endl;
				std::ifstream ifs("/home/pi/parameters.json");
				std::string content((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
				parameters = json::parse(content);
				std::cerr << std::setw(4) << parameters << std::endl;
				pid = std::stoi(parameters.at("pid").get<std::string>());
				Control::mode = std::stoi(parameters.at("mode").get<std::string>());
				Control::frames = std::stoi(parameters.at("frames").get<std::string>());
				Control::timestampsFile = parameters.at("timestamps_file");
				if (Control::mode != 3 && awbgains != "0,0") 
					awbgains = "0,0";
				interval = static_cast<int>(std::max(std::stoi(parameters.at("shutter").get<std::string>())/1000, static_cast<int>(100))); // IN MILLISECONDS
				stillCapturedCount = 0;
				std::cerr << "LIBCAMERA: CAPTURE MODE: " << Control::mode << std::endl;
				capture();
			} else if (capturing && Control::mode == 1) {
				if (signal_received != SIGUSR2) {
					std::cerr << "LIBCAMERA: CAPTURE MODE 1 LOOPING" << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(interval)); // THIS DOESN'T TAKE INTO ACCOUNT 10MS SLEEP
					capture();
				} else if (signal_received == SIGUSR2) {
					capturing = false;
					std::cerr << "LIBCAMERA: STOPPING MODE 1 CAPTURE" << std::endl;
				} 
			} else if (capturing && Control::mode == 3) {
				if (signal_received == SIGUSR1 && stillCapturedCount < Control::frames) {
					std::cerr << "LIBCAMERA: CAPTURE MODE 3 LOOPING" << std::endl;
					capture();
				} else if (stillCapturedCount == Control::frames || signal_received == SIGUSR2) {
					capturing = false;
					std::cerr << "LIBCAMERA: STOPPING MODE 3 CAPTURE" << std::endl;
				}
			}
			signal_received = 0;
			if !(capturing && Control::mode == 1)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "LIBCAMERA: ERROR: *** " << e.what() << " ***" << std::endl;
		return -1;
	}
	return 0;
}