cmake_minimum_required(VERSION 3.6)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/spdlog .)

project(libcamera-still)
add_executable(libcamera-still libcamera_still.cpp)
target_link_libraries(libcamera-still libcamera_app images)

project(libcamera-vid)
add_executable(libcamera-vid libcamera_vid.cpp)
target_link_libraries(libcamera-vid libcamera_app encoders outputs)

project(libcamera-hello)
add_executable(libcamera-hello libcamera_hello.cpp)
target_link_libraries(libcamera-hello libcamera_app)

project(libcamera-raw)
add_executable(libcamera-raw libcamera_raw.cpp)
target_link_libraries(libcamera-raw libcamera_app encoders outputs)

project(libcamera-jpeg)
add_executable(libcamera-jpeg libcamera_jpeg.cpp)
target_link_libraries(libcamera-jpeg libcamera_app images)

project(libcam_fltk)
add_executable(libcam_fltk libcam_fltk.cpp camThread.cpp timelapse.cpp capture.cpp settings.cpp
               zoom.cpp calc.cpp FL_Flex/FL_Flex.cpp mylog.cpp about.cpp timeentry.cpp advanced.cpp
               servoTab.cpp servoThread.cpp bcm283x_board_driver.c PCA9685_servo_driver.c
               sccb_bus.c)
               
target_link_libraries(libcam_fltk libcamera_app encoders outputs fltk m X11 Xext pthread Xinerama
                      Xfixes Xcursor Xft Xrender m fontconfig dl)

set(EXECUTABLES libcamera-still libcamera-vid libcamera-hello libcamera-raw libcamera-jpeg libcam_fltk)

if (ENABLE_TFLITE)
    project(libcamera-detect)
    add_executable(libcamera-detect libcamera_detect.cpp)
    target_link_libraries(libcamera-detect libcamera_app images)
    set(EXECUTABLES ${EXECUTABLES} libcamera-detect)
endif()

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
install(TARGETS ${EXECUTABLES} RUNTIME DESTINATION bin)
