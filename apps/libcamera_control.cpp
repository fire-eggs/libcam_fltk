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
int control_signal_received;
std::string awbgains;
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

static void configure() {
	std::vector<std::string> args = {"/home/pi/libcamera-apps/build/libcamera-control"};
	args.push_back("--frames");
	args.push_back(parameters.at("frames").get<std::string>());
	args.push_back(std::string("--shutter"));
	args.push_back(parameters.at("shutter").get<std::string>());
	args.push_back(std::string("--codec"));
	args.push_back(parameters.at("codec").get<std::string>());
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
	std::cerr << "CAPTURE START" << std::endl;
	capturing = true;
	LibcameraEncoder app;
	configure();
	VideoOptions *options = app.GetOptions();
	options->Parse(global_argc, global_argv);
	if (Control::mode == 2)
		Control::enableBuffer = true;
	output->Reset();
  	std::cerr << "CAPTURE READY - MODE: " << Control::mode << std::endl;
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4)); // MIGHT BE ABLE TO LOOP THIS RATHER THAN WHOLE METHOD
	app.StartEncoder();
	app.OpenCamera();
	app.ConfigureVideo();
	app.StartCamera();
	for (unsigned int count = 0; ; count++)
	{
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
	if (Control::mode == 1) {
		awbgains = std::to_string(options->awb_gain_r) + "," + std::to_string(options->awb_gain_b); // SET AWBGAINS ON EACH PREVIEW FRAME SO IT'S AS ACCURATE AS POSSIBLE FOR WHEN IT SWITCHES TO MODE 3
		std::cerr << "SETTING AWBGAINS: " << awbgains << std::endl;
	}
	if (Control::mode == 1 || Control::mode == 3) {
		stillCapturedCount++;
		std::cerr << "CAPTURE END" << ", CAPTURE MODE: " << Control::mode << ", STILL CAPTURE COUNT: " << stillCapturedCount << ", TOTAL FRAMES REQUESTED: " << Control::frames << std::endl;
	} else {
		std::cerr << "CAPTURE END" << ", CAPTURE MODE: " << Control::mode << ", VIDEO CAPTURE COUNT: " << Control::frames << std::endl;
	}
}

int main(int argc, char *argv[])
{
	try
	{
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
				pid = std::stoi(parameters.at("pid").get<std::string>());
				Control::mode = std::stoi(parameters.at("mode").get<std::string>());
				Control::frames = std::stoi(parameters.at("frames").get<std::string>());
				Control::timestampsFile = parameters.at("timestamps_file");
				if (Control::mode != 3 && awbgains != "0,0") 
					awbgains = "0,0";
				interval = static_cast<int>(std::max(std::stoi(parameters.at("shutter").get<std::string>())/1000, static_cast<int>(100))); // IN MILLISECONDS
				stillCapturedCount = 0;
				std::cerr << "CAPTURE MODE: " << Control::mode << std::endl;
				capture();
				control_signal_received = 0;
			} else if (capturing && Control::mode == 1) {
				if (signal_received != SIGUSR2) {
					std::cerr << "CAPTURE MODE 1 LOOPING" << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(interval)); // THIS DOESN'T TAKE INTO ACCOUNT 10MS SLEEP
					capture();
				} else if (signal_received == SIGUSR2) {
					signal_received = 0;
					capturing = false;
					std::cerr << "STOPPING MODE 1 CAPTURE" << std::endl;
				} 
			} else if (capturing && Control::mode == 3) {
				if (signal_received == SIGUSR1 && stillCapturedCount < Control::frames) {
					signal_received = 0;
					std::cerr << "CAPTURE MODE 2 LOOPING" << std::endl;
					capture();
				} else if (stillCapturedCount == Control::frames || signal_received == SIGUSR2) {
					signal_received = 0;
					capturing = false;
					std::cerr << "STOPPING MODE 3 CAPTURE" << std::endl;
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