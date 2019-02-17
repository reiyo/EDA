/************************************************************************
 *   Propagate signals through a gate by computing output arrivial time
 *   and signal transition time (i.e., slew). Work as sub-functions of STA.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>

#include "Gate.h"
#include "PinNode.h"
#include "process.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare main functions
//-----------------------------------------------------------------------

void propagateSignal( Gate *gate_ptr ); // propagate both fast and slow signals

void propagateSignal( const unsigned &input_pin_id, Gate *gate_ptr ); // clock to Q with early-mode and late-mode

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

double computeGateOutputFall( const InputTimingTable &timing, const double &input_arrival, const double &input_slew, const double &load, vector<double> &output_arrival, vector<double> &output_slew );

double computeGateOutputRise( const InputTimingTable &timing, const double &input_arrival, const double &input_slew, const double &load, vector<double> &output_arrival, vector<double> &output_slew );

//-----------------------------------------------------------------------
//    Define main functions
//-----------------------------------------------------------------------

void propagateSignal( Gate *gate_ptr )
{
    assert( gate_ptr->GetCellPtr()->GetIsNonClocked() );
    const Cell &cur_cell = *(gate_ptr->GetCellPtr());
    const vector<vector<InputTimingTable> > &timing_vec = cur_cell.FetInputTimingVec();
    const unsigned input_no = cur_cell.GetInputPinNo();

    // index corresponds to input pin index, so resize directly
    vector<double> input_fastFall_arrival(input_no), input_fastRise_arrival(input_no);
    vector<double> input_fastFall_slew(input_no), input_fastRise_slew(input_no);
    vector<double> input_slowFall_arrival(input_no), input_slowRise_arrival(input_no);
    vector<double> input_slowFall_slew(input_no), input_slowRise_slew(input_no);

    for( unsigned i=0; i<input_no; ++i )
    {
	PinNode *cur_pnode_ptr = gate_ptr->GetInputPinNode(i);

	if( cur_pnode_ptr == NULL || cur_pnode_ptr->GetFaninPtr() == NULL )
	    continue;

	PinNode &cur_pnode = *(cur_pnode_ptr);
	input_fastFall_arrival[i] = cur_pnode.GetFastFallArrTime();
	input_fastRise_arrival[i] = cur_pnode.GetFastRiseArrTime();
	input_fastFall_slew[i]    = cur_pnode.GetFastFallSlew();
	input_fastRise_slew[i]    = cur_pnode.GetFastRiseSlew();
	input_slowFall_arrival[i] = cur_pnode.GetSlowFallArrTime();
	input_slowRise_arrival[i] = cur_pnode.GetSlowRiseArrTime();
	input_slowFall_slew[i]    = cur_pnode.GetSlowFallSlew();
	input_slowRise_slew[i]    = cur_pnode.GetSlowRiseSlew();
    }

    // use push back to insert due to non-uncate case
    vector<double> output_fastFall_arrival, output_fastRise_arrival; // accumulated delay
    vector<double> output_fastFall_slew, output_fastRise_slew;
    vector<double> output_slowFall_arrival, output_slowRise_arrival; // accumulated delay
    vector<double> output_slowFall_slew, output_slowRise_slew;

    output_fastFall_arrival.reserve(input_no);
    output_fastRise_arrival.reserve(input_no);
    output_fastFall_slew.reserve(input_no);
    output_fastRise_slew.reserve(input_no);
    output_slowFall_arrival.reserve(input_no);
    output_slowRise_arrival.reserve(input_no);
    output_slowFall_slew.reserve(input_no);
    output_slowRise_slew.reserve(input_no);

    const unsigned output_no = cur_cell.GetOutputPinNo();

    // consider every output 
    for( unsigned i=0; i<output_no; ++i )
    {
	GOutPin &gout_pin = gate_ptr->FetGOutPin(i);

	if( gout_pin.GetFanoutPtr() == NULL )
	    continue;

	PinNode &output_pnode = *(gout_pin.GetFanoutPtr());

	if( output_pnode.GetFanoutPtrNo() == 0 )
	    continue;

	const double fall_load = gate_ptr->GetOutputFallLoad(i);
	const double rise_load = gate_ptr->GetOutputRiseLoad(i);
	gout_pin.InitFastDelayDataPtrVec(input_no);
	gout_pin.InitSlowDelayDataPtrVec(input_no);

        for( unsigned j=0; j<input_no; ++j )
	{
	    PinNode *cur_pnode_ptr = gate_ptr->GetInputPinNode(j);

	    if( cur_pnode_ptr == NULL || cur_pnode_ptr->GetFaninPtr() == NULL )
		continue;

            const InputTimingTable &timing = timing_vec[j][i];

	    switch( timing.PinTimingSense )
	    {
		case InputTimingTable::NEGATIVE_UNATE:
		    {
			UnateDelayData *delay_ptr = new UnateDelayData();
			delay_ptr->DelayFromInputRise = computeGateOutputFall( timing, input_fastRise_arrival[j], 
				input_fastRise_slew[j], fall_load, output_fastFall_arrival, output_fastFall_slew );
			delay_ptr->DelayFromInputFall = computeGateOutputRise( timing, input_fastFall_arrival[j], 
				input_fastFall_slew[j], rise_load, output_fastRise_arrival, output_fastRise_slew );
			gout_pin.SetFastDelayDataPtr(j, delay_ptr);

			delay_ptr = new UnateDelayData();
			delay_ptr->DelayFromInputRise = computeGateOutputFall( timing, input_slowRise_arrival[j], 
				input_slowRise_slew[j], fall_load, output_slowFall_arrival, output_slowFall_slew );
			delay_ptr->DelayFromInputFall = computeGateOutputRise( timing, input_slowFall_arrival[j], 
				input_slowFall_slew[j], rise_load, output_slowRise_arrival, output_slowRise_slew );
			gout_pin.SetSlowDelayDataPtr(j, delay_ptr);
			break;
		    }
		case InputTimingTable::POSITIVE_UNATE:
		    {
			UnateDelayData *delay_ptr = new UnateDelayData();
			delay_ptr->DelayFromInputFall = computeGateOutputFall( timing, input_fastFall_arrival[j], 
				input_fastFall_slew[j], fall_load, output_fastFall_arrival, output_fastFall_slew );
			delay_ptr->DelayFromInputRise = computeGateOutputRise( timing, input_fastRise_arrival[j], 
				input_fastRise_slew[j], rise_load, output_fastRise_arrival, output_fastRise_slew );
			gout_pin.SetFastDelayDataPtr(j, delay_ptr);

			delay_ptr = new UnateDelayData();
			delay_ptr->DelayFromInputFall = computeGateOutputFall( timing, input_slowFall_arrival[j], 
				input_slowFall_slew[j], fall_load, output_slowFall_arrival, output_slowFall_slew );
			delay_ptr->DelayFromInputRise = computeGateOutputRise( timing, input_slowRise_arrival[j], 
				input_slowRise_slew[j], rise_load, output_slowRise_arrival, output_slowRise_slew );
			gout_pin.SetSlowDelayDataPtr(j, delay_ptr);
			break;
		    }
		case InputTimingTable::NON_UNATE:
		    {
			NonUnateDelayData *delay_ptr = new NonUnateDelayData();
			delay_ptr->InputRiseOutputFallDelay = computeGateOutputFall( timing, input_fastRise_arrival[j], 
				input_fastRise_slew[j], fall_load, output_fastFall_arrival, output_fastFall_slew );
			delay_ptr->InputFallOutputRiseDelay = computeGateOutputRise( timing, input_fastFall_arrival[j], 
				input_fastFall_slew[j], rise_load, output_fastRise_arrival, output_fastRise_slew );
			delay_ptr->InputFallOutputFallDelay = computeGateOutputFall( timing, input_fastFall_arrival[j], 
				input_fastFall_slew[j], fall_load, output_fastFall_arrival, output_fastFall_slew );
			delay_ptr->InputRiseOutputRiseDelay = computeGateOutputRise( timing, input_fastRise_arrival[j], 
				input_fastRise_slew[j], rise_load, output_fastRise_arrival, output_fastRise_slew );
                        gout_pin.SetFastDelayDataPtr(j, delay_ptr);

			delay_ptr = new NonUnateDelayData();
			delay_ptr->InputRiseOutputFallDelay = computeGateOutputFall( timing, input_slowRise_arrival[j], 
				input_slowRise_slew[j], fall_load, output_slowFall_arrival, output_slowFall_slew );
			delay_ptr->InputFallOutputRiseDelay = computeGateOutputRise( timing, input_slowFall_arrival[j], 
				input_slowFall_slew[j], rise_load, output_slowRise_arrival, output_slowRise_slew );
			delay_ptr->InputFallOutputFallDelay = computeGateOutputFall( timing, input_slowFall_arrival[j], 
				input_slowFall_slew[j], fall_load, output_slowFall_arrival, output_slowFall_slew );
			delay_ptr->InputRiseOutputRiseDelay = computeGateOutputRise( timing, input_slowRise_arrival[j], 
				input_slowRise_slew[j], rise_load, output_slowRise_arrival, output_slowRise_slew );
                        gout_pin.SetSlowDelayDataPtr(j, delay_ptr);
			break;
		    }
		default:
		    assert(0);
		    break;

	    } // end switch
	} // end input

	assert( output_fastFall_arrival.size()>0 );
	output_pnode.SetFastFallArrTime( *min_element(output_fastFall_arrival.begin(), output_fastFall_arrival.end()) ); 
	output_pnode.SetFastFallSlew( *min_element(output_fastFall_slew.begin(), output_fastFall_slew.end()) );
	output_pnode.SetFastRiseArrTime( *min_element(output_fastRise_arrival.begin(), output_fastRise_arrival.end()) );
	output_pnode.SetFastRiseSlew( *min_element(output_fastRise_slew.begin(), output_fastRise_slew.end()) );
	output_pnode.SetSlowFallArrTime( *max_element(output_slowFall_arrival.begin(), output_slowFall_arrival.end()) ); 
	output_pnode.SetSlowFallSlew( *max_element(output_slowFall_slew.begin(), output_slowFall_slew.end()) );
	output_pnode.SetSlowRiseArrTime( *max_element(output_slowRise_arrival.begin(), output_slowRise_arrival.end()) );
	output_pnode.SetSlowRiseSlew( *max_element(output_slowRise_slew.begin(), output_slowRise_slew.end()) );

	output_fastFall_arrival.clear();
	output_fastRise_arrival.clear();
	output_fastFall_slew.clear();
	output_fastRise_slew.clear();
	output_slowFall_arrival.clear();
	output_slowRise_arrival.clear();
	output_slowFall_slew.clear();
	output_slowRise_slew.clear();
    } // end consider an output

    return;
}

// propagate a specified input pin signal to outputs, e.g., clock to Q and QN
void propagateSignal( const unsigned &input_pin_id, Gate *gate_ptr )
{
    assert( gate_ptr->GetCellPtr() != NULL && !(gate_ptr->GetCellPtr()->GetIsNonClocked()) );
    const Cell &cur_cell = *(gate_ptr->GetCellPtr());
    const vector<vector<InputTimingTable> > &timing_vec = cur_cell.FetInputTimingVec();
    PinNode &cur_pnode = *(gate_ptr->GetInputPinNode(input_pin_id));

    // use push back to insert due to non-uncate case
    vector<double> output_fastFall_arrival, output_fastRise_arrival; // accumulated delay
    vector<double> output_fastFall_slew, output_fastRise_slew;
    vector<double> output_slowFall_arrival, output_slowRise_arrival; // accumulated delay
    vector<double> output_slowFall_slew, output_slowRise_slew;
    const unsigned output_no = cur_cell.GetOutputPinNo();

    // consider every output 
    for( unsigned i=0; i<output_no; ++i )
    {
	if( gate_ptr->GetOutputPinNode(i) == NULL )
	    continue;

	PinNode &output_pnode = *(gate_ptr->GetOutputPinNode(i));

	if( output_pnode.GetFanoutPtrNo() == 0 )
	    continue;

	const double fall_load = gate_ptr->GetOutputFallLoad(i);
	const double rise_load = gate_ptr->GetOutputRiseLoad(i);
	const InputTimingTable &timing = timing_vec[input_pin_id][i];

	if( timing.PinTimingSense == InputTimingTable::NON_UNATE )
	{
	    computeGateOutputFall( timing, cur_pnode.GetFastRiseArrTime(), cur_pnode.GetFastRiseSlew(), fall_load, 
		                   output_fastFall_arrival, output_fastFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastFallSlew(), rise_load, 
		                   output_fastRise_arrival, output_fastRise_slew );

	    computeGateOutputFall( timing, cur_pnode.GetSlowRiseArrTime(), cur_pnode.GetSlowRiseSlew(), fall_load, 
		                   output_slowFall_arrival, output_slowFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowFallSlew(), rise_load, 
		                   output_slowRise_arrival, output_slowRise_slew );

	    computeGateOutputFall( timing, cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastFallSlew(), fall_load, 
		                   output_fastFall_arrival, output_fastFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetFastRiseArrTime(), cur_pnode.GetFastRiseSlew(), rise_load, 
		                   output_fastRise_arrival, output_fastRise_slew );

	    computeGateOutputFall( timing, cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowFallSlew(), fall_load, 
		                   output_slowFall_arrival, output_slowFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetSlowRiseArrTime(), cur_pnode.GetSlowRiseSlew(), rise_load, 
		                   output_slowRise_arrival, output_slowRise_slew );
	}
	else if( timing.PinTimingSense == InputTimingTable::NEGATIVE_UNATE )
	{
	    computeGateOutputFall( timing, cur_pnode.GetFastRiseArrTime(), cur_pnode.GetFastRiseSlew(), fall_load, 
		                   output_fastFall_arrival, output_fastFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastFallSlew(), rise_load, 
		                   output_fastRise_arrival, output_fastRise_slew );

	    computeGateOutputFall( timing, cur_pnode.GetSlowRiseArrTime(), cur_pnode.GetSlowRiseSlew(), fall_load, 
		                   output_slowFall_arrival, output_slowFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowFallSlew(), rise_load, 
		                   output_slowRise_arrival, output_slowRise_slew );
	}
	else
	{
	    assert( timing.PinTimingSense == InputTimingTable::POSITIVE_UNATE );
	    computeGateOutputFall( timing, cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastFallSlew(), fall_load, 
		                   output_fastFall_arrival, output_fastFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetFastRiseArrTime(), cur_pnode.GetFastRiseSlew(), rise_load, 
		                   output_fastRise_arrival, output_fastRise_slew );

	    computeGateOutputFall( timing, cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowFallSlew(), fall_load, 
		                   output_slowFall_arrival, output_slowFall_slew );
	    computeGateOutputRise( timing, cur_pnode.GetSlowRiseArrTime(), cur_pnode.GetSlowRiseSlew(), rise_load, 
		                   output_slowRise_arrival, output_slowRise_slew );
	}

	assert( output_fastFall_arrival.size()>0 ); // at least one input has a signal
	output_pnode.SetFastFallArrTime( *min_element(output_fastFall_arrival.begin(), output_fastFall_arrival.end()) ); 
	output_pnode.SetFastFallSlew( *min_element(output_fastFall_slew.begin(), output_fastFall_slew.end()) );
	output_pnode.SetFastRiseArrTime( *min_element(output_fastRise_arrival.begin(), output_fastRise_arrival.end()) );
	output_pnode.SetFastRiseSlew( *min_element(output_fastRise_slew.begin(), output_fastRise_slew.end()) );
	output_pnode.SetSlowFallArrTime( *max_element(output_slowFall_arrival.begin(), output_slowFall_arrival.end()) ); 
	output_pnode.SetSlowFallSlew( *max_element(output_slowFall_slew.begin(), output_slowFall_slew.end()) );
	output_pnode.SetSlowRiseArrTime( *max_element(output_slowRise_arrival.begin(), output_slowRise_arrival.end()) );
	output_pnode.SetSlowRiseSlew( *max_element(output_slowRise_slew.begin(), output_slowRise_slew.end()) );

	output_fastFall_arrival.clear();
	output_fastRise_arrival.clear();
	output_fastFall_slew.clear();
	output_fastRise_slew.clear();
	output_slowFall_arrival.clear();
	output_slowRise_arrival.clear();
	output_slowFall_slew.clear();
	output_slowRise_slew.clear();
    } // end consider an output
 
    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

// compute gate output signal falling information to output_arrival and output_slew
double computeGateOutputFall( const InputTimingTable &timing, const double &input_arrival, const double &input_slew, const double &load, vector<double> &output_arrival, vector<double> &output_slew )
{
    double gate_delay = timing.FallDelayA + timing.FallDelayB * load + timing.FallDelayC * input_slew;

    output_arrival.push_back( input_arrival + gate_delay );
    output_slew.push_back( timing.FallSlewX + timing.FallSlewY * load + timing.FallSlewZ * input_slew );

    return gate_delay;
}

// compute gate output signal rising information to output_arrival and output_slew 
double computeGateOutputRise( const InputTimingTable &timing, const double &input_arrival, const double &input_slew, const double &load, vector<double> &output_arrival, vector<double> &output_slew )
{
    double gate_delay = timing.RiseDelayA + timing.RiseDelayB * load + timing.RiseDelayC * input_slew;

    output_arrival.push_back( input_arrival + gate_delay );
    output_slew.push_back( timing.RiseSlewX + timing.RiseSlewY * load + timing.RiseSlewZ * input_slew );

    return gate_delay;
}

