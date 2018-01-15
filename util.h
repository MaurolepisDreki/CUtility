/* Common Utilities for Basic Operations */
#include <stdlib.h>
#include <stdio.h>

#ifndef INCLUDED_UTIL_H
#define INCLUDED_UTIL_H

/* Forced Malloc: do-or-die malloc for mission-critical components
 *   raises SIGABRT when malloc fails
 *  TODO: should like a recoverable way to do the same thing
 */
#define fmalloc( ptr, size ) {\
	void *tp = malloc( size );\
	if( tp == NULL ) {\
		fprintf( stderr, "Malloc failed at '%s:%i'", __FILE__, __LINE__ );\
		abort();\
	}\
	ptr = tp;\
}

#endif
