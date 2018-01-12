/* A short and temporary test for BitString verification */
#include <iostream>
#include <bitset>
#include <string>
#include <vector>
#include <regex>
#include <cstdlib>

extern "C" {
#include "bitstring.h"
}

using namespace std;

#define DISPLAY_DEBUG true

struct aTest {
	bool (*test)();
	string name;
};

ostream& operator<<( ostream& os, const BitString& bs ) {
	for( int ibyte = 0; ibyte < bs.count / 8 + (bs.count % 8 ? 1 : 0); ibyte++ ) {
		if( ibyte != 0 ) {
			os << " ";
		}
		
		if( ibyte < bs.count /  8 ) {
			os << bitset<8>( bs.data[ibyte] );
		} else {
			for( int ibit = 0; ibit < bs.count % 8; ibit++ )
				os << ((((bs.data[ibyte] << ibit) & 0x80) == 0x80) ? 1 : 0);
		}
	}
	return os;
}

void showhelp( const vector<aTest> &mytests ) {
	cout << "Tests:" << endl;
	cout << "\t0. All Tests" << endl;
	for( int ix = 0; ix < mytests.size(); ix++ ) {
		cout << "\t" << ix + 1 << ". " << mytests[ix].name << endl;
	}
}

void argproc( int argc, char **argv, const vector<aTest> &mytests, vector<int> &runtests ) {
	regex test("[0-9]+");
	regex help("-h|--help");
	cmatch m;
	
	vector<char *> fail;
	
	for( int ix = 1; ix < argc; ix++ ) {
		regex_match( argv[ix], m, test );
		if( !m.empty() ) {
			int id = atoi( argv[ix] );
			if( id <= mytests.size() ) {
				runtests.push_back( id );
			} else {
				fail.push_back( argv[ ix ] );
			}
		} else {
			regex_match( argv[ix], m, help );
			if( !m.empty() ) {
				showhelp( mytests );
				exit( EXIT_SUCCESS );
			} else {
				fail.push_back( argv[ix] );
			}
		}
	}
	
	for( int ie = 0; ie < fail.size(); ie++ ) {
		cerr << "ERROR: Invalid Test: `" << fail[ie] << "'" << endl;
	}
	
	if( fail.size() > 0 ) {
		cerr << "FATAL: Errors Occurred; Use -h or --help to list valid tests." << endl;
		exit( EXIT_FAILURE );
	}
}

#define CHECK( test, fmsg, res ) \
	if( !(test) ) { \
		if( DISPLAY_DEBUG ) \
			cout << "FAIL: " << fmsg << " (" #test ")" << endl; \
		res = false; \
	}

#define DISPL( msg, d ) \
	if( DISPLAY_DEBUG ) \
		cout << "(" << msg << "): " << d << endl;

// Basic Setup/Teardown test
bool test_init() {
	bool result = true;
	BitString t;
	
	BitString_Init( &t, 3 );
	
	CHECK( t.count == 3, "Bit count not stored corectly", result );
	CHECK( t.data != NULL, "Memory not allocated", result );
	DISPL( "3 bit string", t );
	
	BitString_Free( &t );
	CHECK( t.data == NULL, "Memeory not released",  result);

	return result;
}

