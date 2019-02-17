/************************************************************************
 *   Main file of the static timing analysis program for PATMOS contest.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cstdlib>
#include <cstdio>

#include "CellLibrary.h"
#include "Circuit.h"
#include "process.h"
#include "util.h"

int main(int argc, char **argv)
{
    if(argc != 4)
    {
	printf( "Usage: %s [library file] [netlist file] [output file]\n", argv[0] );
	printf( "  Exiting...\n" );
	exit(-1);
    }

    CellLibrary cell_library( argv[1] ); // declare and initialize cell library
//    cell_library.PrintCellLibraryData();    
    
    Circuit circuit( argv[2], cell_library );
//    circuit.PrintCircuitData();

    injectWiringEffects( circuit );

    runSTA( circuit );

    circuit.PrintTimingData( argv[3] );

    printf( "Memory Usage: %.10lfMB\n", getPeakMemoryUsage() );

    return 0;
}
