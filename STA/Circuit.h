/************************************************************************
 *   Define a complete circuit structure.
 *
 *   Defined classes: Circuit
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <cassert>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "Cell.h"
#include "CellLibrary.h"
#include "Gate.h"
#include "PinNode.h"
#include "RATData.h"

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class Circuit;

//-----------------------------------------------------------------------
//    Define classes
//-----------------------------------------------------------------------

class Circuit
{
    public:
	Circuit(const char *file_name, const CellLibrary &cell_library): _is_sequential(false), _clock_ptr(NULL), _clock_period(0.0) { Initialize(file_name, cell_library); }

	bool GetIsSequential() const { assert( _is_sequential || _SeqGate_ptr_vec.size() == 0 ); return _is_sequential; }
	double GetClockPeriod() const                 { return _clock_period; }
	PinNode* GetPinNodePtr(unsigned id)           { return _PinNode_ptr_vec[id]; }
	Gate* GetPIPtr(const unsigned &id) const      { return _PI_ptr_vec[id]; }
	Gate* GetComGatePtr(const unsigned &id) const { return _ComGate_ptr_vec[id]; }
	Gate* GetSeqGatePtr(const unsigned &id) const { return _SeqGate_ptr_vec[id]; }

	unsigned GetPINo()      const { return _PI_ptr_vec.size(); }
	unsigned GetPONo()      const { return _PO_ptr_vec.size(); }
	unsigned GetComGateNo() const { return _ComGate_ptr_vec.size(); }
	unsigned GetSeqGateNo() const { return _SeqGate_ptr_vec.size(); }
	unsigned GetPinNodeNo() const { return _PinNode_ptr_vec.size(); }
	unsigned GetTotGateNo() const { return (GetPINo() + GetPONo() + GetComGateNo() + GetSeqGateNo()); }
	unsigned GetRATDataNo() const { return _RATData_vec.size(); }
 
	void SetRATResultsList( std::list<RATData> &rat_list ); // defined in CircuitRAT.cpp

	std::vector<Gate*> &FetPOPtrVec()                  { return _PO_ptr_vec; }
	std::vector<Gate*> &FetComGatePtrVec()             { return _ComGate_ptr_vec; }
	const std::vector<Gate*> &FetSeqGatePtrVec() const { return _SeqGate_ptr_vec; }
	std::vector<PinNode*> &FetPinNodePtrVec()          { return _PinNode_ptr_vec; }
	std::vector<RATData> &FetRATDataVec()              { return _RATData_vec; }

	void Initialize(const char *file_name, const CellLibrary &cell_library);
	void PrintCircuitData() const;
	void PrintTimingData(); // print on screen
	void PrintTimingData(const char *file_name); // print to file

    private:
	void LoadPrimaryInput( FILE *inf_ptr, const char *node_name, std::map<std::string, PinNode*> &pin_node_map );
	void LoadPrimaryOutput( FILE *inf_ptr, const char *node_name, std::map<std::string, PinNode*> &pin_node_map );
	void LoadInstance( FILE *inf_ptr, std::map<std::string, PinNode*> &pin_node_map, char *pos_ptr, Cell *cur_cell_ptr );
	bool LoadWire( FILE *inf_ptr, std::map<std::string, PinNode*> &pin_node_map, char **pos_ptr, char **reserved_word );
	void LoadRATData( std::map<std::string, PinNode*> &pin_node_map, char *pos_ptr );

	bool _is_sequential; // true if it is a sequential circuit; false otherwise
	PinNode *_clock_ptr;
	double _clock_period;

	// circuit components
	std::vector<Gate*> _PI_ptr_vec;         // primary input gates
	std::vector<Gate*> _PO_ptr_vec;         // primary output gates
	std::vector<Gate*> _ComGate_ptr_vec;    // combinational gates, including no PI or PO gates
	std::vector<Gate*> _SeqGate_ptr_vec;    // D flip-flop gates
        std::vector<PinNode*> _PinNode_ptr_vec; // all pin nodes, Non-sorted 

	std::vector<RATData> _RATData_vec;      // required time constraints  
};

#endif // CIRCUIT_H
