/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * buffer_output.hpp
 */

#pragma once

#include "output.hpp"

class BufferOutput : public Output
{
	public:
		BufferOutput(VideoOptions const *options);
		~BufferOutput();

	protected:
		void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

	private:
		void CloseFile();
		static void WriterThread(BufferOutput &obj);
		std::array<std::byte[18495360], 300> buf_;
		size_t framesBuffered_;
		size_t framesWritten_;
		size_t lastWriteTime_;
		FILE *fp_;
};
