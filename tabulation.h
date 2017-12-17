/* AUTHOR:   Nile Aagard
 * CREATED:  2017-11-02
 * MODIFIED: 2017-11-03
 * DESCRIPTION: A control structure for superficialy mimmicking queryable tabulated data */
 
#ifndef INCLUDED_TABULATION_H
#define INCLUDED_TABULATION_H

#include "listing.h"
#include "hashtree.h"

/* C-string (null terminated char array) returning hashing routine */
typedef void (*Table_Index_Hasher)( void *data, void **hash, size_t *hashlen );
typedef void (*Table_Index_Hasher_Clean)( void **hash );
typedef HashTree Table; /* HashTrees are the basic structure for our table */

/* Nothing Special about Table Initialization,
 *   just use base Initializer.       */
#define Table_Init() HashTree_Init()

/* Table destructor (not simply a wrapper) */
void Table_Free( Table *tab );

/* Create a table index */
void Table_CreateIndex( Table, const char *index, Table_Index_Hasher, Table_Index_Hasher_Clean );

/* Remove a table index */
void Table_DropIndex( Table, const char *index );

/* Add an entry to the table */
void Table_Add( Table, void *data );

/* Remove an entry from the table */
void Table_Del( Table, void *data );

/* Query data in the table */
Listing Table_Query( Table, const char *index, void *data );

#endif
