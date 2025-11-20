#[=======================================================================[.rst:
FindExternalLib
---------------

This is a starter template showing how to write simple CMake ``find_package``
modules. Copy this file, rename it (e.g. ``FindFoo.cmake``), and update the
placeholders below to locate the headers and libraries for your dependency.

Variables to set:

``ExternalLib_ROOT``
  Optional manual hint that points to the dependency install prefix.

Output variables:

``ExternalLib_FOUND``
  Indicates whether the dependency was discovered.
``ExternalLib_INCLUDE_DIRS``
  Include directories that should be added to ``target_include_directories``.
``ExternalLib_LIBRARIES``
  Libraries to pass to ``target_link_libraries``.

Usage example::

  find_package(ExternalLib REQUIRED)
  target_include_directories(prism_tui PRIVATE ${ExternalLib_INCLUDE_DIRS})
  target_link_libraries(prism_tui PRIVATE ${ExternalLib_LIBRARIES})

###########################################################################
# Implementation starts below                                             #
###########################################################################
#]=======================================================================]

# Clear old cache entries so re-configurations pick up changes.
unset(ExternalLib_INCLUDE_DIR CACHE)
unset(ExternalLib_LIBRARY CACHE)

# 1. Search for headers. Adjust the names to match your library.
find_path(ExternalLib_INCLUDE_DIR
    NAMES external_lib.h
    HINTS ${ExternalLib_ROOT}
    PATH_SUFFIXES include
)

# 2. Search for the compiled library/binary artifact.
find_library(ExternalLib_LIBRARY
    NAMES external_lib
    HINTS ${ExternalLib_ROOT}
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ExternalLib
    REQUIRED_VARS ExternalLib_INCLUDE_DIR ExternalLib_LIBRARY
)

if(ExternalLib_FOUND)
    set(ExternalLib_INCLUDE_DIRS ${ExternalLib_INCLUDE_DIR})
    set(ExternalLib_LIBRARIES ${ExternalLib_LIBRARY})
endif()

mark_as_advanced(ExternalLib_INCLUDE_DIR ExternalLib_LIBRARY)
