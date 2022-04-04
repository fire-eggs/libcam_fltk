/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "zoom" tab: GUI definition, callback functions, and inter-thread communication.
 */

#ifndef LIBCAMFLTK_ZOOM_H
#define LIBCAMFLTK_ZOOM_H

// zoom settings: declaration
extern double _zoom;
extern double _panH;
extern double _panV;
extern int _zoomChoice;
extern bool _lever;

extern void onStateChange();

#endif //LIBCAMFLTK_ZOOM_H