// Bit Set/Get test
bool test_bits() {
	bool result = true;
	BitString t;

	BitString_Init( &t, 13 );
	DISPL( "13 bit string", t );

	/* Set bit 6 On*/
	BitString_Set( &t, 6, true );
	CHECK( t.data[0] == 0x02, "Bit 6 Not Set", result );
	CHECK( BitString_Get( &t, 6 ), "Bit 6 Not Set", result );
	DISPL( "bit 6 set", t);

	/* Fill bits 9 - 11 On */
	BitString_Fill( &t, 9, 3, true );
	CHECK( t.data[1] == 0x70, "Bits 9-11 not set", result );
	CHECK( BitString_Get( &t, 9 ), "Bit 9 not set", result );
	CHECK( BitString_Get( &t, 10 ), "Bit 10 not set", result );
	CHECK( BitString_Get( &t, 11 ), "Bit 11 not Set", result );
	DISPL( "Bits 9-11 set", t );

	/* Set bits 6 on (again), 10 off, and 3 off */
	BitString_Set( &t, 6, true );
	BitString_Set( &t, 10, false );
	BitString_Set( &t, 3, false );
	CHECK( t.data[0] == 0x02, "Bits 0-7 not set correctly", result );
	CHECK( t.data[1] == 0x50, "Bits 8-13 not set correctly", result );
	CHECK( BitString_Get( &t, 6 ), "Bit 6 not set correctly", result );
	CHECK( !BitString_Get( &t, 3 ), "Bit 3 not set correctly", result );
	CHECK( !BitString_Get( &t, 10 ), "Bit 10 not set correctly", result );
	DISPL( "bit 10 off", t );

	BitString_Free( &t );	
	return result;
}

// Bit Count Accessor Test
bool test_count() {
	bool result = true;
	vector<int> sizes;
	sizes.push_back( 3 );
	sizes.push_back( 13 );
	sizes.push_back( 26 );

	for( int its = 0; its < sizes.size(); its++ ) {
		BitString t;
		BitString_Init( &t, sizes[its] );

		CHECK( BitString_Count( &t ) == sizes[its], "BitString reported size other than alocated", result );

		BitString_Free( &t );
	}

	return result;
}

