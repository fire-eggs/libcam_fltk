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
		BufferOutput();
		~BufferOutput();
		void WriteOut();

	protected:
		void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

	private:
		void CloseFile();
		void Reset();
		static void WriterThread(BufferOutput &obj);
		std::array<std::byte[18677760], 50> buf_;
		std::atomic_uint framesBuffered_;
		std::atomic_uint framesWritten_;
		FILE *fp_;
};
