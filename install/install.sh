#!/bin/sh
# NOTE: assumes files have been packed with tar to preserve permissions
#
mkdir -p /usr/local/share/libcam_fltk
cp libcam_fltk.sh /usr/local/bin/libcam_fltk
mkdir -p /usr/local/share/applications
cp libcam_fltk.desktop /usr/local/share/applications
cp * /usr/local/share/libcam_fltk
