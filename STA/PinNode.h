/************************************************************************
 *   Define the data structures of pin node for connectivity.
 *
 *   Defined class: PinNode
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef PIN_NODE_H
#define PIN_NODE_H

#include <cassert>
#include <string>
#include <vector>

#include "Element.h"
#include "parameterDefine.h"
#include "RCTreeNode.h"

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class PinNode;
class RCTreeNode; // defined in RCTreeNode.h (cross reference)

//-----------------------------------------------------------------------
//    Define class
//-----------------------------------------------------------------------

class PinNode: public Element
{
    public:
	friend class Circuit;

	// Constructors
	PinNode(const char *n): _name(n), _is_not_visited(true), _fast_fall_arr_time(0.0), _fast_fall_req_time(MIN_REQ_TIME), _fast_fall_slew(0.0), _fast_rise_arr_time(0.0), _fast_rise_req_time(MIN_REQ_TIME), _fast_rise_slew(0.0), _slow_fall_arr_time(0.0), _slow_fall_req_time(MAX_REQ_TIME), _slow_fall_slew(0.0), _slow_rise_arr_time(0.0), _slow_rise_req_time(MAX_REQ_TIME), _slow_rise_slew(0.0), _fall_slew_hat_sq(0.0), _rise_slew_hat_sq(0.0), _fanin_ptr(NULL), _fanout_pin_node_no(0) {}

	PinNode(const std::string &n): _name(n), _is_not_visited(true), _fast_fall_arr_time(0.0), _fast_fall_req_time(MIN_REQ_TIME), _fast_fall_slew(0.0), _fast_rise_arr_time(0.0), _fast_rise_req_time(MIN_REQ_TIME), _fast_rise_slew(0.0), _slow_fall_arr_time(0.0), _slow_fall_req_time(MAX_REQ_TIME), _slow_fall_slew(0.0), _slow_rise_arr_time(0.0), _slow_rise_req_time(MAX_REQ_TIME), _slow_rise_slew(0.0), _fall_slew_hat_sq(0.0), _rise_slew_hat_sq(0.0), _fanin_ptr(NULL), _fanout_pin_node_no(0) {}

	// Parents
	Type GetType() { return PIN_NODE; }

	// Get members
	std::string GetName() { return _name; }
	bool GetIsNotVisited() const { return _is_not_visited; }

	double GetFastFallArrTime() { return _fast_fall_arr_time; }
	double GetFastFallReqTime() { return _fast_fall_req_time; }
	double GetFastFallSlew()    { return _fast_fall_slew; }
	double GetFastRiseArrTime() { return _fast_rise_arr_time; }
	double GetFastRiseReqTime() { return _fast_rise_req_time; }
	double GetFastRiseSlew()    { return _fast_rise_slew; }
	double GetSlowFallArrTime() { return _slow_fall_arr_time; }
	double GetSlowFallReqTime() { return _slow_fall_req_time; }
	double GetSlowFallSlew()    { return _slow_fall_slew; }
	double GetSlowRiseArrTime() { return _slow_rise_arr_time; }
	double GetSlowRiseReqTime() { return _slow_rise_req_time; }
	double GetSlowRiseSlew()    { return _slow_rise_slew; }

	double GetFallSlewHatSq()   { return _fall_slew_hat_sq; }
	double GetRiseSlewHatSq()   { return _rise_slew_hat_sq; }

	Element* GetFaninPtr() { 
	    assert( _fanin_ptr == NULL || (_fanin_ptr->GetType()==PIN_NODE || _fanin_ptr->GetType()==GOUT_PIN) );
	    return _fanin_ptr; }
	unsigned GetFanoutPinNodeNo() const { return _fanout_pin_node_no; }
	Element* GetFanoutPtr(unsigned id) { 
	    assert( id < _fanout_ptr_vec.size() ); 
	    assert( _fanout_ptr_vec[id] == NULL || (_fanout_ptr_vec[id]->GetType()==PIN_NODE || _fanout_ptr_vec[id]->GetType()==GIN_PIN) );
	    return _fanout_ptr_vec[id]; }

	unsigned GetFanoutPtrNo()   { return _fanout_ptr_vec.size(); }
	unsigned GetFanoutNo()      { return _fanout_ptr_vec.size(); }

	// Set members
	void SetIsVisited()                 { _is_not_visited = false; }
	void SetFastFallArrTime(double val) { _fast_fall_arr_time = val; } 
	void SetFastFallReqTime(double val) { if(val > _fast_fall_req_time) {_fast_fall_req_time = val;} }
	void SetFastFallSlew(double val)    { _fast_fall_slew = val; }
	void SetFastRiseArrTime(double val) { _fast_rise_arr_time = val; }
	void SetFastRiseReqTime(double val) { if(val > _fast_rise_req_time) {_fast_rise_req_time = val;} }
	void SetFastRiseSlew(double val)    { _fast_rise_slew = val; }
	void SetSlowFallArrTime(double val) { _slow_fall_arr_time = val; }
	void SetSlowFallReqTime(double val) { if(val < _slow_fall_req_time) {_slow_fall_req_time = val;} }
	void SetSlowFallSlew(double val)    { _slow_fall_slew = val; }
	void SetSlowRiseArrTime(double val) { _slow_rise_arr_time = val; }
	void SetSlowRiseReqTime(double val) { if(val < _slow_rise_req_time) {_slow_rise_req_time = val;} }
	void SetSlowRiseSlew(double val)    { _slow_rise_slew = val; }

	void SetFallSlewHatSq(double val)   { _fall_slew_hat_sq = val; }
	void SetRiseSlewHatSq(double val)   { _rise_slew_hat_sq = val; }

	void SetFaninPtr(Element *element_ptr)    { _fanin_ptr = element_ptr; }
	void SetFanoutPtrVec(unsigned id, Element *element_ptr) {
	    assert(id < _fanout_ptr_vec.size()); _fanout_ptr_vec[id] = element_ptr; }

        std::vector<RCTreeNode>& FetFanoutRCTree() { return _fanout_rc_tree; }

	// Others
	unsigned GrabFanoutRCTreeNodeId(std::string &n); // get id if exists, if not, new a node, then swap name, return id
	unsigned GrabFanoutRCTreeNodeId(const char *n);
	void PrintPinNodeWiringData() const;

    private:
	std::string _name;
	bool _is_not_visited;

	// early timing analysis
	double _fast_fall_arr_time;
        double _fast_fall_req_time;
	double _fast_fall_slew;
	double _fast_rise_arr_time;
	double _fast_rise_req_time;
	double _fast_rise_slew;

	// late timing analysis
	double _slow_fall_arr_time;
        double _slow_fall_req_time;
	double _slow_fall_slew;
	double _slow_rise_arr_time;
	double _slow_rise_req_time;
	double _slow_rise_slew;

	double _fall_slew_hat_sq; // square of output slew hat 
	double _rise_slew_hat_sq; 

	// connect to pin node or gate input/output pin 
	Element *_fanin_ptr;
	std::vector<Element *> _fanout_ptr_vec;
	unsigned _fanout_pin_node_no; // use for very special case that leaf_pnode_no < _fanout_ptr_vec.size()

	std::vector<RCTreeNode> _fanout_rc_tree;
};

#endif // PIN_NODE_H
