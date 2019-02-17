/************************************************************************
 *   Define printing-related member functions of class Circuit.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>

#include "Circuit.h"
#include "PinNode.h"
#include "RATData.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

bool cmpPinNodePtrNameSort( PinNode *a_ptr, PinNode *b_ptr );

//-----------------------------------------------------------------------
//    Define member functions
//-----------------------------------------------------------------------

// display for checking
void Circuit::PrintCircuitData() const
{
    cout << "#.. Print current circuit" << endl;

    if( _is_sequential )
    {
	assert( _SeqGate_ptr_vec.size() > 0 );
	cout << "Sequential circuit:" << endl;
    }
    else
    {
	assert( _SeqGate_ptr_vec.size() == 0 );
	cout << "Combinational circuit:" << endl;
    }

    cout << "PI list:" << endl;

    for( unsigned i=0; i<_PI_ptr_vec.size(); ++i )
	_PI_ptr_vec[i]->PrintGateData();

    cout << "PO list:" << endl;

    for( unsigned i=0; i<_PO_ptr_vec.size(); ++i )
	_PO_ptr_vec[i]->PrintGateData();

    cout << "Com. gates:" << endl;

    for( unsigned i=0; i<_ComGate_ptr_vec.size(); ++i )
	_ComGate_ptr_vec[i]->PrintGateData();

    if( _is_sequential )
    {
	cout << "Seq. gates:" << endl;

	for( unsigned i=0; i<_SeqGate_ptr_vec.size(); ++i )
	    _SeqGate_ptr_vec[i]->PrintGateData();
    }

    for( unsigned i=0; i<_PinNode_ptr_vec.size(); ++i )
        _PinNode_ptr_vec[i]->PrintPinNodeWiringData();

    for( unsigned i=0; i<_RATData_vec.size(); ++i )
	_RATData_vec[i].PrintRATData();

    cout << "#.. Done circuit printing." << endl;

    return;
}

void Circuit::PrintTimingData() 
{
    const unsigned po_no = _PO_ptr_vec.size();
    vector<PinNode*> PO_PinNode_ptr_vec;
    PO_PinNode_ptr_vec.resize( po_no );

    for( unsigned i=0; i<po_no; ++i )
	PO_PinNode_ptr_vec[i] = _PO_ptr_vec[i]->GetInputPinNode(0);

    sort( PO_PinNode_ptr_vec.begin(), PO_PinNode_ptr_vec.end(), cmpPinNodePtrNameSort );

    for( unsigned i=0; i<po_no; ++i )
    {
	PinNode &cur_pnode = *(PO_PinNode_ptr_vec[i]);
	printf( "at %s %.5le %.5le %.5le %.5le %.5le %.5le %.5le %.5le\n", cur_pnode.GetName().c_str(), 
		cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastRiseArrTime(), 
		cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowRiseArrTime(), 
		cur_pnode.GetFastFallSlew(), cur_pnode.GetFastRiseSlew(), 
		cur_pnode.GetSlowFallSlew(), cur_pnode.GetSlowRiseSlew() );
    }

    if( _is_sequential || _RATData_vec.size() > 0 )
    {
	sort( _PinNode_ptr_vec.begin(), _PinNode_ptr_vec.end(), cmpPinNodePtrNameSort );

	for( unsigned i=0; i<_PinNode_ptr_vec.size(); ++i )
	{
	    PinNode &cur_pnode = *(_PinNode_ptr_vec[i]);

	    if( cur_pnode.GetFastFallReqTime() > NEGATIVE_BOUND )
	    {
		const double fast_fall_slack = cur_pnode.GetFastFallArrTime()-cur_pnode.GetFastFallReqTime();
		const double fast_rise_slack = cur_pnode.GetFastRiseArrTime()-cur_pnode.GetFastRiseReqTime();

		printf( "slack %s early %.5le %.5le\n", cur_pnode.GetName().c_str(), fast_fall_slack, fast_rise_slack );
	    }

	    if( cur_pnode.GetSlowFallReqTime() < POSITIVE_BOUND )
	    {
		const double slow_fall_slack = cur_pnode.GetSlowFallReqTime()-cur_pnode.GetSlowFallArrTime();
		const double slow_rise_slack = cur_pnode.GetSlowRiseReqTime()-cur_pnode.GetSlowRiseArrTime();

		printf( "slack %s late %.5le %.5le\n", cur_pnode.GetName().c_str(), slow_fall_slack, slow_rise_slack ); 
	    }
	}
    }

    return;
}

void Circuit::PrintTimingData( const char *file_name ) 
{
    FILE *inf_ptr = fopen( file_name, "w" );

    if( inf_ptr == NULL )
    {
	printf( "Error in opening %s for output\n", file_name );
	printf( "  Exiting...\n" );
	exit(-1);
    }

    const unsigned po_no = _PO_ptr_vec.size();
    vector<PinNode*> PO_PinNode_ptr_vec;
    PO_PinNode_ptr_vec.resize( po_no );

    for( unsigned i=0; i<po_no; ++i )
	PO_PinNode_ptr_vec[i] = _PO_ptr_vec[i]->GetInputPinNode(0);

    sort( PO_PinNode_ptr_vec.begin(), PO_PinNode_ptr_vec.end(), cmpPinNodePtrNameSort );

    for( unsigned i=0; i<po_no; ++i )
    {
	PinNode &cur_pnode = *(PO_PinNode_ptr_vec[i]);
	fprintf( inf_ptr, "at %s %.5le %.5le %.5le %.5le %.5le %.5le %.5le %.5le\n", cur_pnode.GetName().c_str(), 
		 cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastRiseArrTime(), 
		 cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowRiseArrTime(), 
		 cur_pnode.GetFastFallSlew(), cur_pnode.GetFastRiseSlew(), 
		 cur_pnode.GetSlowFallSlew(), cur_pnode.GetSlowRiseSlew() );
    }

    if( _is_sequential || _RATData_vec.size() > 0 )
    {
	sort( _PinNode_ptr_vec.begin(), _PinNode_ptr_vec.end(), cmpPinNodePtrNameSort );

	for( unsigned i=0; i<_PinNode_ptr_vec.size(); ++i )
	{
	    PinNode &cur_pnode = *(_PinNode_ptr_vec[i]);

//	    printf( "at %s %.5le %.5le %.5le %.5le\n", cur_pnode.GetName().c_str(), cur_pnode.GetFastFallArrTime(), cur_pnode.GetFastRiseArrTime(), cur_pnode.GetSlowFallArrTime(), cur_pnode.GetSlowRiseArrTime() );

	    if( cur_pnode.GetFastFallReqTime() > NEGATIVE_BOUND )
	    {
		const double fast_fall_slack = cur_pnode.GetFastFallArrTime()-cur_pnode.GetFastFallReqTime();
		const double fast_rise_slack = cur_pnode.GetFastRiseArrTime()-cur_pnode.GetFastRiseReqTime();

		fprintf( inf_ptr, "slack %s early %.5le %.5le\n", 
		     	 cur_pnode.GetName().c_str(), fast_fall_slack, fast_rise_slack );
	    }

	    if( cur_pnode.GetSlowFallReqTime() < POSITIVE_BOUND )
	    {
		const double slow_fall_slack = cur_pnode.GetSlowFallReqTime()-cur_pnode.GetSlowFallArrTime();
		const double slow_rise_slack = cur_pnode.GetSlowRiseReqTime()-cur_pnode.GetSlowRiseArrTime();

		fprintf( inf_ptr, "slack %s late %.5le %.5le\n", 
			 cur_pnode.GetName().c_str(), slow_fall_slack, slow_rise_slack ); 
	    }
	}
    }

    fclose(inf_ptr);

    return;
}
//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

bool cmpPinNodePtrNameSort( PinNode *a_ptr, PinNode *b_ptr )
{
    return ( (*a_ptr).GetName() < (*b_ptr).GetName() );
}

