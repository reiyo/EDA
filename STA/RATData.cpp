/************************************************************************
 *   Define member functions of class RATData: PrintRATData()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cstdio>

#include "RATData.h"

using namespace std;

void RATData::PrintRATData() const
{
    printf( "RAT: %s ", (*PinNodePtr).GetName().c_str() );

    if( Mode == FAST )
	printf( "early %.5le %.5le\n", FastFallTime, FastRiseTime );
    else if( Mode == SLOW )
	printf( "late %.5le %.5le\n", SlowFallTime, SlowRiseTime );
    else
	printf( "early %.5le %.5le, late %.5le %.5le\n", FastFallTime, FastRiseTime, SlowFallTime, SlowRiseTime );

    return;
}
