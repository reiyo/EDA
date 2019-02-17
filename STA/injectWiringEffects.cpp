/************************************************************************
 *   Inject circuit pin-node loads and Elmore delays in parallel mode.
 *   Note that no fanout RC tree is left after injecting wiring effects.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <iostream>
#include <list>
#include <utility>

#include "Circuit.h"
#include "floatCompareDefine.h"
#include "process.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare main function
//-----------------------------------------------------------------------

void injectWiringEffects( Circuit &circuit );

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

void accumulateLoads( const vector<RCTreeNode> &fanout_rc_tree, const vector<unsigned> &reverse_vec, vector<double> &fall_cap_table, vector<double> &rise_cap_table );

void adjustLinkingAndGetReverseOrder( vector<RCTreeNode> &fanout_rc_tree, vector<unsigned> &reverse_vec );

void computeElmoreDelays( const vector<unsigned> &reverse_vec, vector<RCTreeNode> &fanout_rc_tree );

void injectWiringEffects( PinNode *pin_node_ptr );

bool is_topologically_ordered( vector<RCTreeNode> &fanout_rc_tree, const vector<unsigned> &reverse_vec );

void resistShortCircuit( PinNode &cur_pnode );

void resistShortCircuit( PinNode &cur_pnode, GOutPin &gout_pin, const double &fall_cap, const double &rise_cap );

//-----------------------------------------------------------------------
//    Define main function
//-----------------------------------------------------------------------

void injectWiringEffects( Circuit &circuit )
{
    for( unsigned i=0; i<circuit.GetPinNodeNo(); ++i )
	injectWiringEffects( circuit.GetPinNodePtr(i) );

    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

void accumulateLoads( const vector<RCTreeNode> &fanout_rc_tree, const vector<unsigned> &reverse_vec, vector<double> &fall_cap_table, vector<double> &rise_cap_table )
{
    // accumulate loads in reverse order
    for( unsigned i=0; i<fanout_rc_tree.size(); ++i )
    {
        const unsigned cur_id = reverse_vec[i];
	const list<pair<unsigned, double> > &fanout_id_res_list = fanout_rc_tree[cur_id].FetFanoutIdResList();
	list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	{
	    const unsigned fanout_id = (*listIter).first;

	    fall_cap_table[cur_id] += fall_cap_table[fanout_id];
	    rise_cap_table[cur_id] += rise_cap_table[fanout_id];
	}
    }

    return;
}

// decompose fanin and fanout sets from the original fanout list for each RCTreeNode, and 
// find a reverse traversal order, reverse_vec, of the given tree, i.e., from leaves to root in topological
void adjustLinkingAndGetReverseOrder( vector<RCTreeNode> &fanout_rc_tree, vector<unsigned> &reverse_vec )
{
    assert( fanout_rc_tree.size() > 1 ); // at least a root and a leaf
    const int node_no = static_cast<int>(fanout_rc_tree.size());
    reverse_vec.assign(node_no, 0);
    vector<bool> unreached_table(node_no, true);

    unreached_table[0] = false; // traverse the root
    int next_id = node_no-2;    // position to be inserted

    for( int i=node_no-1; i>=0; --i )
    {
	const unsigned cur_id = reverse_vec[i];
	list<pair<unsigned, double> > &adjacent_list = fanout_rc_tree[cur_id].FetFanoutIdResList();
	list<pair<unsigned, double> >::iterator listIter = adjacent_list.begin();

	for( ; listIter!=adjacent_list.end(); )
	{
	    const unsigned adj_id = (*listIter).first;

	    if( unreached_table[adj_id] )
	    {
		reverse_vec[next_id--] = adj_id;
                unreached_table[adj_id] = false;
		++listIter;
	    }
	    else
	    {
		fanout_rc_tree[cur_id].SetFaninIdRes((*listIter));
		listIter = adjacent_list.erase(listIter);
	    }
	}
    }

    assert( is_topologically_ordered(fanout_rc_tree, reverse_vec) );

    return;
}

void computeElmoreDelays( const vector<unsigned> &reverse_vec, vector<RCTreeNode> &fanout_rc_tree )
{
    const unsigned node_no = fanout_rc_tree.size();
    RCTreeNode &root_node = fanout_rc_tree[0];
    PinNode &root_pnode = *(root_node.GetPinNodePtr());
    const unsigned leaf_no = root_pnode.GetFanoutPinNodeNo();
    vector<double> fall_cap_table(node_no);
    vector<double> rise_cap_table(node_no);

    // initialize 
    for( unsigned i=0; i<node_no; ++i )
    {
        fall_cap_table[i] = fanout_rc_tree[i].GetCap(); // id is one-by-one mapping
	rise_cap_table[i] = fall_cap_table[i];
    }

    vector<double> pin_fall_cap_table(leaf_no+1, 0.0); // input pin falling capacitance, the one in id 0 is no use
    vector<double> pin_rise_cap_table(leaf_no+1, 0.0);

    // inject gate input capacitance to tap nodes
    for( unsigned i=1; i<=leaf_no; ++i )
    {
	const unsigned fanout_no = (fanout_rc_tree[i].GetPinNodePtr())->GetFanoutNo();

        for( unsigned j=0; j<fanout_no; ++j ) // consider if a pin node drives multiple gates although it may not be possible
	{
	    const GInPin *gInPin_ptr = static_cast<GInPin*>((fanout_rc_tree[i].GetPinNodePtr())->GetFanoutPtr(j));
	    const unsigned cell_pin_id = gInPin_ptr->GetPinId();
	    const Gate *gate_ptr = gInPin_ptr->GetGatePtr();

	    if( gate_ptr->GetCellPtr() != NULL ) // otherwise it is PO
	    {
		pin_fall_cap_table[i] += gate_ptr->GetInputFallCap(cell_pin_id);
		pin_rise_cap_table[i] += gate_ptr->GetInputRiseCap(cell_pin_id);
	    }
	}

	fall_cap_table[i] += pin_fall_cap_table[i];
	rise_cap_table[i] += pin_rise_cap_table[i];
    }

    accumulateLoads( fanout_rc_tree, reverse_vec, fall_cap_table, rise_cap_table ); // accumulate loads in reverse order

    { // inject accumulated loadings in gate output pin
	GOutPin *gOutPin_ptr = static_cast<GOutPin*>(root_pnode.GetFaninPtr());
	assert( gOutPin_ptr!=NULL );

	if( gOutPin_ptr == NULL ) // very special case
	    return;

	assert( root_pnode.GetFanoutPtrNo() == leaf_no );

	if( root_pnode.GetFanoutPtrNo() == leaf_no )
	{
	    gOutPin_ptr->SetFallLoad(fall_cap_table[0]);
	    gOutPin_ptr->SetRiseLoad(rise_cap_table[0]);
	}
	else // very special case
	{
	    assert( root_pnode.GetFanoutPtrNo() > leaf_no );
	    resistShortCircuit( root_pnode, *gOutPin_ptr, fall_cap_table[0], rise_cap_table[0] );
	}
    }

    vector<double> fall_delay_table(node_no, 0.0);
    vector<double> rise_delay_table(node_no, 0.0);

    { // accumulate delays from root to leaves and update fall_cap_table & rise_cap_table simultaneously
	{ // root node
	    const list<pair<unsigned, double> > &fanout_id_res_list = root_node.FetFanoutIdResList();
	    list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	    for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	    {
		const unsigned fanout_id = (*listIter).first;

		fall_delay_table[fanout_id] = ((*listIter).second * fall_cap_table[fanout_id]);
                rise_delay_table[fanout_id] = ((*listIter).second * rise_cap_table[fanout_id]);
	    }

	    fall_cap_table[0] = 0.0; // since local accumulated delay is zero
	    rise_cap_table[0] = 0.0;
	}

	// other nodes
	for( int i=static_cast<int>(node_no)-2; i>=0; --i )
	{
	    const unsigned cur_id = reverse_vec[i];
	    const RCTreeNode &cur_node = fanout_rc_tree[cur_id];
	    const list<pair<unsigned, double> > &fanout_id_res_list = cur_node.FetFanoutIdResList();
	    list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	    for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	    {
		const unsigned fanout_id = (*listIter).first;

		fall_delay_table[fanout_id] = ((*listIter).second * fall_cap_table[fanout_id]) + fall_delay_table[cur_id];
		rise_delay_table[fanout_id] = ((*listIter).second * rise_cap_table[fanout_id]) + rise_delay_table[cur_id];
	    }

	    if( cur_id > leaf_no )
	    {
		fall_cap_table[cur_id] = cur_node.GetCap() * fall_delay_table[cur_id]; // replace for slew computation
		rise_cap_table[cur_id] = cur_node.GetCap() * rise_delay_table[cur_id];
	    }
	    else
	    {
		fall_cap_table[cur_id] = (cur_node.GetCap()+pin_fall_cap_table[cur_id]) * fall_delay_table[cur_id];
		rise_cap_table[cur_id] = (cur_node.GetCap()+pin_rise_cap_table[cur_id]) * rise_delay_table[cur_id];
	    }
	}
    }

    accumulateLoads( fanout_rc_tree, reverse_vec, fall_cap_table, rise_cap_table ); // accumulate loads in reverse order
    vector<double> fall_beta_table(node_no);
    vector<double> rise_beta_table(node_no);

    {
	{ // root node
	    const list<pair<unsigned, double> > &fanout_id_res_list = root_node.FetFanoutIdResList();
	    list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	    for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	    {
		const unsigned fanout_id = (*listIter).first;

		fall_beta_table[fanout_id] = ((*listIter).second * fall_cap_table[fanout_id]);
                rise_beta_table[fanout_id] = ((*listIter).second * rise_cap_table[fanout_id]);
	    }
	}

	// other nodes
	for( int i=static_cast<int>(node_no)-2; i>=0; --i )
	{
	    const unsigned cur_id = reverse_vec[i];
	    const RCTreeNode &cur_node = fanout_rc_tree[cur_id];
	    const list<pair<unsigned, double> > &fanout_id_res_list = cur_node.FetFanoutIdResList();
	    list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	    for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	    {
		const unsigned fanout_id = (*listIter).first;

		fall_beta_table[fanout_id] = ((*listIter).second * fall_cap_table[fanout_id]) + fall_beta_table[cur_id];
		rise_beta_table[fanout_id] = ((*listIter).second * rise_cap_table[fanout_id]) + rise_beta_table[cur_id];
	    }
	}
    }

    // inject Elmore delay and beta square values to pin nodes
    for( unsigned i=1; i<=leaf_no; ++i )
    {
        PinNode &cur_pin_node = fanout_rc_tree[i].FetPinNode();
	double cur_fall_delay = fall_delay_table[i];
	double cur_rise_delay = rise_delay_table[i];

	for( unsigned j=0; j<cur_pin_node.GetFanoutNo(); ++j )
	{
	    assert( cur_pin_node.GetFanoutPtr(j) != NULL && (cur_pin_node.GetFanoutPtr(j))->GetType() == Element::GIN_PIN );
	    GInPin &gin_pin = *(static_cast<GInPin*>(cur_pin_node.GetFanoutPtr(j)));
	    gin_pin.SetFallArrDelay(cur_fall_delay);
	    gin_pin.SetRiseArrDelay(cur_rise_delay);
	}
	
	assert( DGE( (cur_fall_delay * cur_fall_delay) - (2 * fall_beta_table[i]), 0.0 ) );
	assert( DGE( (cur_rise_delay * cur_rise_delay) - (2 * rise_beta_table[i]), 0.0 ) );
	cur_pin_node.SetFallSlewHatSq( (2 * fall_beta_table[i]) - (cur_fall_delay * cur_fall_delay) );
	cur_pin_node.SetRiseSlewHatSq( (2 * rise_beta_table[i]) - (cur_rise_delay * cur_rise_delay) );
    }

    return;
}

inline void injectWiringEffects( PinNode *pin_node_ptr )
{
    vector<RCTreeNode> &fanout_rc_tree = pin_node_ptr->FetFanoutRCTree();

    if( fanout_rc_tree.size() > 0 )
    {
	assert( pin_node_ptr->GetFanoutPinNodeNo() > 0 );
	vector<unsigned> reverse_vec;

	adjustLinkingAndGetReverseOrder( fanout_rc_tree, reverse_vec ); // adjust fanin and fanouts and set a reverse vector
	computeElmoreDelays( reverse_vec, fanout_rc_tree );
	vector<RCTreeNode>().swap( fanout_rc_tree ); // free memory
    }
    else
    {
	assert( pin_node_ptr->GetFanoutPinNodeNo() == 0 );
	resistShortCircuit( *pin_node_ptr );
    }

    return;
}

// true if it satisfies both forward topological and backward topological orders
bool is_topologically_ordered( vector<RCTreeNode> &fanout_rc_tree, const vector<unsigned> &reverse_vec )
{
    assert( fanout_rc_tree.size() == reverse_vec.size() );
    const unsigned node_no = reverse_vec.size();

    { // forward test
	vector<bool> nontraversed_table( node_no, true );

        for( int i=static_cast<int>(node_no)-1; i>=0; --i )
	{
            const unsigned cur_id = reverse_vec[i];

	    if( i == static_cast<int>(node_no)-1 )
	    {
		if( cur_id != 0 )
		    return false;

		assert( fanout_rc_tree[cur_id].GetPinNodePtr() != NULL );
		assert( fanout_rc_tree[cur_id].FetFanoutIdResList().size() > 0 ); // root must have fanout
	    }
	    else
	    {
                 const unsigned fanin_id = fanout_rc_tree[cur_id].GetFaninId();
                 
		 if( nontraversed_table[fanin_id] )
		     return false;

		 const list<pair<unsigned, double> > &fanout_id_res_list = fanout_rc_tree[cur_id].FetFanoutIdResList();
		 list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

		 for( ; listIter!=fanout_id_res_list.end(); ++listIter )
		 {
		     const unsigned fanout_id = (*listIter).first;
		     assert( DGE( (*listIter).second, 0.0 ) );

                     if( !(nontraversed_table[fanout_id]) )
			 return false;
		 }

		 // tree leaf must be connected to pin-node, but
		 // be careful, a tree node connected to pin node may not be in tree leaf
		 assert( fanout_id_res_list.size() > 0 || fanout_rc_tree[cur_id].GetPinNodePtr() != NULL );
	    }

	    assert( DGE( fanout_rc_tree[cur_id].GetCap(), 0.0 ) ); // every node has a specified capacitance
	    nontraversed_table[cur_id] = false;
	}
    }

    { // backward test
	vector<bool> nontraversed_table( node_no, true );

	for( unsigned i=0; i<reverse_vec.size()-1; ++i )
	{
	    const unsigned cur_id = reverse_vec[i];
	    const unsigned fanin_id = fanout_rc_tree[cur_id].GetFaninId();

	    if( !(nontraversed_table[fanin_id]) )
		return false;

	    const list<pair<unsigned, double> > &fanout_id_res_list = fanout_rc_tree[cur_id].FetFanoutIdResList();
	    list<pair<unsigned, double> >::const_iterator listIter = fanout_id_res_list.begin();

	    for( ; listIter!=fanout_id_res_list.end(); ++listIter )
	    {
		const unsigned fanout_id = (*listIter).first;

		if( nontraversed_table[fanout_id] )
		    return false;
	    }

	    nontraversed_table[cur_id] = false;
	}
    }

    return true;
}

inline void resistShortCircuit( PinNode &cur_pnode )
{
    Element *fanin_ptr = cur_pnode.GetFaninPtr();

    if( fanin_ptr == NULL || fanin_ptr->GetType() != Element::GOUT_PIN || 
	    static_cast<GOutPin*>(fanin_ptr)->GetGatePtr()->GetCellPtr() == NULL )
	return;

    //printf( "it is %s\n", cur_pnode.GetName().c_str() );

    double fall_load = 0.0, rise_load=0.0;

    for( unsigned i=0; i<cur_pnode.GetFanoutPtrNo(); ++i )
    {
	assert( (cur_pnode.GetFanoutPtr(i))->GetType() == Element::GIN_PIN );
	GInPin &gin_pin = *(static_cast<GInPin*>(cur_pnode.GetFanoutPtr(i)));
        Cell *cell_ptr = (gin_pin.GetGatePtr())->GetCellPtr();

	if( cell_ptr == NULL ) // PO
	    return;

	fall_load += cell_ptr->GetInputFallCap(gin_pin.GetPinId());
	rise_load += cell_ptr->GetInputRiseCap(gin_pin.GetPinId());
    }

    //printf( "load = %le, %le\n", fall_load, rise_load );

    static_cast<GOutPin*>(fanin_ptr)->SetFallLoad( fall_load );
    static_cast<GOutPin*>(fanin_ptr)->SetRiseLoad( rise_load );

    return;
}

void resistShortCircuit( PinNode &cur_pnode, GOutPin &gout_pin, const double &fall_cap, const double &rise_cap )
{
    double total_fall_load = fall_cap; 
    double total_rise_load = rise_cap;

    for( unsigned i=0; i<cur_pnode.GetFanoutPtrNo(); ++i )
    {
	if( (cur_pnode.GetFanoutPtr(i))->GetType() != Element::GIN_PIN )
	    continue;

	GInPin &gin_pin = *(static_cast<GInPin*>(cur_pnode.GetFanoutPtr(i)));
	Cell *cell_ptr = (gin_pin.GetGatePtr())->GetCellPtr();

	if( cell_ptr == NULL ) // PO
	    return;

	total_fall_load += cell_ptr->GetInputFallCap(gin_pin.GetPinId());
	total_rise_load += cell_ptr->GetInputRiseCap(gin_pin.GetPinId());
    }

    gout_pin.SetFallLoad( total_fall_load );
    gout_pin.SetRiseLoad( total_rise_load );

    return;
}
