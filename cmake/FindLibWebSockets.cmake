# This module tries to find libWebsockets library and include files
#
# LIBWEBSOCKETS_INCLUDE_DIRS, path where to find libwebsockets.h
# LIBWEBSOCKETS_LIBRARIES, the library to link against
# LIBWEBSOCKETS_FOUND, If false, do not try to use libWebSockets
#

find_path(LIBWEBSOCKETS_INCLUDE_DIRS libwebsockets.h)
find_library(LIBWEBSOCKETS_LIBRARIES websockets)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibWebSockets REQUIRED_VARS LIBWEBSOCKETS_INCLUDE_DIRS LIBWEBSOCKETS_LIBRARIES)
mark_as_advanced(LIBWEBSOCKETS_INCLUDE_DIRS LIBWEBSOCKETS_LIBRARIES)
