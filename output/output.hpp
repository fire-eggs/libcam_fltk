/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * output.hpp - video stream output base class
 */

#pragma once

#include <cstdio>

#include <atomic>

#include "core/video_options.hpp"

class Output
{
public:
	static Output *Create(VideoOptions const *options);
	static Output *Create();

	Output(VideoOptions const *options);
	Output();
	virtual ~Output();
	virtual void Signal(); // a derived class might redefine what this means
	virtual void WriteOut();
	virtual void Reset();
	virtual void Initialize();
	virtual void ConfigTimestamp();
	void OutputReady(void *mem, size_t size, int64_t timestamp_us, bool keyframe);

protected:
	enum Flag
	{
		FLAG_NONE = 0,
		FLAG_KEYFRAME = 1,
		FLAG_RESTART = 2
	};
	enum State
	{
		DISABLED = 0,
		WAITING_KEYFRAME = 1,
		RUNNING = 2
	};
	virtual void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags);
	VideoOptions const *options_;
	FILE *fp_timestamps_;
	uint32_t flags;
	State state_;
	int64_t last_timestamp_;

private:
	std::atomic<bool> enable_;
	int64_t time_offset_;
};
