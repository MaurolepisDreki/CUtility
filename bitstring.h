/// @file
/*******************************************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> BitString <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< *
 *******************************************************************************
 * Compressed Array of Bits primarily used for flags as a replacement for such *
 *  methods as using an integer as a flag placeholder.  Advantages of using    *
 *  this system (over using static types) include the simplification of        *
 *  setting and reading flags and dynamic allocation of memory; by dynamically *
 *  allocating memory for flags we cease to be limited to however many bits    *
 *  are in the type of our choosing (eg unsigned int) making it easier to have *
 *  user defined flags in such systems as our event engine.                    *
 ******************************************************************************/

#ifndef INCLUDED_BITSTRING_H
#define INCLUDED_BITSTRING_H

#include <stdbool.h>

typedef unsigned char byte;
typedef struct {
	int count;
	byte *data;
} BitString;

/**
 * Initialize the BitString
 *   @param bs		The BitString to initialize
 *   @param len	The initial number of bits represented
 */
void BitString_Init( BitString *bs, int len );

/**
 * Cleanup the BitString
 *   @param bs		The BitString to be cleaned
 */
void BitString_Free( BitString *bs );

/**
 * Retrieve the bit-count from the BitString
 *   bs := the BitString to read
 */
int BitString_Count( BitString *bs );

/**
 * Set/Get individual bits in the BitString
 *   @param bs		The BitString to access
 *   @param index	The index of the bit to access (starts at 0)
 *   @param value	The value of the bit accessed (Set Only)
 *   @return		The value of the bit accessed (Get Only)
 */
void BitString_Set( BitString *bs, int index, bool value );
bool BitString_Get( BitString *bs, int index );

/**
 * Copy bits between BitStrings 
 *   @param destBS		Destination BitString
 *   @param origBS		Origin BitString
 *   @param destIndex	Destination start bit
 *   @param origIndex	Origin start bit
 *   @param count			Number of bits to copy
 */
void BitString_Copy( BitString *destBS, int destIndex, BitString *origBS, int origIndex, int count );

/**
 * Fill range of bits in BitString
 *   @param bs		The BitString to interact with
 *   @param index	The first bit to set
 *   @param count	The number of bits to set
 *   @param value	The value the bits should contain
 */
void BitString_Fill( BitString *bs, int index, int count, bool value );

/* Insert/Remove bits in the BitString */


#endif
 
