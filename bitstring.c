#include "bitstring.h"
#include <stdlib.h>
#include "util.h"

/* Calculation for the number of bytes needed to store our bits
 *   len := the number of bits we will require
 */
#define BitString_Bits2Bytes( len ) ((len % 8) ? len / 8 + 1 : len / 8)

/* Constructor */
void BitString_Init( BitString *bs, int len ) {
	bs->count = len;
	/*bs->data = malloc( BitString_Bits2Bytes( len ) );*/
	fmalloc( bs->data, BitString_Bits2Bytes( len ) );

	/* Zero-out allocated memory */
	for( int ibyte = 0; ibyte < BitString_Bits2Bytes( len ); ibyte++ )
		bs->data[ibyte] = 0x00;
};

/* Destructor */
void BitString_Free( BitString *bs ) {
	if( bs->data != NULL )
		free( bs->data );
	bs->data = NULL;
}

/* Count Accessor */
int BitString_Count( BitString *bs ) {
	return bs->count;
}

/* Common Assertion that the target bit is in the appropriate range of the 
 *    BitString 
 *   mybs   := the BitString to check
 *   target := the index of the target bit
 *   onfail := how to act when the assertion fails
 */
#define BitString_Assert_InRange( mybs, target, onfail ) \
	if( target < 0 || target >= mybs->count ) { onfail; }

/* Get Bit */
bool BitString_Get( BitString *bs, int index ) {
	BitString_Assert_InRange( bs, index, return false );
	return (bs->data[index / 8] & (0x01 << (7 - (index % 8)))) != 0x00 ;
}

/* Set Bit */
void BitString_Set( BitString *bs, int index, bool value ) {
	BitString_Assert_InRange( bs, index, return );
	if( value ) {
		bs->data[index / 8] |= 0x01 << (7 - (index % 8));
	} else {
		bs->data[index / 8] &= ~(0x01 << (7 - (index % 8)));
	}
}

#define OVERBYTE( bits ) ((bits) % 8)
#define UNDERBYTE( bits ) (8 - (OVERBYTE( bits ) ? OVERBYTE( bits ) : 8))
#define ACCESSED_BYTES( start, count ) (((count) + OVERBYTE( start )) / 8 + (OVERBYTE((count) + (start)) ? 1 : 0))
#define BYTEWISE_BITMASK( byte, start, count ) (( (byte) == 0 ? 0xFF >> OVERBYTE( start ) : 0xFF ) & ( (byte) == ACCESSED_BYTES( start, count ) - 1 ? 0xFF << UNDERBYTE( (start) + (count) ) : 0xFF ))

/* Fill range of bits */
void BitString_Fill( BitString *bs, int index, int count, bool value ) {
	/* fast method: byte at once */
	byte buff;
	for( int ibyte = 0; ibyte < ACCESSED_BYTES( index, count ); ibyte++ ) {
		buff = BYTEWISE_BITMASK(ibyte, index, count);
		
		if( value ) {
			bs->data[index / 8 + ibyte] |= buff;
		} else {
			bs->data[index / 8 + ibyte] &= ~buff;
		}
	}
}

/* Copy Range of bits */
void BitString_Copy( BitString *destBS, int destIndex, BitString *origBS, int origIndex, int count ) {
	if( destBS == origBS ) {
		/* DO NOT ATTEMPT TO COPY TO SELF!
		 * Instead, make a duplicate of self and copy from it.
		 * Or do not copy if not necessary. */
		if( destIndex != origIndex ) {
			BitString tmpBS;
			BitString_Init( &tmpBS, origBS->count );
			BitString_Copy( &tmpBS, 0, origBS, 0, tmpBS.count );
			BitString_Copy( destBS, destIndex, &tmpBS, origIndex, count );
			BitString_Free( &tmpBS );
		}
	} else {
		byte mask, buff;
		for( int ibyte = 0; ibyte < ACCESSED_BYTES(destIndex, count) && ibyte < ACCESSED_BYTES( origIndex, count ); ibyte++ ) {
			/* Create an 8-bit bit-mask */
			mask = BYTEWISE_BITMASK(ibyte, destIndex, count);
			
			/* Populate buffer from origin with byte aligned to destination */
			if( OVERBYTE( destIndex ) > OVERBYTE( origIndex ) ) {
				buff = origBS->data[ origIndex / 8 + ibyte ] >> (OVERBYTE( destIndex ) - OVERBYTE( origIndex ));
				if( ibyte > 0 ) 
					buff |= origBS->data[origIndex / 8 + ibyte - 1] << UNDERBYTE( OVERBYTE( destIndex ) - OVERBYTE( origIndex ) );
			} else if( OVERBYTE( destIndex ) < OVERBYTE( origIndex ) ) {
				buff = origBS->data[ origIndex / 8 + ibyte ] << (OVERBYTE( origIndex ) - OVERBYTE( destIndex ));
				if( ibyte < ACCESSED_BYTES( origIndex, count ) - 1 ) 
					buff |= origBS->data[origIndex / 8 + ibyte + 1] >> UNDERBYTE( OVERBYTE( origIndex ) - OVERBYTE( destIndex ) );
			} else {
				buff = origBS->data[ origIndex / 8 + ibyte ];
			}
			
			/* Clear unnecessary bits from the buffer */
			buff &= mask;
			
			/* Clear and set target bits */
			destBS->data[destIndex / 8 + ibyte] &= ~mask;
			destBS->data[destIndex / 8 + ibyte] |= buff;
		}
	}
}

