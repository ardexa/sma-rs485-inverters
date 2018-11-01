#
# Copyright (c) 2013-2018 Ardexa Pty Ltd
#
# This code is licensed under the MIT License (MIT).
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# CMAKE file for the Ardexa plugin to read an SMA inverter
#
# Notes:
# 1.	yasdi development kit should be installed at the same directory level as this git proj in a directory called 'sma'
#		..eg; in the home directory, in a subdir called 'sma'
# 2.	../sma/include/packet.h Line 38: struct TDevice * Device; should be struct _TDevice * Device;
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Set the project name
PROJECT(ardexa-sma)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR .)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Source files
set(ARDEXA_SMA_SRC
    src/main.cpp
    src/arguments.cpp
    src/utils.cpp
)

# Include directories
include_directories(
	../sma/include
	../sma/core
	../sma/smalib
	../sma/os
	../sma/protocol
	../sma/master
	../sma/libs
	../sma/projects/generic-cmake/incprj
)

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x -fmessage-length=0")
	set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -fmessage-length=0 ")
endif()

# Build the application
add_executable(ardexa-sma ${ARDEXA_SMA_SRC})
TARGET_LINK_LIBRARIES(ardexa-sma dl pthread yasdi yasdimaster)

# add the install targets
install (TARGETS ardexa-sma DESTINATION /usr/local/bin)
