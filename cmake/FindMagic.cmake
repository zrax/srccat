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

find_path(Magic_INCLUDE_DIR magic.h)
find_library(Magic_LIBRARY magic)

mark_as_advanced(Magic_INCLUDE_DIR Magic_LIBRARY)

# LibMagic uses a numeric version (e.g. 524 == 5.24).
# This probably could be parsed and stored, but srccat doesn't need it (yet)
# so we just assume that finding it is sufficient for now.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Magic
    REQUIRED_VARS Magic_INCLUDE_DIR Magic_LIBRARY
)

if(Magic_FOUND AND NOT TARGET Magic::Magic)
    add_library(Magic::Magic UNKNOWN IMPORTED)
    set_target_properties(Magic::Magic PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${Magic_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Magic_INCLUDE_DIR}"
    )
endif()
