#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <list>
#include <queue>
#include <vector>

#include "Circuit.h"
#include "Gate.h"
#include "PinNode.h"
#include "process.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

bool areAllComGatesInputNonTraversed( const Circuit &circuit ); // check-only

bool areAllComGatesInputTraversed( const Circuit &circuit );    // check-only

bool areOutputsReached( GOutPin& gout_pin );

void injectPOsRATData( Circuit &circuit, queue<Gate*> &waited_queue );

void runComBackwardSlowSTA( const RATData &rat_data, Circuit &circuit );

void visitAndNotifyDrivingGate( PinNode &cur_pnode, queue<Gate*> &waited_queue );

//-----------------------------------------------------------------------
//    Define main functions
//-----------------------------------------------------------------------

void runComBackwardSTA( Circuit &circuit )
{
    queue<Gate*> waited_queue;

    injectPOsRATData( circuit, waited_queue ); // inject PO to let propagation

    while( !waited_queue.empty() )
    {
	Gate &cur_gate = *(waited_queue.front());
	waited_queue.pop();

        for( unsigned i=0; i<cur_gate.GetOutputNo(); ++i )
	{
	    if( cur_gate.GetOutputPinNode(i) == NULL )
		continue;

	    PinNode &cur_pnode = *(cur_gate.GetOutputPinNode(i));
	    const unsigned fanout_no = cur_pnode.GetFanoutNo();
	    vector<double> cur_fastFall_ratime, cur_fastRise_ratime, cur_slowFall_ratime, cur_slowRise_ratime;
	    cur_fastFall_ratime.reserve( fanout_no );
	    cur_fastRise_ratime.reserve( fanout_no );
	    cur_slowFall_ratime.reserve( fanout_no );
	    cur_slowRise_ratime.reserve( fanout_no );

	    for( unsigned j=0; j<fanout_no; ++j )
	    {
		Element *driven_ptr = cur_pnode.GetFanoutPtr(j);

		if( driven_ptr->GetType() == Element::GIN_PIN )
		    continue;

		PinNode &driven_pnode = *(static_cast<PinNode*>(driven_ptr));

		if( driven_pnode.GetFanoutPtrNo() == 0 )
		    continue;

                assert( (driven_pnode.GetFanoutPtr(0))->GetType() == Element::GIN_PIN );
		GInPin &driven_gin_pin = *(static_cast<GInPin*>(driven_pnode.GetFanoutPtr(0)));
		cur_fastFall_ratime.push_back( driven_pnode.GetFastFallReqTime() - driven_gin_pin.GetFallArrDelay() );
		cur_fastRise_ratime.push_back( driven_pnode.GetFastRiseReqTime() - driven_gin_pin.GetRiseArrDelay() );
                cur_slowFall_ratime.push_back( driven_pnode.GetSlowFallReqTime() - driven_gin_pin.GetFallArrDelay() );
		cur_slowRise_ratime.push_back( driven_pnode.GetSlowRiseReqTime() - driven_gin_pin.GetRiseArrDelay() );
	    }

	    cur_pnode.SetIsVisited();

	    if( cur_fastFall_ratime.size() > 0 )
	    {
		cur_pnode.SetFastFallReqTime( *max_element(cur_fastFall_ratime.begin(), cur_fastFall_ratime.end()) );
		cur_pnode.SetFastRiseReqTime( *max_element(cur_fastRise_ratime.begin(), cur_fastRise_ratime.end()) );
		cur_pnode.SetSlowFallReqTime( *min_element(cur_slowFall_ratime.begin(), cur_slowFall_ratime.end()) );
		cur_pnode.SetSlowRiseReqTime( *min_element(cur_slowRise_ratime.begin(), cur_slowRise_ratime.end()) );
	    }
	}

	if( cur_gate.GetCellPtr() != NULL ) // not PI
	{
	    backtraceSignal( cur_gate );

	    for( unsigned i=0; i<cur_gate.GetInputNo(); ++i )
	    {
		assert( cur_gate.GetInputPinNode(i) != NULL );

                if( cur_gate.GetInputPinNode(i) == NULL )
		    continue;

                visitAndNotifyDrivingGate( *(cur_gate.GetInputPinNode(i)), waited_queue );
	    }
	}
    }

    return;
}

