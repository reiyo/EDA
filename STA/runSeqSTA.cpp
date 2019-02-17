/************************************************************************
 *   Perform backward STA after injecting setup/hold time and input given
 *   required time constraints.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

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
//    Declare main functions 
//-----------------------------------------------------------------------

void runSeqBackwardSTA( Circuit &circuit );

void runSeqForwardSTA( Circuit &circuit );

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

extern bool areOutputsReached( GOutPin& gout_pin );

void injectFFsRATData( Circuit &circuit, queue<Gate*> &waited_queue );

extern void injectPOsRATData( Circuit &circuit, queue<Gate*> &waited_queue );

void runSeqForwardSTA( Circuit &circuit, vector<Gate*> &waited_queue );

extern void visitAndNotifyDrivingGate( PinNode &cur_pnode, queue<Gate*> &waited_queue );

//-----------------------------------------------------------------------
//    Define main functions
//-----------------------------------------------------------------------

void runSeqBackwardSTA( Circuit &circuit )
{
    queue<Gate*> waited_queue;

    injectFFsRATData( circuit, waited_queue ); // inject flip-flop related constraints

    injectPOsRATData( circuit, waited_queue ); // inject PO to let propagation

    while( !waited_queue.empty() )
    {
	Gate &cur_gate = *(waited_queue.front());
	waited_queue.pop();

//	if( cur_gate.GetCellPtr() != NULL )
//	    printf( "it is %s\n", cur_gate.GetCellPtr()->GetName().c_str() );

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

	if( cur_gate.GetCellPtr() != NULL && cur_gate.GetIsNonClocked() ) // not PI
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

// run both early-mode and late-mode STA on a sequential circuit
void runSeqForwardSTA( Circuit &circuit )
{
    assert( circuit.GetIsSequential() ); // for sequential circuit
    //assert( areAllComGatesInputNonTraversed(circuit) );

    vector<Gate*> waited_queue; // vector queue
    waited_queue.reserve( circuit.GetTotGateNo() );

    // insert primary inputs into queue
    for( unsigned i=0; i<circuit.GetPINo(); ++i )
	waited_queue.push_back( circuit.GetPIPtr(i) );

    runSeqForwardSTA( circuit, waited_queue ); // propagate from PI to flip-flops and primary outputs
    waited_queue.clear();
    const vector<Gate*> &SeqGate_ptr_vec = circuit.FetSeqGatePtrVec();

    // propagate clock signals through flip-flops to Q's and QN's output pin nodes
    for( unsigned i=0; i<SeqGate_ptr_vec.size(); ++i )
	propagateSignal( SeqGate_ptr_vec[i]->GetClockPinId(), SeqGate_ptr_vec[i] );

    // insert flip-flops into queue
    for( unsigned i=0; i<SeqGate_ptr_vec.size(); ++i )
	waited_queue.push_back( circuit.GetSeqGatePtr(i) );

    runSeqForwardSTA( circuit, waited_queue ); // propagate from flip-flops to flip-flops and primary outputs
    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

void injectFFsRATData( Circuit &circuit, queue<Gate*> &waited_queue )
{
    const vector<Gate*> &SeqGate_ptr_vec = circuit.FetSeqGatePtrVec();
    const double clock_period = circuit.GetClockPeriod();

    for( unsigned i=0; i<SeqGate_ptr_vec.size(); ++i )
    {
        Gate &cur_gate = *(SeqGate_ptr_vec[i]);
	const unsigned clock_pin_id = cur_gate.GetClockPinId();
	PinNode* clk_pnode_ptr = cur_gate.GetInputPinNode(clock_pin_id);

	if( clk_pnode_ptr == NULL )
	    continue;

	visitAndNotifyDrivingGate( *clk_pnode_ptr, waited_queue );
	const vector<ClockParams*> &clock_params_vec = (cur_gate.GetCellPtr())->FetClockParamsVec();

	for( unsigned j=0; j<cur_gate.GetInputNo(); ++j )
	{
	    if( j == clock_pin_id || cur_gate.GetInputPinNode(j) == NULL )
		continue;

	    PinNode &input_pnode = *(cur_gate.GetInputPinNode(j));

            visitAndNotifyDrivingGate( input_pnode, waited_queue );

	    if( clock_params_vec[j] == NULL )
		continue;

	    ClockParams &clock_params = *(clock_params_vec[j]);

	    // setup time constraint
	    if( clock_params.SetupEdgeType == ClockParams::RISING )
	    {
		double fall_setup = clock_params.FallSetupG + clock_params.FallSetupH * (clk_pnode_ptr->GetFastRiseSlew())
		                  + clock_params.FallSetupJ * input_pnode.GetSlowFallSlew();
		double rise_setup = clock_params.RiseSetupG + clock_params.RiseSetupH * (clk_pnode_ptr->GetFastRiseSlew())
		                  + clock_params.RiseSetupJ * input_pnode.GetSlowRiseSlew();

		input_pnode.SetSlowFallReqTime( clock_period + clk_pnode_ptr->GetFastRiseArrTime() - fall_setup );
		input_pnode.SetSlowRiseReqTime( clock_period + clk_pnode_ptr->GetFastRiseArrTime() - rise_setup );
	    }
	    else
	    {
		assert( clock_params.SetupEdgeType == ClockParams::FALLING );
		double fall_setup = clock_params.FallSetupG + clock_params.FallSetupH * (clk_pnode_ptr->GetFastFallSlew())
		                  + clock_params.FallSetupJ * input_pnode.GetSlowFallSlew();
		double rise_setup = clock_params.RiseSetupG + clock_params.RiseSetupH * (clk_pnode_ptr->GetFastFallSlew())
		                  + clock_params.RiseSetupJ * input_pnode.GetSlowRiseSlew();

		input_pnode.SetSlowFallReqTime( clock_period + clk_pnode_ptr->GetFastFallArrTime() - fall_setup );
		input_pnode.SetSlowRiseReqTime( clock_period + clk_pnode_ptr->GetFastFallArrTime() - rise_setup );
	    }

	    // hold time constraint
	    if( clock_params.HoldEdgeType == ClockParams::RISING )
	    {
		double fall_hold = clock_params.FallHoldM + clock_params.FallHoldN * (clk_pnode_ptr->GetSlowRiseSlew())
		                 + clock_params.FallHoldP * input_pnode.GetFastFallSlew();
		double rise_hold = clock_params.RiseHoldM + clock_params.RiseHoldN * (clk_pnode_ptr->GetSlowRiseSlew())
		                 + clock_params.RiseHoldP * input_pnode.GetFastRiseSlew();

		input_pnode.SetFastFallReqTime( clk_pnode_ptr->GetSlowRiseArrTime() + fall_hold );
		input_pnode.SetFastRiseReqTime( clk_pnode_ptr->GetSlowRiseArrTime() + rise_hold );

	    }
	    else
	    {
		assert( clock_params.HoldEdgeType == ClockParams::FALLING );
		double fall_hold = clock_params.FallHoldM + clock_params.FallHoldN * (clk_pnode_ptr->GetSlowFallSlew())
		                 + clock_params.FallHoldP * input_pnode.GetFastFallSlew();
		double rise_hold = clock_params.RiseHoldM + clock_params.RiseHoldN * (clk_pnode_ptr->GetSlowFallSlew())
		                 + clock_params.RiseHoldP * input_pnode.GetFastRiseSlew();

		input_pnode.SetFastFallReqTime( clk_pnode_ptr->GetSlowFallArrTime() + fall_hold );
		input_pnode.SetFastRiseReqTime( clk_pnode_ptr->GetSlowFallArrTime() + rise_hold );
	    }
	} // end input consideration
    }

    return;
}

// propagate from gates in waited_queue to flip-flops or primary outputs
void runSeqForwardSTA( Circuit &circuit, vector<Gate*> &waited_queue )
{
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
		{
		    driven_gate_ptr = (static_cast<GInPin*>(cur_pnode.GetFanoutPtr(k)))->GetGatePtr();
		}

		driven_gate_ptr->IncInputVisitedCount();
		assert( driven_gate_ptr->GetInputVisitedCount() <= driven_gate_ptr->GetInputNo() );

		// all inputs have been visited
		if( driven_gate_ptr->GetInputVisitedCount() >= driven_gate_ptr->GetInputNo() )
		{
		    if( driven_gate_ptr->GetIsNonClocked() && driven_gate_ptr->GetCellPtr() != NULL ) // not FF or PO
		    {
			waited_queue.push_back( driven_gate_ptr );
			propagateSignal( driven_gate_ptr ); // propagate signal through this gate
		    }
		}
	    } // end fanout consideration 
	}
    }

    return;
}

