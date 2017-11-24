add_executable (multiprojector_calibration digicam_control.cpp sl_multiproj.cpp)

add_subdirectory (checkerboard_generator)
add_subdirectory (blend_mask_generator)
add_subdirectory (GUI)
target_link_libraries (multiprojector_calibration opencv_highgui opencv_core opencv_imgproc opencv_calib3d gphoto2 jpeg)
