/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * control.hpp
 */

#pragma once

class Control
{
public:
	Control() = default;
    static int mode;
	static bool enableBuffer;
	static int frames;
    static std::string timestampsFile;
};
