/************************************************************************
 *   Define the data structures of RC tree nodes for wiring delay
 *   estimation.
 *
 *   Defined class: RCTreeNode
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef RCTREE_NODE_H
#define RCTREE_NODE_H

#include <list>
#include <string>
#include <utility>

#include "PinNode.h" // can be removed due to cross reference to each other

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class PinNode;    // defined in PinNode.h (cross reference)
class RCTreeNode;

//-----------------------------------------------------------------------
//    Define class
//-----------------------------------------------------------------------

class RCTreeNode
{
    public:
	friend class Circuit;

	RCTreeNode(std::string &n): _pin_node_ptr(NULL), _cap(0.0) { _name.swap(n); }
	RCTreeNode(const std::string &n, PinNode* pn_ptr): _name(n), _pin_node_ptr(pn_ptr), _cap(0.0) {}
 
	std::string GetName() const    { return _name; }
	PinNode* GetPinNodePtr()       { return _pin_node_ptr; }
	unsigned GetFaninId() const    { return _fanin_id_res.first; }
	double GetCap() const          { return _cap; }

	void SetFaninIdRes(const std::pair<unsigned, double> &p) { _fanin_id_res = p; }

	PinNode& FetPinNode() { return (*_pin_node_ptr); }
	std::list<std::pair<unsigned, double> >& FetFanoutIdResList() { return _fanout_id_res_list; }
	const std::list<std::pair<unsigned, double> >& FetFanoutIdResList() const { return _fanout_id_res_list; }

    private:
	std::string _name;
	PinNode* _pin_node_ptr; // an RC tree leaf must be connected to a pin node, but not vice versa

	// pair of node id of _fanout_rc_tree in PinNode.h and resistance between current and fanin nodes
	// _fanout_id_res_list contains all adjacent nodes including fanin node before injecting wiring effects
	std::pair<unsigned, double> _fanin_id_res; 
	std::list<std::pair<unsigned, double> > _fanout_id_res_list; 
	double _cap; // capacitance of current node
};

#endif // RCTREE_NODE_H
