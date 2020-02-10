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

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5Core REQUIRED)
find_package(Qt5LinguistTools)
find_package(KF5SyntaxHighlighting REQUIRED)

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

add_executable(srccat "")
target_sources(srccat PRIVATE ${srccat_SOURCES} ${srccat_HEADERS})
target_link_libraries(srccat
    PRIVATE Qt5::Core KF5::SyntaxHighlighting
)

target_compile_options(srccat PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra>
)

target_compile_features(srccat PRIVATE
    cxx_auto_type
    cxx_generalized_initializers
    cxx_lambdas
    cxx_range_for
    cxx_uniform_initialization
)

if(Qt5LinguistTools_FOUND)
    add_subdirectory(i18n)
endif()

install(TARGETS srccat RUNTIME DESTINATION bin)
