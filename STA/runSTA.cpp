/************************************************************************
 *   Console of combinational or sequential STA.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cassert>
#include <vector>

#include "Circuit.h"
#include "Gate.h"
#include "process.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare main functions 
//-----------------------------------------------------------------------

void resistDefectPinNodes( Circuit &circuit );

void runSTA( Circuit &circuit );

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

bool areAllPinNodesNonVisited( Circuit &circuit ); // check only

bool areAllPinNodesVisited( Circuit &circuit );    // check only

void injectGivenRATData( Circuit &circuit );

void propagateVirtualSignal( PinNode &cur_pnode );

//-----------------------------------------------------------------------
//    Define main functions
//-----------------------------------------------------------------------

// resist simple defects on pin nodes to facilite STA signal propagation, e.g., off nodes in s400 and s1196
void resistDefectPinNodes( Circuit &circuit )
{
    vector<PinNode*> &pnodes = circuit.FetPinNodePtrVec();

    for( unsigned i=0; i<pnodes.size(); ++i )
    {
	PinNode &cur_pnode = *(pnodes[i]);
//	printf( "name = %s\n", cur_pnode.GetName().c_str() );

	// pin node with no fanin
	if( cur_pnode.GetFaninPtr() == NULL )
	    propagateVirtualSignal( cur_pnode );

	if( cur_pnode.GetFanoutPtrNo() == 0 )
	{
            Element *fanin_ptr = cur_pnode.GetFaninPtr();

	    while( fanin_ptr != NULL && fanin_ptr->GetType() != Element::GOUT_PIN )
	    {
		assert( fanin_ptr->GetType() == Element::PIN_NODE );
		fanin_ptr = static_cast<PinNode*>(fanin_ptr)->GetFaninPtr();
	    }

	    if( fanin_ptr == NULL )
		continue;

	    static_cast<GOutPin*>(fanin_ptr)->IncVisitedCount();
	}
    }

    return;
}

// gate with no input specified to anything (no off)
void resistDefectGates( Circuit &circuit )
{
    vector<Gate*> &gates = circuit.FetComGatePtrVec();

    for( unsigned i=0; i<gates.size(); ++i )
    {
	Gate &cur_gate = *(gates[i]);

	for( unsigned j=0; j<cur_gate.GetInputNo(); ++j )
	    if( cur_gate.FetGInPin(j).GetFaninPtr() == NULL )
		cur_gate.IncInputVisitedCount();
    }

    return;
}

void runSTA( Circuit &circuit )
{
    resistDefectPinNodes( circuit );
    resistDefectGates( circuit );

    if( circuit.GetIsSequential() )
    {
	runSeqForwardSTA( circuit );

	injectGivenRATData( circuit );

	assert( areAllPinNodesNonVisited(circuit) );
	runSeqBackwardSTA( circuit );
	assert( areAllPinNodesVisited(circuit) );
    }
    else
    {
	runComForwardSTA( circuit ); // get arrival time data, but it would set visited count in gates

	if( circuit.GetRATDataNo() > 0 )
	{
	    injectGivenRATData( circuit );

	    assert( areAllPinNodesNonVisited(circuit) );
	    runComBackwardSTA( circuit ); 
	    assert( areAllPinNodesVisited(circuit) );
	}
    }

    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

bool areAllPinNodesNonVisited( Circuit &circuit )
{
    vector<PinNode*> &pnode_vec = circuit.FetPinNodePtrVec();

    for( unsigned i=0; i<pnode_vec.size(); ++i )
	if( pnode_vec[i]->GetIsNotVisited() == false )
	    return false;

    return true;
}

bool areAllPinNodesVisited( Circuit &circuit )
{
    vector<PinNode*> &pnode_vec = circuit.FetPinNodePtrVec();

    for( unsigned i=0; i<pnode_vec.size(); ++i )
	if( pnode_vec[i]->GetIsNotVisited() )
	    return false;
    
    return true;
}

void injectGivenRATData( Circuit &circuit )
{
    vector<RATData> &rat_vec = circuit.FetRATDataVec();

    for( unsigned i=0; i<rat_vec.size(); ++i )
    {
	RATData &cur_data = rat_vec[i];
	PinNode &cur_pnode = (*cur_data.PinNodePtr);

	switch( cur_data.Mode )
	{
	    case RATData::BOTH:
		{
		    cur_pnode.SetFastFallReqTime(cur_data.FastFallTime); 
		    cur_pnode.SetFastRiseReqTime(cur_data.FastRiseTime);
		    cur_pnode.SetSlowFallReqTime(cur_data.SlowFallTime);
		    cur_pnode.SetSlowRiseReqTime(cur_data.SlowRiseTime);
		    break;
		}
	    case RATData::SLOW:
		{
		    cur_pnode.SetSlowFallReqTime(cur_data.SlowFallTime);
		    cur_pnode.SetSlowRiseReqTime(cur_data.SlowRiseTime);
		    break;
		}
	    case RATData::FAST:
		{
		    cur_pnode.SetFastFallReqTime(cur_data.FastFallTime);
		    cur_pnode.SetFastRiseReqTime(cur_data.FastRiseTime);
		    break;
		}
	    default:
		assert(0);
		break;
	}
    }
}

void propagateVirtualSignal( PinNode &cur_pnode )
{
    for( unsigned i=0; i<cur_pnode.GetFanoutPtrNo(); ++i )
    {
        Element *fanout_ptr = cur_pnode.GetFanoutPtr(i);

	if( fanout_ptr->GetType() == Element::GIN_PIN )
	{
//	    printf( "inject name = %s\n", static_cast<GInPin*>(fanout_ptr)->GetGatePtr()->GetCellPtr()->GetName().c_str() );
	    static_cast<GInPin*>(fanout_ptr)->GetGatePtr()->IncInputVisitedCount();
	    assert( static_cast<GInPin*>(fanout_ptr)->GetGatePtr()->GetInputVisitedCount() <
                    static_cast<GInPin*>(fanout_ptr)->GetGatePtr()->GetInputNo() );
	}
	else
	{
	    assert( fanout_ptr->GetType() == Element::PIN_NODE );
	    propagateVirtualSignal( *(static_cast<PinNode*>(fanout_ptr)) );
	}
    }

    return;
}

