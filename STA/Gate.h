/************************************************************************
 *    Define data structure for combinational and sequential gates.
 *
 *    Defined classes: Gate, GInPin, GOutPin, SeqGate
 *   
 *    Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef GATE_H
#define GATE_H

#include <cassert>
#include <vector>

#include "Cell.h"
#include "DelayData.h"
#include "Element.h"
#include "PinNode.h"

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class Gate;
class GInPin;
class GOutPin;
class SeqGate;

//-----------------------------------------------------------------------
//    Define classes
//-----------------------------------------------------------------------

class GInPin: public Element
{
    public:
	GInPin(Gate *g_ptr): _fall_arr_delay(0.0), _rise_arr_delay(0.0), _gate_ptr(g_ptr), _fanin_ptr(NULL) {}
        GInPin(const int &pid, Gate *g_ptr, PinNode *i_ptr): _pin_id(pid), _fall_arr_delay(0.0), _rise_arr_delay(0.0), _gate_ptr(g_ptr), _fanin_ptr(i_ptr) {}

        Type GetType() { return GIN_PIN; }

	unsigned GetPinId() const        { return _pin_id; }
	double   GetFallArrDelay() const { return _fall_arr_delay; }
	double   GetRiseArrDelay() const { return _rise_arr_delay; }
	Gate*    GetGatePtr() const      { return _gate_ptr; }
	PinNode* GetFaninPtr()           { return _fanin_ptr; }

	void SetPinId(const unsigned &pid) { _pin_id = pid; }
	void SetFallArrDelay(const double &val) { _fall_arr_delay = val; }
        void SetRiseArrDelay(const double &val) { _rise_arr_delay = val; }
	void SetGatePtr(Gate *gate_ptr) { _gate_ptr = gate_ptr; }
	void SetFaninPtr(PinNode *node_ptr) { _fanin_ptr = node_ptr; }

    private:
	unsigned _pin_id;       // corresponds to index of _input_pin_name_vec of Cell class
	double _fall_arr_delay; // wiring delay to the driving pin node, but not accumulated delay
	double _rise_arr_delay;

	Gate *_gate_ptr;           // belongs to which gate
	unsigned _fanin_driven_id; // driving pin node's id of _fanout_ptr_vec
	PinNode *_fanin_ptr;       // driving node, i.e., PIN_NODE
};

class GOutPin: public Element
{
    public:
	GOutPin(Gate *g_ptr): _fall_load(0.0), _rise_load(0.0), _visited_count(0), _gate_ptr(g_ptr), _fanout_ptr(NULL) {}
	GOutPin(const int &pid, Gate *g_ptr, PinNode *o_ptr): _pin_id(pid), _fall_load(0.0), _rise_load(0.0), _visited_count(0), _gate_ptr(g_ptr), _fanout_ptr(o_ptr) {}

	Type GetType() { return GOUT_PIN; }

	unsigned   GetPinId() const                        { return _pin_id; }
        double     GetFallLoad() const                     { return _fall_load; }
	double     GetRiseLoad() const                     { return _rise_load; }
	DelayData* GetFastDelayDataPtr(const unsigned &id) { return _fast_delay_data_ptr_vec[id]; }
	DelayData* GetSlowDelayDataPtr(const unsigned &id) { return _slow_delay_data_ptr_vec[id]; }
	unsigned   GetVisitedCount() const                 { return _visited_count; }
	Gate*      GetGatePtr() const                      { return _gate_ptr; }
	PinNode*   GetFanoutPtr()                          { return _fanout_ptr; }

	void InitFastDelayDataPtrVec(const unsigned &in_no) { _fast_delay_data_ptr_vec.assign(in_no, NULL); }
	void InitSlowDelayDataPtrVec(const unsigned &in_no) { _slow_delay_data_ptr_vec.assign(in_no, NULL); }
	void IncVisitedCount() { ++_visited_count; }

	void SetPinId(const unsigned &pid) { _pin_id = pid; }
	void SetFallLoad(const double &val) { _fall_load = val; }
	void SetRiseLoad(const double &val) { _rise_load = val; }
	void SetFastDelayDataPtr(const unsigned &id, DelayData*ptr) { _fast_delay_data_ptr_vec[id] = ptr; }
	void SetSlowDelayDataPtr(const unsigned &id, DelayData*ptr) { _slow_delay_data_ptr_vec[id] = ptr; }

	void SetGatePtr(Gate *gate_ptr) { _gate_ptr = gate_ptr; }
	void SetFanoutPtr(PinNode *node_ptr) { _fanout_ptr = node_ptr; }

    private:
	unsigned _pin_id;  // corresponds to index of _output_pin_name_vec of Cell class
	double _fall_load; // output falling loading capacitance
	double _rise_load; // output rising oading capacitance

	std::vector<DelayData*> _fast_delay_data_ptr_vec; // record gate delay, index corresponding to input pin id
	std::vector<DelayData*> _slow_delay_data_ptr_vec;

	unsigned _visited_count; // for backward STA

	Gate *_gate_ptr;
	PinNode *_fanout_ptr; // driven node, i.e., pin node
};

// combinational gate including PI, PO
class Gate: public Element
{
    public:
	friend class Circuit;

	Gate(): _cell_ptr(NULL), _is_non_clocked(true), _input_visited_count(0) {}
	Gate(Cell *c_ptr);

	virtual Type GetType() { return GATE; }

	Cell* GetCellPtr() const { return _cell_ptr; }
	bool GetIsNonClocked() const { return _is_non_clocked; }
	unsigned GetClockPinId() const { assert( _cell_ptr!=NULL && !_is_non_clocked ); return _cell_ptr->GetClockPinId(); }
	unsigned GetInputNo() const { return _input_vec.size(); }
	unsigned GetOutputNo() const { return _output_vec.size(); }
	unsigned GetInputVisitedCount() const { return _input_visited_count; }

	double GetInputFallCap(const unsigned &id) const { return _cell_ptr->GetInputFallCap(id); }
	double GetInputRiseCap(const unsigned &id) const { return _cell_ptr->GetInputRiseCap(id); }
	double GetOutputFallLoad(const unsigned &id) const { return _output_vec[id].GetFallLoad(); }
	double GetOutputRiseLoad(const unsigned &id) const { return _output_vec[id].GetRiseLoad(); }
	PinNode* GetInputPinNode(const unsigned &id) { return _input_vec[id].GetFaninPtr(); }
	PinNode* GetOutputPinNode(const unsigned &id) { return _output_vec[id].GetFanoutPtr(); }

	GInPin  &FetGInPin(const unsigned &id)  { return _input_vec[id]; }
        GOutPin &FetGOutPin(const unsigned &id) { return _output_vec[id]; }

	void IncInputVisitedCount() { ++_input_visited_count; } // both combinational and sequential circuits
	void PrintGateData(); // display data for checking

    protected:
	Cell *_cell_ptr;      // point to cell
	bool _is_non_clocked; // same as _cell_ptr->GetIsNonClocked(), just for performance 

	std::vector<GInPin> _input_vec;
	std::vector<GOutPin> _output_vec;

	unsigned _input_visited_count; // for forward STA
};

//-----------------------------------------------------------------------
//    Define inline member functions
//-----------------------------------------------------------------------

inline Gate::Gate(Cell *c_ptr): _cell_ptr(c_ptr), _is_non_clocked(true), _input_visited_count(0)
{
    _input_vec.assign(c_ptr->GetInputPinNo(), GInPin(this) ); 

    for( unsigned i=0; i<_input_vec.size(); ++i )
    {
	assert( _input_vec[i].GetGatePtr() == this && _input_vec[i].GetFaninPtr() == NULL );
	_input_vec[i].SetPinId(i);
    }

    _output_vec.assign(c_ptr->GetOutputPinNo(), GOutPin(this) ); 

    for( unsigned i=0; i<_output_vec.size(); ++i )
    {
	assert( _output_vec[i].GetGatePtr() == this && _output_vec[i].GetFanoutPtr() == NULL );
	_output_vec[i].SetPinId(i);
    }

    return;
}

#endif // GATE_H
