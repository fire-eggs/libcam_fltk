/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * control_output.cpp
 */

#include <thread>
#include <pthread.h>
#include <chrono>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "core/control.hpp"
#include "control_output.hpp"
using namespace std::chrono;

// We're going to align the frames within the buffer to friendly byte boundaries
// static constexpr int ALIGN = 16; // power of 2, please

FILE *Control::pipe;
int Control::frames;
bool Control::enableBuffer;

ControlOutput::ControlOutput() : Output(), buf_(), framesBuffered_(0), framesWritten_(0)
{
	char * myfifo = new char [14];
	strcpy(myfifo, "/home/pi/pipe");
	mkfifo(myfifo, 0666);
	std::cerr << "PIPE CREATED" << std::endl;
}

ControlOutput::~ControlOutput()
{

}

void ControlOutput::WriteOut()
{
	if (fp_timestamps_)
		fclose(fp_timestamps_);
	fp_timestamps_ = nullptr;
	if (Control::enableBuffer) 
	{
		while(framesWritten_ < framesBuffered_)
		{
			if (fwrite(buf_[framesWritten_], 18677760, 1, Control::pipe) != 1)
				std::cerr << "failed to write output bytes" << std::endl;
			else
			{
				std::cerr << "Frames Written: " << (framesWritten_+1) << ", Frames Buffered: " << framesBuffered_ << std::endl;
				framesWritten_++;
			}
		}
	}
}

void ControlOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
	if (!Control::enableBuffer) 
	{	
		auto start = high_resolution_clock::now();
		if (fwrite(mem, size, 1, Control::pipe) != 1)
			throw std::runtime_error("failed to write output bytes");
		else
			framesWritten_++;
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cerr << "Write took: " << duration.count() << "ms" << std::endl;
	}
	else 
	{
		framesBuffered_++;
		auto start = high_resolution_clock::now();
		memcpy(&buf_[framesBuffered_ - 1], mem, 18677760); // NEED TO PAD/ALIGN TO 4096
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cerr << "Copy took: " << duration.count() << "ms, Frames Buffered: " << framesBuffered_ << std::endl;
	}
}

void ControlOutput::Reset()
{
	std::cerr << "RESETTING BUFFER" << std::endl;
	framesWritten_ = 0;
	framesBuffered_ = 0;
	flags = 2;
	state_ = WAITING_KEYFRAME;
	last_timestamp_ = 0;
	// if (fp_)
	// 	fclose(fp_);
	if (!Control::pipe) {
		Control::pipe = fopen("/home/pi/pipe", "w");
		std::cerr << "PIPE OPENED BY CONSUMER" << std::endl;
	}
	if (Control::mode == 2 && !Control::timestampsFile.empty())
	{
		fp_timestamps_ = fopen(Control::timestampsFile.c_str(), "w");
		if (!fp_timestamps_)
			throw std::runtime_error("Failed to open timestamp file " + Control::timestampsFile);
		fprintf(fp_timestamps_, "# timecode format v2\n");
	}
}