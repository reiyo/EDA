/************************************************************************
 *   Define member functions of class PinNode: PrintPinNodeWiringData()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "Cell.h"
#include "Gate.h"
#include "PinNode.h"

using namespace std;

// Note the input augement n becomes NULL afterwards
unsigned PinNode::GrabFanoutRCTreeNodeId(string &n)
{
    unsigned i=0;

    for( ; i<_fanout_rc_tree.size(); ++i )
	if( _fanout_rc_tree[i].GetName() == n )
	    return i;

    _fanout_rc_tree.push_back( RCTreeNode(n) );
    assert( _fanout_rc_tree.size() == (i+1) );

    return i;
}

// Note the input augement n becomes NULL afterwards
unsigned PinNode::GrabFanoutRCTreeNodeId(const char *n)
{
    string name = n;
    unsigned i=0;

    for( ; i<_fanout_rc_tree.size(); ++i )
	if( _fanout_rc_tree[i].GetName() == name )
	    return i;

    _fanout_rc_tree.push_back( RCTreeNode(name) );
    assert( _fanout_rc_tree.size() == (i+1) );

    return i;
}

void PinNode::PrintPinNodeWiringData() const
{
    cout << "pin-node " << _name << endl;

    if( _fanin_ptr == NULL )
	cout << "  fanin: NULL" << endl;
    else
    {
	cout << "  fanin: ";
	assert( _fanin_ptr->GetType() == PIN_NODE || _fanin_ptr->GetType() == GOUT_PIN );

	if( _fanin_ptr->GetType() == PIN_NODE )
	    cout << "pin-node " << static_cast<PinNode*>(_fanin_ptr)->GetName() << endl;
	else
	{
	    GOutPin *gOutPin_ptr = static_cast<GOutPin*>(_fanin_ptr);
	    Gate *gate_ptr = gOutPin_ptr->GetGatePtr();
	    assert( gate_ptr != NULL );
	    Cell *cell_ptr = gate_ptr->GetCellPtr();

	    if( cell_ptr == NULL )
	    {
		assert( gOutPin_ptr->GetPinId() == 0 );
		cout << "gate PI" << endl;
	    }
	    else
	    {
		const unsigned pin_id = gOutPin_ptr->GetPinId();
		cout << "gate " << cell_ptr->GetName() << "'s output " << cell_ptr->GetOutputPinName(pin_id) << endl;
	    }
	}
    }

    for( unsigned i=0; i<_fanout_ptr_vec.size(); ++i )
    {
	cout << "  fanout " << i << ": ";
	assert( _fanout_ptr_vec[i]->GetType() == PIN_NODE || _fanout_ptr_vec[i]->GetType() == GIN_PIN );

	if( _fanout_ptr_vec[i]->GetType() == PIN_NODE )
	    cout << "pin-node " << static_cast<PinNode*>(_fanout_ptr_vec[i])->GetName() << endl;
	else
	{
	    GInPin *gInPin_ptr = static_cast<GInPin*>(_fanout_ptr_vec[i]);
	    Gate *gate_ptr = gInPin_ptr->GetGatePtr();
	    assert( gate_ptr != NULL );
	    Cell *cell_ptr = gate_ptr->GetCellPtr();

	    if( cell_ptr == NULL )
	    {
		assert( gInPin_ptr->GetPinId() == 0 );
		cout << "gate PO" << endl;
	    }
	    else
	    {
		const unsigned pin_id = gInPin_ptr->GetPinId();
		cout << "gate " << cell_ptr->GetName() << "'s input " << cell_ptr->GetInputPinName(pin_id) << endl;
	    }
	}
    }

    assert( _fanin_ptr != NULL && _fanout_ptr_vec.size() > 0 );

    cout << "  RC tree: ";

    for( unsigned i=0; i<_fanout_rc_tree.size(); ++i )
	cout << _fanout_rc_tree[i].GetName() << " ";

    cout << endl;

    return;
}
