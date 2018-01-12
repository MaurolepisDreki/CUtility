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

/* Initialize the BitString
 *   bs  := the BitString to initialize
 *   len := the initial number of bits represented
 */
void BitString_Init( BitString *bs, int len );

/* Cleanup the BitString
 *   bs := the BitString to be cleaned
 */
void BitString_Free( BitString *bs );

/* Retrieve the bit-count from the BitString
 *   bs := the BitString to read
 */
int BitString_Count( BitString *bs );

/* Set/Get individual bits in the BitString
 *   bs     := The BitString to access
 *   index  := the index of the bit to access (starts at 0)
 *   value  := the value of the bit accessed (Set Only)
 *   return := the value of the bit accessed (Get Only)
 */
void BitString_Set( BitString *bs, int index, bool value );
bool BitString_Get( BitString *bs, int index );

/* Copy bits between BitStrings 
 *   destBS    := destination BitString
 *   origBS    := origin BitString
 *   destIndex := destination start bit
 *   origIndex := origin start bit
 *   count     := number of bits to copy
 */
void BitString_Copy( BitString *destBS, int destIndex, BitString *origBS, int origIndex, int count );

/* Fill range of bits in BitString
 *   bs    := the BitString to interact with
 *   index := the first bit to set
 *   count := the number of bits to set
 *   value := the value the bits should contain
 */
void BitString_Fill( BitString *bs, int index, int count, bool value );

/* Insert/Remove bits in the BitString */


#endif
 
