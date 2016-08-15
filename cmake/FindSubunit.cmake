# - Try to find the subunit libraries
#  Once done this will define
#
#  SUBUNIT_INCLUDE_DIRS - the subunit include directory
#  SUBUNIT_LIBRARIES - subunit library
#

find_path(SUBUNIT_INCLUDE_DIRS subunit/child.h)
find_library(SUBUNIT_LIBRARIES NAMES subunit)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Subunit REQUIRED_VARS SUBUNIT_INCLUDE_DIRS SUBUNIT_LIBRARIES)
mark_as_advanced(SUBUNIT_INCLUDE_DIRS SUBUNIT_LIBRARIES)
