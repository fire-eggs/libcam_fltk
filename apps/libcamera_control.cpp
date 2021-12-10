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
#include "core/still_options.hpp"
#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"
#include "image/image.hpp"

using namespace std::placeholders;
using libcamera::Stream;

// GLOBAL HACKS UNTIL CREATION CONTROL CLASS
int global_argc;
char **global_argv;
pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } }; // NOT REALLY SURE WHAT THIS DOES

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	std::cerr << "Received signal " << signal_number << std::endl;
}

static int get_key_or_signal(StillOptions const *options, pollfd p[1])
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
	}
	return key;
}

class LibcameraStillApp : public LibcameraApp
{
public:
	LibcameraStillApp() : LibcameraApp(std::make_unique<StillOptions>()) {}

	StillOptions *GetOptions() const { return static_cast<StillOptions *>(options_.get()); }
};

static std::string generate_filename(StillOptions const *options)
{
	char filename[128];
	std::string folder = options->output; // sometimes "output" is used as a folder name
	if (!folder.empty() && folder.back() != '/')
		folder += "/";
	if (options->datetime)
	{
		std::time_t raw_time;
		std::time(&raw_time);
		char time_string[32];
		std::tm *time_info = std::localtime(&raw_time);
		std::strftime(time_string, sizeof(time_string), "%m%d%H%M%S", time_info);
		snprintf(filename, sizeof(filename), "%s%s.%s", folder.c_str(), time_string, options->encoding.c_str());
	}
	else if (options->timestamp)
		snprintf(filename, sizeof(filename), "%s%u.%s", folder.c_str(), (unsigned)time(NULL),
				 options->encoding.c_str());
	else
		snprintf(filename, sizeof(filename), options->output.c_str(), options->framestart);
	filename[sizeof(filename) - 1] = 0;
	return std::string(filename);
}

static void update_latest_link(std::string const &filename, StillOptions const *options)
{
	// Create a fixed-name link to the most recent output file, if requested.
	if (!options->latest.empty())
	{
		struct stat buf;
		if (stat(options->latest.c_str(), &buf) == 0 && unlink(options->latest.c_str()))
			std::cerr << "WARNING: could not delete latest link " << options->latest << std::endl;
		else
		{
			if (symlink(filename.c_str(), options->latest.c_str()))
				std::cerr << "WARNING: failed to create latest link " << options->latest << std::endl;
			else if (options->verbose)
				std::cerr << "Link " << options->latest << " created" << std::endl;
		}
	}
}

static void save_image(LibcameraStillApp &app, CompletedRequestPtr &payload, Stream *stream,
					   std::string const &filename)
{
	StillOptions const *options = app.GetOptions();
	unsigned int w, h, stride;
	app.StreamDimensions(stream, &w, &h, &stride);
	libcamera::PixelFormat const &pixel_format = stream->configuration().pixelFormat;
	const std::vector<libcamera::Span<uint8_t>> mem = app.Mmap(payload->buffers[stream]);
	if (stream == app.RawStream())
		dng_save(mem, w, h, stride, pixel_format, payload->metadata, filename, app.CameraId(), options);
	else if (options->encoding == "jpg")
		jpeg_save(mem, w, h, stride, pixel_format, payload->metadata, filename, app.CameraId(), options);
	else if (options->encoding == "png")
		png_save(mem, w, h, stride, pixel_format, filename, options);
	else if (options->encoding == "bmp")
		bmp_save(mem, w, h, stride, pixel_format, filename, options);
	else
		yuv_save(mem, w, h, stride, pixel_format, filename, options);
	if (options->verbose)
		std::cerr << "Saved image " << w << " x " << h << " to file " << filename << std::endl;
}

static void save_images(LibcameraStillApp &app, CompletedRequestPtr &payload)
{
	StillOptions *options = app.GetOptions();
	std::string filename = generate_filename(options);
	save_image(app, payload, app.StillStream(), filename);
	update_latest_link(filename, options);
	if (options->raw)
	{
		filename = filename.substr(0, filename.rfind('.')) + ".dng";
		save_image(app, payload, app.RawStream(), filename);
	}
	options->framestart++;
	if (options->wrap)
		options->framestart %= options->wrap;
}

