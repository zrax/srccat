# This file is part of srccat.
# Copyright (c) 2017 Michael Hansen
#
# srccat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# srccat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with srccat.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.1)
project(srccat)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_ID MATCHES ".*Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt5Core REQUIRED)
find_package(Qt5LinguistTools)
find_package(KF5SyntaxHighlighting REQUIRED)
find_package(Magic REQUIRED)

set(CMAKE_AUTORCC ON)

set(srccat_SOURCES
    srccat.cpp
    esc_highlight.cpp
    esc_color.cpp
)

set(srccat_HEADERS
    esc_highlight.h
    esc_color.h
)

if(NOT WIN32)
    set(srccat_SOURCES ${srccat_SOURCES} pager.cpp)
    set(srccat_HEADERS ${srccat_HEADERS} pager.h)
endif()

include_directories(${Magic_INCLUDE_DIR})

add_executable(srccat ${srccat_SOURCES} ${srccat_HEADERS})
target_link_libraries(srccat Qt5::Core KF5::SyntaxHighlighting ${Magic_LIBRARY})

if(Qt5LinguistTools_FOUND)
    add_subdirectory(i18n)
endif()

install(TARGETS srccat RUNTIME DESTINATION bin)
