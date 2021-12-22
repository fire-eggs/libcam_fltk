/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * control_output.hpp
 */

#pragma once

#include "output.hpp"

class ControlOutput : public Output
{
	public:
		ControlOutput();
		~ControlOutput();
		void WriteOut();
		void Reset();
		void Initialize();
		void ConfigTimestamp();

	protected:
		void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

	private:
		std::array<std::byte[18677760], 360> buf_;
		std::atomic_uint framesBuffered_;
		std::atomic_uint framesWritten_;
		FILE *fp_;
};