static void yuv420_save(std::vector<libcamera::Span<uint8_t>> const &mem, unsigned int w, unsigned int h,
						unsigned int stride, std::string const &filename, StillOptions const *options)
{
	std::cerr << "DUSTIN yuv420_save" << std::endl;
	if (options->encoding == "yuv420")
	{
		if ((w & 1) || (h & 1))
			throw std::runtime_error("both width and height must be even");
		if (mem.size() != 1)
			throw std::runtime_error("incorrect number of planes in YUV420 data");
		FILE *fp = fopen(filename.c_str(), "w");
		if (!fp)
			throw std::runtime_error("failed to open file " + filename);
		try
		{
			uint8_t *Y = (uint8_t *)mem[0].data();
			for (unsigned int j = 0; j < h; j++)
			{
				if (fwrite(Y + j * stride, w, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
			uint8_t *U = Y + stride * h;
			h /= 2, w /= 2, stride /= 2;
			for (unsigned int j = 0; j < h; j++)
			{
				if (fwrite(U + j * stride, w, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
			uint8_t *V = U + stride * h;
			for (unsigned int j = 0; j < h; j++)
			{
				if (fwrite(V + j * stride, w, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
		}
		catch (std::exception const &e)
		{
			fclose(fp);
			throw;
		}
	}
	else
		throw std::runtime_error("output format " + options->encoding + " not supported");
}

static void yuyv_save(std::vector<libcamera::Span<uint8_t>> const &mem, unsigned int w, unsigned int h,
					  unsigned int stride, std::string const &filename, StillOptions const *options)
{
	std::cerr << "DUSTIN yuyv_save" << std::endl;
	if (options->encoding == "yuv420")
	{
		if ((w & 1) || (h & 1))
			throw std::runtime_error("both width and height must be even");
		FILE *fp = fopen(filename.c_str(), "w");
		if (!fp)
			throw std::runtime_error("failed to open file " + filename);
		try
		{
			// We could doubtless do this much quicker. Though starting with
			// YUV420 planar buffer would have been nice.
			std::vector<uint8_t> row(w);
			uint8_t *ptr = (uint8_t *)mem[0].data();
			for (unsigned int j = 0; j < h; j++, ptr += stride)
			{
				for (unsigned int i = 0; i < w; i++)
					row[i] = ptr[i << 1];
				if (fwrite(&row[0], w, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
			ptr = (uint8_t *)mem[0].data();
			for (unsigned int j = 0; j < h; j += 2, ptr += 2 * stride)
			{
				for (unsigned int i = 0; i < w / 2; i++)
					row[i] = ptr[(i << 2) + 1];
				if (fwrite(&row[0], w / 2, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
			ptr = (uint8_t *)mem[0].data();
			for (unsigned int j = 0; j < h; j += 2, ptr += 2 * stride)
			{
				for (unsigned int i = 0; i < w / 2; i++)
					row[i] = ptr[(i << 2) + 3];
				if (fwrite(&row[0], w / 2, 1, fp) != 1)
					throw std::runtime_error("failed to write file " + filename);
			}
			fclose(fp);
		}
		catch (std::exception const &e)
		{
			fclose(fp);
			throw;
		}
	}
	else
		throw std::runtime_error("output format " + options->encoding + " not supported");
}

static void rgb_save(std::vector<libcamera::Span<uint8_t>> const &mem, unsigned int w, unsigned int h,
					 unsigned int stride, std::string const &filename, StillOptions const *options)
{
	std::cerr << "DUSTIN rgb_save" << std::endl;
	if (options->encoding != "rgb")
		throw std::runtime_error("encoding should be set to rgb");
	FILE *fp = fopen(filename.c_str(), "w");
	if (!fp)
		throw std::runtime_error("failed to open file " + filename);
	try
	{
		uint8_t *ptr = (uint8_t *)mem[0].data();
		for (unsigned int j = 0; j < h; j++, ptr += stride)
		{
			if (fwrite(ptr, 3 * w, 1, fp) != 1)
				throw std::runtime_error("failed to write file " + filename);
		}
		fclose(fp);
	}
	catch (std::exception const &e)
	{
		fclose(fp);
		throw;
	}
}

void yuv_save(std::vector<libcamera::Span<uint8_t>> const &mem, unsigned int w, unsigned int h, unsigned int stride,
			  libcamera::PixelFormat const &pixel_format, std::string const &filename, StillOptions const *options)
{
	std::cerr << "DUSTIN yuv_save" << std::endl;
	if (pixel_format == libcamera::formats::YUYV)
		yuyv_save(mem, w, h, stride, filename, options);
	else if (pixel_format == libcamera::formats::YUV420)
		yuv420_save(mem, w, h, stride, filename, options);
	else if (pixel_format == libcamera::formats::BGR888 || pixel_format == libcamera::formats::RGB888)
		rgb_save(mem, w, h, stride, filename, options);
	else
		throw std::runtime_error("unrecognised YUV/RGB save format");
}

static void Still() {
	LibcameraStillApp app;
	// StillOptions *options = app.GetOptions();
	StillOptions *options = app.GetOptions(); // NEED TO SET OPTIONS BASED ON FILE INPUT PARAMETERS
	options->Parse(global_argc, global_argv);
	bool output = !options->output.empty() || options->datetime || options->timestamp; // output requested?
	bool keypress = options->keypress || options->signal; // "signal" mode is much like "keypress" mode
	unsigned int still_flags = LibcameraApp::FLAG_STILL_NONE;
	if (options->encoding == "rgb" || options->encoding == "png")
		still_flags |= LibcameraApp::FLAG_STILL_BGR;
	else if (options->encoding == "bmp")
		still_flags |= LibcameraApp::FLAG_STILL_RGB;
	if (options->raw)
		still_flags |= LibcameraApp::FLAG_STILL_RAW;

	app.OpenCamera();
	if (options->immediate)
		app.ConfigureStill(still_flags);
	else
		app.ConfigureViewfinder();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();
	auto timelapse_time = start_time;

	for (unsigned int count = 0; ; count++)
	{
		LibcameraApp::Msg msg = app.Wait();
		if (msg.type == LibcameraApp::MsgType::Quit)
			return;
		else if (msg.type != LibcameraApp::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");

		auto now = std::chrono::high_resolution_clock::now();
		int key = get_key_or_signal(options, p);
		if (key == 'x' || key == 'X')
			return;

		// In viewfinder mode, simply run until the timeout. When that happens, switch to
		// capture mode if an output was requested.
		if (app.ViewfinderStream())
		{
			std::cerr << "DUSTIN  ViewfinderStream" << std::endl;
			if (options->verbose)
				std::cerr << "Viewfinder frame " << count << std::endl;

			bool timed_out = options->timeout && now - start_time > std::chrono::milliseconds(options->timeout);
			bool keypressed = key == '\n';
			bool timelapse_timed_out = options->timelapse &&
									   now - timelapse_time > std::chrono::milliseconds(options->timelapse);

			if (timed_out || keypressed || timelapse_timed_out)
			{
				// Trigger a still capture unless:
				if (!output || // we have no output file
					(timed_out && options->timelapse) || // timed out in timelapse mode
					(!keypressed && keypress)) // no key was pressed (in keypress mode)
					return;
				else
				{
					timelapse_time = std::chrono::high_resolution_clock::now();
					app.StopCamera();
					app.Teardown();
					app.ConfigureStill(still_flags);
					app.StartCamera();
				}
			}
			else
			{
				CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
				app.ShowPreview(completed_request, app.ViewfinderStream());
			}
		}
		// In still capture mode, save a jpeg. Go back to viewfinder if in timelapse mode,
		// otherwise quit.
		else if (app.StillStream())
		{
			app.StopCamera();
			std::cerr << "Still capture image received" << std::endl;
			save_images(app, std::get<CompletedRequestPtr>(msg.payload));
			if (options->timelapse || options->signal || options->keypress)
			{
				app.Teardown();
				app.ConfigureViewfinder();
				app.StartCamera();
			}
			else
				return;
		}
	}
}

static void Video(std::unique_ptr<Output> & output) {
	std::cerr << "VIDEO START" << std::endl;
	LibcameraEncoder app;
	VideoOptions *options = app.GetOptions(); // NEED TO SET OPTIONS BASED ON FILE INPUT PARAMETERS
	options->Parse(global_argc, global_argv);
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
	app.StartEncoder();
	app.OpenCamera();
	app.ConfigureVideo();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	for (unsigned int count = 0; ; count++)
	{
		LibcameraEncoder::Msg msg = app.Wait();
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			break;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		int key = get_key_or_signal(options, p);
		if (key == '\n')
			output->Signal();

		if (options->verbose)
			std::cerr << "Viewfinder frame " << count << std::endl;
		auto now = std::chrono::high_resolution_clock::now();
		bool timeout = !options->frames && options->timeout &&
					   (now - start_time > std::chrono::milliseconds(options->timeout));
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout || key == 'x' || key == 'X')
		{
			app.StopCamera(); // stop complains if encoder very slow to close
			app.StopEncoder();
			break;
		}
		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBuffer(completed_request, app.VideoStream());
	}
	output->WriteOut();
	std::cerr << "VIDEO END" << std::endl;
}

int main(int argc, char *argv[])
{
	try
	{
		global_argc = argc;
		global_argv = argv;
		signal(SIGUSR1, default_signal_handler);
		signal(SIGUSR2, default_signal_handler);
		std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create());
		while (true) 
		{
			if (signal_received == 10) // TWO SIGNALS SHOULD BE: 1 CHECK FILE FOR MODE AND PARAMETERS, 2 STOP CURRENT CAPTURE
			{
				std::cerr << "HERE1" << std::endl;
				signal_received = 0;
				Video(output);
			} else if (signal_received == 12)
			{
				std::cerr << "HERE2" << std::endl;
				signal_received = 0;
				Still();
			}
			// std::cerr << "SLEEPING FOR 1 SEC" << std::endl;
			std::this_thread::sleep_for (std::chrono::milliseconds(10));
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
		return -1;
	}
	return 0;
}