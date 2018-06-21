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

# Don't delete .ts files when running "make clean".
# WARNING: This workaround does not work with the ninja generator
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

set(srccat_TRANSLATIONS
    srccat_en.ts
)

function(prefix_srcdir outvar prefix)
    set(outlist "")
    foreach(path ${ARGN})
        list(APPEND outlist "${prefix}/${path}")
    endforeach()
    set(${outvar} "${outlist}" PARENT_SCOPE)
endfunction()

prefix_srcdir(srccat_TS_SOURCES ${CMAKE_SOURCE_DIR} ${srccat_SOURCES} ${srccat_HEADERS})
qt5_create_translation(srccat_LUPDATE ${srccat_TS_SOURCES} ${srccat_TRANSLATIONS})
add_custom_target(lupdate DEPENDS ${srccat_LUPDATE})

qt5_add_translation(srccat_QM ${srccat_TRANSLATIONS})
add_custom_target(translations DEPENDS ${srccat_QM})
add_dependencies(srccat translations)

configure_file(srccat_i18n.qrc.in srccat_i18n.qrc COPYONLY)
target_sources(srccat PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/srccat_i18n.qrc")
