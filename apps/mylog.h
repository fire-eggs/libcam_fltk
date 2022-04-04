/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * Logging functions using spdlog.
 */

#ifndef LIBCAMFLTK_MYLOG_H
#define LIBCAMFLTK_MYLOG_H

void initlog();
void dolog(const char *, ...);

#endif //LIBCAMFLTK_MYLOG_H
