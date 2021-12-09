/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * buffer_output.cpp
 */

#include "buffer_output.hpp"
#include <thread>
#include <pthread.h>
#include <chrono>
using namespace std::chrono;

// We're going to align the frames within the buffer to friendly byte boundaries
// static constexpr int ALIGN = 16; // power of 2, please

BufferOutput::BufferOutput(VideoOptions const *options) : Output(options), buf_(), framesBuffered_(1), framesWritten_(0), lastWriteTime_(0)
{
	if (options_->output == "-")
		fp_ = stdout;
	else if (!options_->output.empty())
	{
		fp_ = fopen(options_->output.c_str(), "w");
	}
	if (!fp_)
		throw std::runtime_error("could not open output file");

	std::thread t1(WriterThread, std::ref(*this));
	t1.detach();
}

BufferOutput::~BufferOutput()
{
	std::cerr << "GOODBYE" << std::endl;
	CloseFile();
}

void BufferOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
	auto start = high_resolution_clock::now();
	memcpy(&buf_[framesBuffered_], mem, 18677760); // Need to pad/align to 4096
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);
	std::cerr << "Copy took: " << duration.count() << "ms framesBuffered: " << framesBuffered_ << " framesWritten: " << framesWritten_ << " lastWriteTime(broken): " << lastWriteTime_ << std::endl;
	framesBuffered_++;
}

void BufferOutput::CloseFile()
{
	if (fp_ && fp_ != stdout)
		fclose(fp_);
	fp_ = nullptr;
}

void BufferOutput::WriterThread(BufferOutput &obj) // NEED A WAY TO HOLD PROGRAM EXIT UNTIL WRITER THREAD IS DONE
{
	// NOT SURE THIS DOES ANYTHING
	int policy;
    struct sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    param.sched_priority = sched_get_priority_min(policy);
    pthread_setschedparam(pthread_self(), policy, &param);

	while (obj.fp_ != nullptr)
	{
		obj.lastWriteTime_ = 0;
		while(obj.framesBuffered_ - obj.framesWritten_ > 0)
		{
			auto start = high_resolution_clock::now();
			if (fwrite(obj.buf_[obj.framesBuffered_ - 1], 18677760, 1, obj.fp_) != 1) // NEED TO % 300
				throw std::runtime_error("failed to write output bytes");
			else
				obj.framesWritten_++;
			auto stop = high_resolution_clock::now();
			auto duration = duration_cast<milliseconds>(stop - start);
			obj.lastWriteTime_ = obj.lastWriteTime_ + duration.count();
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(100));
	}
	std::cerr << "NULL!!!" << std::endl;
}