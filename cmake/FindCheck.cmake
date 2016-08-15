# - Try to find the CHECK libraries
#  Once done this will define
#
#  CHECK_INCLUDE_DIRS - the check include directory
#  CHECK_LIBRARIES - check library
#

find_path(CHECK_INCLUDE_DIRS check.h)
find_library(CHECK_LIBRARIES NAMES check)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Check REQUIRED_VARS CHECK_INCLUDE_DIRS CHECK_LIBRARIES)
mark_as_advanced(CHECK_INCLUDE_DIRS CHECK_LIBRARIES)
