cmake_minimum_required(VERSION 3.12)
project(CustomHeadsetOpenVR LANGUAGES CXX C)

# Find all .cpp files in the src directory and store them in a variable
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Add a library target using all the found source files
ADD_LIBRARY(driver_CustomHeadsetOpenVR SHARED
	${SOURCE_FILES}
	../ThirdParty/easywsclient/easywsclient.cpp
	../ThirdParty/hidapi/linux/hid.c
	# ../ThirdParty/hidapi/libusb/hid.c
	../ThirdParty/zlib/adler32.c
	../ThirdParty/zlib/compress.c
	../ThirdParty/zlib/crc32.c
	../ThirdParty/zlib/deflate.c
	../ThirdParty/zlib/gzclose.c
	../ThirdParty/zlib/gzlib.c
	../ThirdParty/zlib/gzread.c
	../ThirdParty/zlib/gzwrite.c
	../ThirdParty/zlib/infback.c
	../ThirdParty/zlib/inffast.c
	../ThirdParty/zlib/inflate.c
	../ThirdParty/zlib/inftrees.c
	../ThirdParty/zlib/trees.c
	../ThirdParty/zlib/uncompr.c
	../ThirdParty/zlib/zutil.c
)

target_include_directories(driver_CustomHeadsetOpenVR PUBLIC ../ThirdParty/json/include)
target_include_directories(driver_CustomHeadsetOpenVR PUBLIC ../ThirdParty/openvr/headers)
target_include_directories(driver_CustomHeadsetOpenVR PRIVATE ../ThirdParty/hidapi/hidapi)
target_include_directories(driver_CustomHeadsetOpenVR PRIVATE ../ThirdParty/zlib)

set_target_properties(driver_CustomHeadsetOpenVR PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Needed for zlib POSIX functions (close, fwrite, etc.)
target_compile_definitions(driver_CustomHeadsetOpenVR PRIVATE _GNU_SOURCE)

# Link against system libraries needed by hidapi (libusb, libudev)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
pkg_check_modules(LIBUDEV REQUIRED libudev)
target_include_directories(driver_CustomHeadsetOpenVR PRIVATE ${LIBUSB_INCLUDE_DIRS} ${LIBUDEV_INCLUDE_DIRS})
target_link_libraries(driver_CustomHeadsetOpenVR ${LIBUSB_LIBRARIES} ${LIBUDEV_LIBRARIES} pthread)

add_custom_command(TARGET driver_CustomHeadsetOpenVR POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rm -rf CustomHeadsetOpenVR
	COMMAND ${CMAKE_COMMAND} -E make_directory CustomHeadsetOpenVR/bin/linux64
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/DriverFiles CustomHeadsetOpenVR
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:driver_CustomHeadsetOpenVR> CustomHeadsetOpenVR/bin/linux64/driver_CustomHeadsetOpenVR.so
)

# cmake ../ ; make -j16 ; rsync -a --delete CustomHeadsetOpenVR/ ~/.steam/steam/steamapps/common/SteamVR/drivers/CustomHeadsetOpenVR/