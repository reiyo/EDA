/************************************************************************
 *    Define data structures for cell library.
 *    (Assume a sequential gate contains just a clock pin)
 *
 *    Defined classes: Cell, ClockParams, InputTimingTable
 *
 *    Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef CELL_H
#define CELL_H

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class Cell;             // components of cell library, ex. AND2X1
class ClockParams;      // clock setup and hold parameters
class InputTimingTable; // slew, delay parameters of an input of a gate 

//-----------------------------------------------------------------------
//    Define classes
//-----------------------------------------------------------------------

class Cell
{
    public:
	friend class CellLibrary;

	std::string GetName() const { return _name; }
	bool GetIsNonClocked() const { return _is_non_clocked; }
	std::string GetInputPinName(const unsigned &id) const { return _input_pin_name_vec[id]; }
	std::string GetClockPinName() const { return _input_pin_name_vec[_clock_pin_id]; }
	double GetInputFallCap(const unsigned &id) const { return _input_fall_cap_vec[id]; }
	double GetInputRiseCap(const unsigned &id) const { return _input_rise_cap_vec[id]; }
	std::string GetOutputPinName(const unsigned &id) const { return _output_pin_name_vec[id]; }
	int GetInputPinId(const std::string &name) const;
	unsigned GetClockPinId() const { return _clock_pin_id; }
	int GetOutputPinId(const std::string &name) const;
	unsigned GetInputPinNo() const { return _input_pin_name_vec.size(); }
	unsigned GetOutputPinNo() const { return _output_pin_name_vec.size(); }
	const std::vector<std::vector<InputTimingTable> >& FetInputTimingVec() const { return _input_timing_vec; }
	const std::vector<ClockParams*>& FetClockParamsVec() const { return _clock_params_vec; }

	void SetName(const char *n) { _name = n; }

	void PrintCellData() const; // display data for checking

    protected:
	std::string _name;    // cell name
	bool _is_non_clocked; // true if it is combinational, false if it is sequential 

	// input pin data, including clock pin
	std::vector<std::string> _input_pin_name_vec; 
	std::vector<double> _input_fall_cap_vec; // falling input pin capacitance
	std::vector<double> _input_rise_cap_vec;
	std::vector<std::vector<InputTimingTable> > _input_timing_vec; // _input_timing_vec[input num][output num]

	// clock pin data
	unsigned _clock_pin_id; // ex. 5 if clock pin name is stored in _input_pin_name_vec[5]
	std::vector<ClockParams*> _clock_params_vec; // clock to input pin with id corresponding to _input_pin_name_vec

	// output pin data
	std::vector<std::string> _output_pin_name_vec;
};

class ClockParams
{
    public:
	enum EdgeType 
	{ 
	    FALLING, 
	    RISING 
	};

	EdgeType SetupEdgeType;
	EdgeType HoldEdgeType;

	double FallSetupG;
	double FallSetupH;
	double FallSetupJ;
	double FallHoldM;
	double FallHoldN;
	double FallHoldP;

	double RiseSetupG;
	double RiseSetupH;
	double RiseSetupJ;
	double RiseHoldM;
	double RiseHoldN;
	double RiseHoldP;
};

class InputTimingTable
{
    public:
	enum TimingSense 
	{ 
	    POSITIVE_UNATE, 
	    NEGATIVE_UNATE, 
	    NON_UNATE, 
	    UNKNOWN_UNATE 
	};

	InputTimingTable(): PinTimingSense(UNKNOWN_UNATE) {} // constructor

	void SetPinTimingSense(std::string &str);

	TimingSense PinTimingSense;

	double FallSlewX;
	double FallSlewY;
	double FallSlewZ;
	double FallDelayA;
	double FallDelayB;
	double FallDelayC;

	double RiseSlewX;
	double RiseSlewY;
	double RiseSlewZ;
	double RiseDelayA;
	double RiseDelayB;
	double RiseDelayC;
};

//-----------------------------------------------------------------------
//    Define inline member functions
//-----------------------------------------------------------------------

inline int Cell::GetInputPinId(const std::string &name) const
{
    for( unsigned i=0; i<_input_pin_name_vec.size(); ++i )
	if( _input_pin_name_vec[i] == name )
	    return static_cast<int>(i);

    return -1;
}

inline int Cell::GetOutputPinId(const std::string &name) const
{
    for( unsigned i=0; i<_output_pin_name_vec.size(); ++i )
	if( _output_pin_name_vec[i] == name )
	    return static_cast<int>(i);

    return -1;
}

inline void InputTimingTable::SetPinTimingSense(std::string &str)
{
    assert( str == "positive_unate" || str == "negative_unate" || str == "non_unate" );

    if( str[0] == 'p' )
        PinTimingSense = POSITIVE_UNATE;
    else if( str[0] == 'n' )
    {
	if( str[1] == 'e' )
	    PinTimingSense = NEGATIVE_UNATE;
	else
            PinTimingSense = NON_UNATE;
    }

    return;
}

#endif // CELL_H
