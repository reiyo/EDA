/************************************************************************
 *   Define the data structures of required arrival time constraints.
 *
 *   Defined class: RATData
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef RAT_DATA_H
#define RAT_DATA_H

#include <cassert>
#include <string>

#include "PinNode.h"

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class RATData;

//-----------------------------------------------------------------------
//    Define class
//-----------------------------------------------------------------------

class RATData
{
    public:
	enum ModeType
	{
	    SLOW, // late mode
	    FAST, // early mode
	    BOTH  // both late and early mode
	};

        RATData(PinNode *ptr, ModeType m, double sf_t, double sr_t, double ff_t, double fr_t): 
	    PinNodePtr(ptr), Mode(m), SlowFallTime(sf_t), SlowRiseTime(sr_t), FastFallTime(ff_t), FastRiseTime(fr_t) {}

	std::string GetPinNodeName() const { assert( PinNodePtr!=NULL ); return PinNodePtr->GetName(); }
	void PrintRATData() const;

	bool operator < (const RATData &rhs) const
	{
	    return (this->GetPinNodeName() < rhs.GetPinNodeName());
	}

	// member variables
	PinNode *PinNodePtr;
	ModeType Mode; 

	double SlowFallTime;
	double SlowRiseTime;
	double FastFallTime;
	double FastRiseTime;
};

#endif //RATData