// Bit Copy Test
bool test_copy() {
	bool result = true;
	BitString t1, t2;

	// For simplicity, both BitStrings are the same length
	BitString_Init( &t1, 33 );
	BitString_Init( &t2, 33 );
	CHECK( t1.count == t2.count, "BitStrings are different lengths", result );
	CHECK( t1.data != t2.data, "BitStrings point to same data", result );
	DISPL( "t1", t1 );
	DISPL( "t2", t2 );

	// Setup t1
	BitString_Fill( &t1, 0, 1, true );
	BitString_Fill( &t1, 2, 3, true );
	BitString_Fill( &t1, 6, 3, true );
	BitString_Fill( &t1, 10, 6, true );
	BitString_Fill( &t1, 18, 4, true );
	BitString_Fill( &t1, 23, 1, true );
	BitString_Fill( &t1, 25, 1, true );
	BitString_Fill( &t1, 27, 5, true );
	CHECK( t1.data[0] == 0xBB, "Byte 0 incorrect", result );
	CHECK( t1.data[1] == 0xBF, "Byte 1 incorrect", result );
	CHECK( t1.data[2] == 0x3D, "Byte 2 incorrect", result );
	CHECK( t1.data[3] == 0x5F, "Byte 3 incorrect", result );
	CHECK( t1.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t1 := 0xBBBF3D5F0", t1 );
	
	/* Case 1: BitString_Copy(P, I, Q, I, M); copy one string to another */
	BitString_Copy( &t2, 0, &t1, 0, 33 );
	CHECK( t2.data[0] == 0xBB, "Byte 0 incorrect", result );
	CHECK( t2.data[1] == 0xBF, "Byte 1 incorrect", result );
	CHECK( t2.data[2] == 0x3D, "Byte 2 incorrect", result );
	CHECK( t2.data[3] == 0x5F, "Byte 3 incorrect", result );
	CHECK( t2.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t2 <- t1", t2 );
	
	/* Case 2: BitString_Copy(P, I, P, I, M); copy a string to it's self */
	BitString_Copy( &t2, 0, &t2, 0, 33 );
	CHECK( t2.data[0] == 0xBB, "Byte 0 incorrect", result );
	CHECK( t2.data[1] == 0xBF, "Byte 1 incorrect", result );
	CHECK( t2.data[2] == 0x3D, "Byte 2 incorrect", result );
	CHECK( t2.data[3] == 0x5F, "Byte 3 incorrect", result );
	CHECK( t2.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t1 <- t1", t1 );
	
	/* Case 3: BitString_Copy(P, I, Q, I + N, M); copy one string to another with negative offset */
	BitString_Fill( &t2, 0, 33, false ); /*< Clear t2 */
	BitString_Copy( &t2, 3, &t1, 5, 15 );
	CHECK( t2.data[0] == 0x0E, "Byte 0 incorrect", result );
	CHECK( t2.data[1] == 0xFC, "Byte 1 incorrect", result );
	CHECK( t2.data[2] == 0xC0, "Byte 2 incorrect", result );
	CHECK( t2.data[3] == 0x00, "Byte 3 incorrect", result );
	CHECK( t2.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t2[3] <-[15] t1[5]", t2 );
	
	/* Case 4: BitString_Copy(P, I, Q, I - N, M); copy one string to another with positive offset */
	BitString_Fill( &t2, 0, 33, false ); /*< Clear t2 */
	BitString_Copy( &t2, 5, &t1, 3, 15 );
	CHECK( t2.data[0] == 0x06, "Byte 0 incorrect", result );
	CHECK( t2.data[1] == 0xEF, "Byte 1 incorrect", result );
	CHECK( t2.data[2] == 0xC0, "Byte 2 incorrect", result );
	CHECK( t2.data[3] == 0x00, "Byte 3 incorrect", result );
	CHECK( t2.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t2[5] <-[15] t1[3]", t2 );
	
	/* Custom Case */
	BitString_Fill( &t2, 0, 33, false );
	BitString_Copy( &t2, 2, &t1, 21, 3 );
	CHECK( t2.data[0] == 0x28, "Byte 0 incorect", result );
	CHECK( t2.data[1] == 0x00, "Byte 1 incorrect", result );
	CHECK( t2.data[2] == 0x00, "Byte 2 incorrect", result );
	CHECK( t2.data[3] == 0x00, "Byte 3 incorrect", result );
	CHECK( t2.data[4] == 0x00, "Byte 4 incorrect", result );
	DISPL( "t2[2] <-[3] t1[21]", t2 );

	/* TODO: try more positions; must activate every line and condition in the code! */
	
	BitString_Free( &t1 );
	BitString_Free( &t2 );
	return result;
}

void setup( vector<aTest> &ourtests ) {
	ourtests.push_back( { &test_init, "Initialization Test" } );
	ourtests.push_back( { &test_count, "Bit Count Accessor Test" } );
	ourtests.push_back( { &test_bits, "Bit Assignment & Retrival Test" } );
	ourtests.push_back( { &test_copy, "Bitwise copy Test" } );
}

#define RUNTEST( treg, tix, failed ) \
	if( DISPLAY_DEBUG ) { \
		cout << "  :: " << treg[tix].name << " ::  " << endl; \
		bool status = treg[tix].test(); \
		cout << endl << "+---------------------+" << endl; \
		cout << "| Test Status: "; \
		if( status ) { \
			cout << "Passed"; \
		} else { \
			cout << "Failed"; \
			failed.push_back(tix); \
		} \
		cout << " |" << endl; \
		cout << "+---------------------+" << endl << endl << endl; \
	} else { \
		if( treg[tix].test() ) { \
			cout << "."; \
		} else { \
			cout << "F"; \
			failed.push_back(tix); \
		} \
	}

int main( int argc, char **argv ) {
	vector<aTest> registry;
	vector<int> schedual, failures;
	setup( registry );
	argproc( argc, argv, registry, schedual );
	if( schedual.size() == 0 ) {
		schedual.push_back( 0 );
	}
	
	// Iterate over the schedual
	for( int ix = 0; ix < schedual.size(); ix++ ) {
		if( schedual[ix] == 0 ) {
			// Run all tests
			for( int iy = 0; iy < registry.size(); iy++ ) {
				RUNTEST( registry, iy, failures );
			}
		} else {
			RUNTEST( registry, schedual[ix] - 1, failures );
		}
	}
	
	if( failures.size() > 0 ) {
		cout << endl << "Failures: ";
		for( int ifail = 0; ifail < failures.size(); ifail++ ) {
			cout << failures[ifail] << " ";
		}
		cout << endl;
	}
	
	return EXIT_SUCCESS;
}


#undef RUNTEST
