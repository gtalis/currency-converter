#
# CurrencyConverter
#
project(currencyconverter C CXX)

cmake_minimum_required(VERSION 2.8)

set (VERSION_MAJOR	0)
set (VERSION_MINOR	1)
set (VERSION_PATCH	0)

SET(SW_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
add_definitions(-DVERSION="${SW_VERSION}" -Wall -Werror)

# Set C++11 as a minimum required standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


###############################################################################
#
# dependencies
#
###############################################################################

find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(CURL libcurl REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})

add_executable (currencyconverter src/rateManager.cpp src/utils.cpp src/main.cpp)
target_link_libraries(currencyconverter ${CURL_LIBRARIES})
target_link_libraries(currencyconverter ${LIBXML2_LIBRARIES})

