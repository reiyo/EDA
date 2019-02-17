/************************************************************************
 *   Define member functions of class Gate: PrintGateData()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <iostream>

#include "Gate.h"

using namespace std;

void Gate::PrintGateData()
{
    if( _cell_ptr == NULL )
    {
	if( _input_vec.size() == 0 ) // primary input
	{
	    assert( _output_vec.size() == 1 );
	    cout << "gate PI" << endl;
	}
	else // primary output
	{
	    assert( _input_vec.size() == 1 && _output_vec.size() == 0 );
	    cout << "gate PO" << endl;
	}
    }
    else
	cout << "gate " << _cell_ptr->GetName() << ", input " << _input_vec.size() << ", output " << _output_vec.size() << endl;

    for( unsigned i=0; i<_input_vec.size(); ++i )
    {
	assert( _input_vec[i].GetPinId() == i && _input_vec[i].GetGatePtr() == this );
	assert( _input_vec[i].GetFaninPtr() != NULL );
	PinNode *pin_node_ptr = _input_vec[i].GetFaninPtr();

	if( _cell_ptr == NULL )
	    cout << "  input " << pin_node_ptr->GetName() << endl;
	else
	{
	    cout << "  input " << _cell_ptr->GetInputPinName(i) << ": ";
	    
	    if( pin_node_ptr != NULL ) 
		cout << pin_node_ptr->GetName() << endl;
	    else
		cout << "NULL" << endl;
	}
    }

    for( unsigned i=0; i<_output_vec.size(); ++i )
    {
	assert( _output_vec[i].GetPinId() == i && _output_vec[i].GetGatePtr() == this);

        if( _output_vec[i].GetFanoutPtr() == NULL )
	    continue;

	PinNode *pin_node_ptr = _output_vec[i].GetFanoutPtr();

	if( _cell_ptr == NULL )
	    cout << "  output " << pin_node_ptr->GetName() << endl;
	else
	{
	    cout << "  output " << _cell_ptr->GetOutputPinName(i) << ": ";

	    if( pin_node_ptr != NULL )
		cout << pin_node_ptr->GetName() << endl;
	    else
		cout << "NULL" << endl;
	}
    }

    return;
}
