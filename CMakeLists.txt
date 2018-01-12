cmake_minimum_required( VERSION 3.6 )
project( "C Utility" 
	VERSION 0.1.0.0 
	DESCRIPTION "Structures and Utilities common to most applications but not native to C"
	LANGUAGES C CXX
)

# Check for required libraries and headers
include(CheckIncludeFile)

CHECK_INCLUDE_FILE( "stdbool.h" HAVE_STDBOOL_H )
if( NOT HAVE_STDBOOL_H )
	message( FATAL_ERROR "stdbool.h is required by the listing structure which is used everywhere" )
endif( NOT HAVE_STDBOOL_H )

CHECK_INCLUDE_FILE( "string.h" HAVE_STRING_H )
if( NOT HAVE_STRING_H )
	message( FATAL_ERROR "string.h contains core memory functions that are required by everything" )
endif( NOT HAVE_STRING_H )

find_package( Threads )
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
if( NOT CMAKE_USE_PTHREADS_INIT )
	message( FATAL_ERROR "Library currently only compatable with POSIX Threads" )
endif( NOT CMAKE_USE_PTHREADS_INIT )

# force C compiler for C sources and C++ compiler for C++ sources
#   (apperently C++ compiler superceeds C compiler by default)
set_property( SOURCE *.c PROPERTY LANGUAGE C )
set_property( SOURCE *.cpp PROPERTY LANGUAGE CXX )

# Include GDB Macro metadata in debug build
set( CMAKE_C_FLAGS_DEBUG "-g3" )

add_library( listing STATIC 
	"listing.c"
	"listing_foreach.c"
)

add_library( hashtree STATIC
	"hashtree.c"
	"hashtree_foreach.c"
)

add_library( bitstring STATIC
	"bitstring.c"
)

add_library( tabulation STATIC
	"tabulation.c"
)

target_link_libraries( tabulation 
	PUBLIC listing
	PUBLIC hashtree
)

add_executable( bitstring_test 
	bitstring_test.cpp
)

target_link_libraries( bitstring_test
	PUBLIC bitstring
)
