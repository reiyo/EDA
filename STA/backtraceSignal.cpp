/************************************************************************
 *   Backtrace signals through a gate by computing input required arrival
 *   time under the known required arrival time on its output. 
 *   Work as sub-functions of STA.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

// backtrace signal changes

#include <algorithm>
#include <vector>

#include "DelayData.h"
#include "Gate.h"
#include "PinNode.h"
#include "process.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare main functions
//-----------------------------------------------------------------------

void backtraceSignal( Gate &cur_gate );     // backtraceFastSignal() + backtraceSlowSignal()

void backtraceFastSignal( Gate &cur_gate ); // can be called independently

void backtraceSlowSignal( Gate &cur_gate );

//-----------------------------------------------------------------------
//    Define main functions
//-----------------------------------------------------------------------

void backtraceSignal( Gate &cur_gate )
{
    assert( cur_gate.GetCellPtr() != NULL && (cur_gate.GetCellPtr())->GetIsNonClocked() );
    //printf( "name = %s\n", cur_gate.GetCellPtr()->GetName().c_str() );
    const Cell &cur_cell = *(cur_gate.GetCellPtr());
    const vector<vector<InputTimingTable> > &timing_vec = cur_cell.FetInputTimingVec();
    vector<double> input_fastFall_ratime, input_fastRise_ratime;
    vector<double> input_slowFall_ratime, input_slowRise_ratime;
    const unsigned input_no = cur_cell.GetInputPinNo();

    for( unsigned i=0; i<input_no; ++i )
    {
	PinNode *input_pnode_ptr = cur_gate.GetInputPinNode(i);

	if( input_pnode_ptr == NULL || input_pnode_ptr->GetFaninPtr() == NULL )
	    continue;

	PinNode &input_pnode = *(input_pnode_ptr);

	for( unsigned j=0; j<cur_cell.GetOutputPinNo(); ++j )
	{
	    GOutPin &output_pin = cur_gate.FetGOutPin(j);

	    if( output_pin.GetFanoutPtr() == NULL )
		continue;

	    PinNode &output_pnode = *(output_pin.GetFanoutPtr());

	    if( output_pnode.GetFanoutPtrNo() == 0 )
		continue;

	    const InputTimingTable &timing = timing_vec[i][j];

	    switch( timing.PinTimingSense )
	    {
		case InputTimingTable::NEGATIVE_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL && output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::UNATE );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetFastDelayDataPtr(i));
			input_fastFall_ratime.push_back( output_pnode.GetFastRiseReqTime() - delay_ptr->DelayFromInputFall );
			input_fastRise_ratime.push_back( output_pnode.GetFastFallReqTime() - delay_ptr->DelayFromInputRise );
			delay_ptr = static_cast<UnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));
			input_slowFall_ratime.push_back( output_pnode.GetSlowRiseReqTime() - delay_ptr->DelayFromInputFall );
			input_slowRise_ratime.push_back( output_pnode.GetSlowFallReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::POSITIVE_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL && output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::UNATE );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetFastDelayDataPtr(i));
			input_fastFall_ratime.push_back( output_pnode.GetFastFallReqTime() - delay_ptr->DelayFromInputFall );
			input_fastRise_ratime.push_back( output_pnode.GetFastRiseReqTime() - delay_ptr->DelayFromInputRise );
			delay_ptr = static_cast<UnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));
			input_slowFall_ratime.push_back( output_pnode.GetSlowFallReqTime() - delay_ptr->DelayFromInputFall );
			input_slowRise_ratime.push_back( output_pnode.GetSlowRiseReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::InputTimingTable::NON_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL && output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::NON_UNATE );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::NON_UNATE );
			NonUnateDelayData *delay_ptr = static_cast<NonUnateDelayData*>(output_pin.GetFastDelayDataPtr(i));

			input_fastFall_ratime.push_back( output_pnode.GetFastRiseReqTime() - 
				                         delay_ptr->InputFallOutputRiseDelay );
			input_fastRise_ratime.push_back( output_pnode.GetFastFallReqTime() - 
				                         delay_ptr->InputRiseOutputFallDelay );
			input_fastFall_ratime.push_back( output_pnode.GetFastFallReqTime() - 
				                         delay_ptr->InputFallOutputFallDelay );
			input_fastRise_ratime.push_back( output_pnode.GetFastRiseReqTime() - 
				                         delay_ptr->InputRiseOutputRiseDelay );

                        delay_ptr = static_cast<NonUnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));

			input_slowFall_ratime.push_back( output_pnode.GetSlowRiseReqTime() - 
				                         delay_ptr->InputFallOutputRiseDelay );
			input_slowRise_ratime.push_back( output_pnode.GetSlowFallReqTime() - 
				                         delay_ptr->InputRiseOutputFallDelay );
			input_slowFall_ratime.push_back( output_pnode.GetSlowFallReqTime() - 
				                         delay_ptr->InputFallOutputFallDelay );
			input_slowRise_ratime.push_back( output_pnode.GetSlowRiseReqTime() - 
				                         delay_ptr->InputRiseOutputRiseDelay );
			break;
		    }
		default:
		    assert(0);
		    break;
	    }
	} // end output

	// special case may happened at PI
	if( (input_pnode.GetFaninPtr())->GetType() == Element::GOUT_PIN && input_pnode.GetFanoutNo() > 1 )
	{
            input_fastFall_ratime.push_back( input_pnode.GetFastFallReqTime() );
	    input_fastRise_ratime.push_back( input_pnode.GetFastRiseReqTime() );
            input_slowFall_ratime.push_back( input_pnode.GetSlowFallReqTime() );
	    input_slowRise_ratime.push_back( input_pnode.GetSlowRiseReqTime() );
	}

	assert( input_fastFall_ratime.size()>0 ); // at least one output has a signal
	input_pnode.SetFastFallReqTime( *max_element(input_fastFall_ratime.begin(), input_fastFall_ratime.end()) );
	input_pnode.SetFastRiseReqTime( *max_element(input_fastRise_ratime.begin(), input_fastRise_ratime.end()) );
	input_pnode.SetSlowFallReqTime( *min_element(input_slowFall_ratime.begin(), input_slowFall_ratime.end()) );
	input_pnode.SetSlowRiseReqTime( *min_element(input_slowRise_ratime.begin(), input_slowRise_ratime.end()) );

	input_fastFall_ratime.clear();
	input_fastRise_ratime.clear();
	input_slowFall_ratime.clear();
	input_slowRise_ratime.clear();
    } // end input

    return;
}

void backtraceFastSignal( Gate &cur_gate )
{
    assert( cur_gate.GetCellPtr() != NULL && (cur_gate.GetCellPtr())->GetIsNonClocked() );
    const Cell &cur_cell = *(cur_gate.GetCellPtr());
    const vector<vector<InputTimingTable> > &timing_vec = cur_cell.FetInputTimingVec();
    vector<double> input_fall_ratime, input_rise_ratime;
    const unsigned input_no = cur_cell.GetInputPinNo();

    for( unsigned i=0; i<input_no; ++i )
    {
	PinNode *input_pnode_ptr = cur_gate.GetInputPinNode(i);

	if( input_pnode_ptr == NULL || input_pnode_ptr->GetFaninPtr() == NULL )
	    continue;

	PinNode &input_pnode = *(input_pnode_ptr);

	for( unsigned j=0; j<cur_cell.GetOutputPinNo(); ++j )
	{
	    GOutPin &output_pin = cur_gate.FetGOutPin(j);

	    if( output_pin.GetFanoutPtr() == NULL )
		continue;

	    PinNode &output_pnode = *(output_pin.GetFanoutPtr());

	    if( output_pnode.GetFanoutPtrNo() == 0 )
		continue;

	    const InputTimingTable &timing = timing_vec[i][j];

	    switch( timing.PinTimingSense )
	    {
		case InputTimingTable::NEGATIVE_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetFastDelayDataPtr(i));
			input_fall_ratime.push_back( output_pnode.GetFastRiseReqTime() - delay_ptr->DelayFromInputFall );
			input_rise_ratime.push_back( output_pnode.GetFastFallReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::POSITIVE_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetFastDelayDataPtr(i));
			input_fall_ratime.push_back( output_pnode.GetFastFallReqTime() - delay_ptr->DelayFromInputFall );
			input_rise_ratime.push_back( output_pnode.GetFastRiseReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::InputTimingTable::NON_UNATE:
		    {
			assert( output_pin.GetFastDelayDataPtr(i) != NULL );
			assert( output_pin.GetFastDelayDataPtr(i)->GetType() == DelayData::NON_UNATE );
			NonUnateDelayData *delay_ptr = static_cast<NonUnateDelayData*>(output_pin.GetFastDelayDataPtr(i));

			input_fall_ratime.push_back( output_pnode.GetFastRiseReqTime() - 
				                     delay_ptr->InputFallOutputRiseDelay );

			input_rise_ratime.push_back( output_pnode.GetFastFallReqTime() - 
				                     delay_ptr->InputRiseOutputFallDelay );

			input_fall_ratime.push_back( output_pnode.GetFastFallReqTime() - 
				                     delay_ptr->InputFallOutputFallDelay );

			input_rise_ratime.push_back( output_pnode.GetFastRiseReqTime() - 
				                     delay_ptr->InputRiseOutputRiseDelay );

			break;
		    }
		default:
		    assert(0);
		    break;
	    }
	} // end output

	// special case may happened at PI
	if( (input_pnode.GetFaninPtr())->GetType() == Element::GOUT_PIN && input_pnode.GetFanoutNo() > 1 )
	{
            input_fall_ratime.push_back( input_pnode.GetFastFallReqTime() );
	    input_rise_ratime.push_back( input_pnode.GetFastRiseReqTime() );
	}

	input_pnode.SetFastFallReqTime( *max_element(input_fall_ratime.begin(), input_fall_ratime.end()) );
	input_pnode.SetFastRiseReqTime( *max_element(input_rise_ratime.begin(), input_rise_ratime.end()) );

	input_fall_ratime.clear();
	input_rise_ratime.clear();
    } // end input

    return;
}

void backtraceSlowSignal( Gate &cur_gate )
{
    assert( cur_gate.GetCellPtr() != NULL && (cur_gate.GetCellPtr())->GetIsNonClocked() ); // not PI
    const Cell &cur_cell = *(cur_gate.GetCellPtr());
    const vector<vector<InputTimingTable> > &timing_vec = cur_cell.FetInputTimingVec();
    vector<double> input_fall_ratime, input_rise_ratime;
    const unsigned input_no = cur_cell.GetInputPinNo();

    for( unsigned i=0; i<input_no; ++i )
    {
	PinNode *input_pnode_ptr = cur_gate.GetInputPinNode(i);

	if( input_pnode_ptr == NULL || input_pnode_ptr->GetFaninPtr() == NULL )
	    continue;

	PinNode &input_pnode = *(input_pnode_ptr);

	for( unsigned j=0; j<cur_cell.GetOutputPinNo(); ++j )
	{
	    GOutPin &output_pin = cur_gate.FetGOutPin(j);

	    if( output_pin.GetFanoutPtr() == NULL )
		continue;

	    PinNode &output_pnode = *(output_pin.GetFanoutPtr());

	    if( output_pnode.GetFanoutPtrNo() == 0 )
		continue;

	    const InputTimingTable &timing = timing_vec[i][j];

	    switch( timing.PinTimingSense )
	    {
		case InputTimingTable::NEGATIVE_UNATE:
		    {
			assert( output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));
			input_fall_ratime.push_back( output_pnode.GetSlowRiseReqTime() - delay_ptr->DelayFromInputFall );
			input_rise_ratime.push_back( output_pnode.GetSlowFallReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::POSITIVE_UNATE:
		    {
			assert( output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::UNATE );
			UnateDelayData *delay_ptr = static_cast<UnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));
			input_fall_ratime.push_back( output_pnode.GetSlowFallReqTime() - delay_ptr->DelayFromInputFall );
			input_rise_ratime.push_back( output_pnode.GetSlowRiseReqTime() - delay_ptr->DelayFromInputRise );
			break;
		    }
		case InputTimingTable::InputTimingTable::NON_UNATE:
		    {
			assert( output_pin.GetSlowDelayDataPtr(i) != NULL );
			assert( output_pin.GetSlowDelayDataPtr(i)->GetType() == DelayData::NON_UNATE );
			NonUnateDelayData *delay_ptr = static_cast<NonUnateDelayData*>(output_pin.GetSlowDelayDataPtr(i));

			input_fall_ratime.push_back( output_pnode.GetSlowRiseReqTime() - 
				                     delay_ptr->InputFallOutputRiseDelay );

			input_rise_ratime.push_back( output_pnode.GetSlowFallReqTime() - 
				                     delay_ptr->InputRiseOutputFallDelay );

			input_fall_ratime.push_back( output_pnode.GetSlowFallReqTime() - 
				                     delay_ptr->InputFallOutputFallDelay );

			input_rise_ratime.push_back( output_pnode.GetSlowRiseReqTime() - 
				                     delay_ptr->InputRiseOutputRiseDelay );

			break;
		    }
		default:
		    assert(0);
		    break;
	    }
	} // end output

	// special case may happened at PI
	if( (input_pnode.GetFaninPtr())->GetType() == Element::GOUT_PIN && input_pnode.GetFanoutNo() > 1 )
	{
            input_fall_ratime.push_back( input_pnode.GetSlowFallReqTime() );
	    input_rise_ratime.push_back( input_pnode.GetSlowRiseReqTime() );
	}

	input_pnode.SetSlowFallReqTime( *min_element(input_fall_ratime.begin(), input_fall_ratime.end()) );
	input_pnode.SetSlowRiseReqTime( *min_element(input_rise_ratime.begin(), input_rise_ratime.end()) );

	input_fall_ratime.clear();
	input_rise_ratime.clear();
    } // end input

    return;
}
