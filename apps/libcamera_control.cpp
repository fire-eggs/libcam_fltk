/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
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

using namespace std::placeholders;

// Some keypress/signal handling.

// static int signal_received;
// static void default_signal_handler(int signal_number)
// {
// 	signal_received = signal_number;
// 	std::cerr << "Received signal " << signal_number << std::endl;
// }

// static int get_key_or_signal(VideoOptions const *options, pollfd p[1])
// {
// 	int key = 0;
// 	if (options->keypress)
// 	{
// 		poll(p, 1, 0);
// 		if (p[0].revents & POLLIN)
// 		{
// 			char *user_string = nullptr;
// 			size_t len;
// 			[[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
// 			key = user_string[0];
// 		}
// 	}
// 	if (options->signal)
// 	{
// 		if (signal_received == SIGUSR1)
// 			key = '\n';
// 		else if (signal_received == SIGUSR2)
// 			key = 'x';
// 	}
// 	return key;
// }

int main(int argc, char *argv[])
{
	try
	{
		std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create());
		while (true) {
			std::cerr << "DUSTIN START" << std::endl;
			LibcameraEncoder app;
			VideoOptions *options = app.GetOptions();
			options->Parse(argc, argv);
			app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
			app.StartEncoder();
			app.OpenCamera();
			app.ConfigureVideo();
			app.StartCamera();
			auto start_time = std::chrono::high_resolution_clock::now();

			// Monitoring for keypresses and signals.
			// signal(SIGUSR1, default_signal_handler);
			// signal(SIGUSR2, default_signal_handler);
			// pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

			for (unsigned int count = 0; ; count++)
			{
				std::cerr << "HERE0" << std::endl;
				try
				{
				LibcameraEncoder::Msg msg = app.Wait();
				// if (msg.type == LibcameraEncoder::MsgType::Quit)
				// 	break;
				// else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
				// 	throw std::runtime_error("unrecognised message!");
				// int key = get_key_or_signal(options, p);
				// if (key == '\n')
				// 	output->Signal();

				// if (options->verbose)
				// 	std::cerr << "Viewfinder frame " << count << std::endl;
				std::cerr << "HERE1" << std::endl;
				auto now = std::chrono::high_resolution_clock::now();
				bool timeout = !options->frames && options->timeout &&
							   (now - start_time > std::chrono::milliseconds(options->timeout));
				bool frameout = options->frames && count >= options->frames;
				// if (timeout || frameout || key == 'x' || key == 'X')
				if (timeout || frameout)
				{
					std::cerr << "HERE2" << std::endl;
					app.StopCamera(); // stop complains if encoder very slow to close
					std::cerr << "HERE3" << std::endl;
					app.StopEncoder();
					std::cerr << "HERE4" << std::endl;
					break;
				}
				std::cerr << "HERE5" << std::endl;
				CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
				app.EncodeBuffer(completed_request, app.VideoStream());
				// app.ShowPreview(completed_request, app.VideoStream());
				std::cerr << "HERE6" << std::endl;
				}
				catch (std::exception const &e)
				{
					std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
				}
				
			}
			std::cerr << "DUSTIN END" << std::endl;
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
		return -1;
	}
	return 0;
}