mkdir -p release
cp libcam_fltk release
cp ../install/* release
chmod +x release/install.sh release/libcam_fltk.sh release/libcam_fltk.desktop
find . -name "*.so" -print0 | xargs -0 -I foo cp foo release
cd release
tar -cf ../release.tar *
cd ..
gzip release.tar