// run both early-mode and late-mode STA on a combinational circuit
void runComForwardSTA( Circuit &circuit )
{
    assert( !circuit.GetIsSequential() ); // for combinational circuit only now
    assert( areAllComGatesInputNonTraversed(circuit) );

    vector<Gate*> waited_queue;
    waited_queue.reserve( circuit.GetTotGateNo() );

    // insert primary inputs into queue
    for( unsigned i=0; i<circuit.GetPINo(); ++i )
	waited_queue.push_back( circuit.GetPIPtr(i) );

    for( unsigned i=0; i<waited_queue.size(); ++i )
    {
	Gate &cur_gate = *(waited_queue[i]);

	for( unsigned j=0; j<cur_gate.GetOutputNo(); ++j )
	{
	    if( cur_gate.GetOutputPinNode(j) == NULL )
		continue;

            PinNode &cur_pnode = *(cur_gate.GetOutputPinNode(j));
	    const double output_fast_fall_slew_sq = cur_pnode.GetFastFallSlew() * cur_pnode.GetFastFallSlew();
	    const double output_fast_rise_slew_sq = cur_pnode.GetFastRiseSlew() * cur_pnode.GetFastRiseSlew();
	    const double output_slow_fall_slew_sq = cur_pnode.GetSlowFallSlew() * cur_pnode.GetSlowFallSlew();
	    const double output_slow_rise_slew_sq = cur_pnode.GetSlowRiseSlew() * cur_pnode.GetSlowRiseSlew();

	    for( unsigned k=0; k<cur_pnode.GetFanoutNo(); ++k )
	    {
                Gate *driven_gate_ptr;
		assert( (cur_pnode.GetFanoutPtr(k))->GetType() == Element::PIN_NODE || 
			(cur_pnode.GetFanoutPtr(k))->GetType() == Element::GIN_PIN );

		if( (cur_pnode.GetFanoutPtr(k))->GetType() == Element::PIN_NODE )
		{
		    PinNode &driven_pnode = *(static_cast<PinNode*>(cur_pnode.GetFanoutPtr(k)));

		    if( driven_pnode.GetFanoutPtrNo() == 0 ) // very special case
			continue;

		    assert( (driven_pnode.GetFanoutPtr(0))->GetType() == Element::GIN_PIN );
		    const GInPin &gin_pin = *(static_cast<GInPin*>(driven_pnode.GetFanoutPtr(0)));
		    const double wire_fall_delay = gin_pin.GetFallArrDelay();
		    const double wire_rise_delay = gin_pin.GetRiseArrDelay();

		    driven_pnode.SetFastFallArrTime( cur_pnode.GetFastFallArrTime() + wire_fall_delay );
		    driven_pnode.SetFastRiseArrTime( cur_pnode.GetFastRiseArrTime() + wire_rise_delay );
		    driven_pnode.SetFastFallSlew( sqrt( output_fast_fall_slew_sq + driven_pnode.GetFallSlewHatSq() ) );
		    driven_pnode.SetFastRiseSlew( sqrt( output_fast_rise_slew_sq + driven_pnode.GetRiseSlewHatSq() ) );

		    driven_pnode.SetSlowFallArrTime( cur_pnode.GetSlowFallArrTime() + wire_fall_delay );
		    driven_pnode.SetSlowRiseArrTime( cur_pnode.GetSlowRiseArrTime() + wire_rise_delay );
		    driven_pnode.SetSlowFallSlew( sqrt( output_slow_fall_slew_sq + driven_pnode.GetFallSlewHatSq() ) );
		    driven_pnode.SetSlowRiseSlew( sqrt( output_slow_rise_slew_sq + driven_pnode.GetRiseSlewHatSq() ) );

		    assert( driven_pnode.GetFanoutNo() == 1 );
		    assert( (driven_pnode.GetFanoutPtr(0))->GetType() == Element::GIN_PIN );
		    driven_gate_ptr = (static_cast<GInPin*>(driven_pnode.GetFanoutPtr(0)))->GetGatePtr();
		}
		else
		    driven_gate_ptr = (static_cast<GInPin*>(cur_pnode.GetFanoutPtr(k)))->GetGatePtr();

		driven_gate_ptr->IncInputVisitedCount();
		assert( driven_gate_ptr->GetInputVisitedCount() <= driven_gate_ptr->GetInputNo() );

		// all inputs have been visited
		if( driven_gate_ptr->GetInputVisitedCount() >= driven_gate_ptr->GetInputNo() )
		{
		    if( driven_gate_ptr->GetCellPtr() != NULL ) // not PO
		    {
			waited_queue.push_back( driven_gate_ptr );
			propagateSignal( driven_gate_ptr ); // propagate signal through this gate
		    }
		}
	    } // end fanout consideration 
	}
    }

    assert( areAllComGatesInputTraversed(circuit) );

    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

bool areAllComGatesInputNonTraversed( const Circuit &circuit )
{
    for( unsigned i=0; i<circuit.GetComGateNo(); ++i )
    {
        Gate* gate_ptr = circuit.GetComGatePtr(i);

	if( gate_ptr->GetInputVisitedCount() != 0 )
	    return false;
    }

    return true;
}

bool areAllComGatesInputTraversed( const Circuit &circuit )
{
    for( unsigned i=0; i<circuit.GetComGateNo(); ++i )
    {
	Gate* gate_ptr = circuit.GetComGatePtr(i);

	if( gate_ptr->GetInputVisitedCount() != gate_ptr->GetInputNo() )
	    return false;
    }

    return true;
}

bool areOutputsReached( GOutPin& gout_pin )
{
    if( gout_pin.GetVisitedCount() < (gout_pin.GetFanoutPtr())->GetFanoutNo() )
	return false;

    Gate &cur_gate = *(gout_pin.GetGatePtr());

    for( unsigned i=0; i<cur_gate.GetOutputNo(); ++i )
    {
	GOutPin &curGout_pin = cur_gate.FetGOutPin(i);

	if( curGout_pin.GetFanoutPtr() == NULL )
	    continue;

	if( curGout_pin.GetVisitedCount() < (curGout_pin.GetFanoutPtr())->GetFanoutNo() )
	    return false;
    }

    return true;
}

void injectPOsRATData( Circuit &circuit, queue<Gate*> &waited_queue )
{
    vector<Gate*> &po_ptr_vec = circuit.FetPOPtrVec();

    for( unsigned i=0; i<po_ptr_vec.size(); ++i )
    {
	Gate* cur_gate = po_ptr_vec[i];
	assert( cur_gate->GetInputNo() == 1 && cur_gate->GetInputPinNode(0) != NULL );
	PinNode &cur_pnode = *(cur_gate->GetInputPinNode(0));
	const bool is_not_visited = cur_pnode.GetIsNotVisited();
	cur_pnode.SetIsVisited();

	if( cur_pnode.GetFaninPtr() != NULL )
	{
	    GOutPin *gout_pin=NULL;

	    if( (cur_pnode.GetFaninPtr())->GetType() == Element::PIN_NODE )
	    {
		if( is_not_visited )
		{
		    PinNode *tmp_ptr = static_cast<PinNode*>(cur_pnode.GetFaninPtr());
		    assert( tmp_ptr->GetFaninPtr()->GetType() == Element::GOUT_PIN );
		    gout_pin = static_cast<GOutPin*>(tmp_ptr->GetFaninPtr());
		    gout_pin->IncVisitedCount();

		    if( areOutputsReached( *gout_pin ) )
			waited_queue.push( gout_pin->GetGatePtr() );
		}
	    }
	    else
	    {
		assert( (cur_pnode.GetFaninPtr())->GetType() == Element::GOUT_PIN );
		gout_pin = static_cast<GOutPin*>(cur_pnode.GetFaninPtr());
		gout_pin->IncVisitedCount();

		if( areOutputsReached( *gout_pin ) )
		    waited_queue.push( gout_pin->GetGatePtr() );
	    }
	}
    }

    return;
}

void visitAndNotifyDrivingGate( PinNode &cur_pnode, queue<Gate*> &waited_queue )
{
    const bool non_visited = cur_pnode.GetIsNotVisited();
    cur_pnode.SetIsVisited();
    Element *fanin_ptr = cur_pnode.GetFaninPtr();

    if( fanin_ptr == NULL )
	return;

    if( fanin_ptr->GetType() == Element::PIN_NODE )
    {
	if( non_visited )
	{
	    PinNode *tmp_ptr = static_cast<PinNode*>(fanin_ptr);

	    if( tmp_ptr->GetFaninPtr() == NULL ) // very special case
	    {
		(*tmp_ptr).SetIsVisited();
		return;
	    }

	    assert( tmp_ptr->GetFaninPtr()->GetType() == Element::GOUT_PIN );
	    GOutPin &gout_pin = *(static_cast<GOutPin*>(tmp_ptr->GetFaninPtr()));
	    gout_pin.IncVisitedCount();

	    if( areOutputsReached( gout_pin ) )
		waited_queue.push( gout_pin.GetGatePtr() );
	}
    }
    else
    {
	assert( fanin_ptr->GetType() == Element::GOUT_PIN );
	GOutPin &gout_pin = *(static_cast<GOutPin*>(fanin_ptr));
	gout_pin.IncVisitedCount();

	if( areOutputsReached( gout_pin ) )
	    waited_queue.push( gout_pin.GetGatePtr() );
    }

    return;
}

