cmake_minimum_required(VERSION 3.12) # Specify a modern CMake version
project(MyProject LANGUAGES CXX)

# Find all .cpp files in the src directory and store them in a variable
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Add a library target using all the found source files
ADD_LIBRARY(driver_CustomHeadsetOpenVR SHARED ${SOURCE_FILES} ../ThirdParty/easywsclient/easywsclient.cpp)

target_include_directories(driver_CustomHeadsetOpenVR PUBLIC ../ThirdParty/json/include)
target_include_directories(driver_CustomHeadsetOpenVR PUBLIC ../ThirdParty/openvr/headers)

set_target_properties(driver_CustomHeadsetOpenVR PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_custom_command(TARGET driver_CustomHeadsetOpenVR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf CustomHeadsetOpenVR
    COMMAND ${CMAKE_COMMAND} -E make_directory CustomHeadsetOpenVR/bin/linux64
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/DriverFiles CustomHeadsetOpenVR
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:driver_CustomHeadsetOpenVR> CustomHeadsetOpenVR/bin/linux64/driver_CustomHeadsetOpenVR.so
)

# cmake ../ ; make -j16 ; rsync -a --delete CustomHeadsetOpenVR/ ~/.steam/steam/steamapps/common/SteamVR/drivers/CustomHeadsetOpenVR/