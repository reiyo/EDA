/************************************************************************
 *   Define member functions of class CellLibrary: Initialization(),
 *   PrintCellLibraryData()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "CellLibrary.h"

using namespace std;

// parse cell library file
void CellLibrary::Initialize(const char *file_name)
{
    ifstream inf( file_name, ifstream::in );

    if( !inf )
    {
	cerr << "Error opening " << file_name << " for input" << endl;
	cerr << " Exiting..." << endl;
	exit(-1);
    }

    // variables for parsing
    string cur_str, pin_name, direction;
    double cur_double;
    inf >> cur_str;

    // read cell by cell
    while( !inf.eof() )
    {
	assert( !strcmp(cur_str.c_str(), "cell") );
	Cell *cell_ptr = new Cell();       // create a new cell
	Cell &cur_cell = (*cell_ptr);
	inf >> cur_cell._name >> cur_str;
	assert( !inf.eof() );
	bool is_non_clocked = true;

	while( cur_str[0] == 'p' )
	{
	    assert( cur_str == "pin" );
	    inf >> pin_name >> direction;
	    assert( direction == "input" || direction == "output" || direction == "clock" );

	    switch( direction[0] )
	    {
		case 'i': // input
		    {
			cur_cell._input_pin_name_vec.push_back(pin_name);
			inf >> cur_double;
			cur_cell._input_fall_cap_vec.push_back(cur_double);
			inf >> cur_double;
			cur_cell._input_rise_cap_vec.push_back(cur_double);
			break;
		    }
		case 'o': // output
		    {
			cur_cell._output_pin_name_vec.push_back(pin_name);
			break;
		    }
		case 'c': // clock
		    {
			cur_cell._clock_pin_id = (cur_cell._input_pin_name_vec).size();
			cur_cell._input_pin_name_vec.push_back(pin_name);
			inf >> cur_double;
			cur_cell._input_fall_cap_vec.push_back(cur_double);
			inf >> cur_double;
			cur_cell._input_rise_cap_vec.push_back(cur_double);
			is_non_clocked = false;
			break;
		    }
		default:
		    assert(0);
		    break;
	    }

	    inf >> cur_str;
	}

	// resize a 2D timing vector space
	cur_cell._input_timing_vec.resize(cur_cell._input_pin_name_vec.size(), 
		vector<InputTimingTable>(cur_cell._output_pin_name_vec.size()));

	if( is_non_clocked )
	{
	    cur_cell._is_non_clocked = true;

	    do
	    {
		assert( cur_str == "timing" );
		inf >> cur_str;
		const int input_id = cur_cell.GetInputPinId(cur_str);
		inf >> cur_str;
		const int output_id = cur_cell.GetOutputPinId(cur_str);
		InputTimingTable &cur_timing = (cur_cell._input_timing_vec)[input_id][output_id];
		inf >> cur_str;
		cur_timing.SetPinTimingSense( cur_str );
		inf >> cur_timing.FallSlewX >> cur_timing.FallSlewY >> cur_timing.FallSlewZ;
		inf >> cur_timing.RiseSlewX >> cur_timing.RiseSlewY >> cur_timing.RiseSlewZ;
		inf >> cur_timing.FallDelayA >> cur_timing.FallDelayB >> cur_timing.FallDelayC;
		inf >> cur_timing.RiseDelayA >> cur_timing.RiseDelayB >> cur_timing.RiseDelayC;
	    } while( inf >> cur_str && cur_str[0] == 't' ); // timing
	}
	else
	{
	    cur_cell._is_non_clocked = false;
	    cur_cell._clock_params_vec.assign( cur_cell._input_pin_name_vec.size(), NULL );

	    do
	    {
		assert( cur_str=="timing" || cur_str=="setup" || cur_str=="hold" || cur_str=="preset" || cur_str=="clear" );

		switch( cur_str[0] )
		{
		    case 't': // timing
			{
			    assert( cur_str == "timing" );
			    inf >> cur_str;
			    const int input_id = cur_cell.GetInputPinId(cur_str);
			    inf >> cur_str;
			    const int output_id = cur_cell.GetOutputPinId(cur_str);
			    InputTimingTable &cur_timing = (cur_cell._input_timing_vec)[input_id][output_id];
			    inf >> cur_str;
			    cur_timing.SetPinTimingSense( cur_str );
			    inf >> cur_timing.FallSlewX >> cur_timing.FallSlewY >> cur_timing.FallSlewZ;
			    inf >> cur_timing.RiseSlewX >> cur_timing.RiseSlewY >> cur_timing.RiseSlewZ;
			    inf >> cur_timing.FallDelayA >> cur_timing.FallDelayB >> cur_timing.FallDelayC;
			    inf >> cur_timing.RiseDelayA >> cur_timing.RiseDelayB >> cur_timing.RiseDelayC;
			    break;
			}
		    case 's': // setup
			{
			    inf >> pin_name;
			    assert( pin_name == cur_cell.GetClockPinName() );
			    inf >> pin_name;
			    const int input_id = cur_cell.GetInputPinId(pin_name);

			    if( (cur_cell._clock_params_vec)[input_id] == NULL )
				(cur_cell._clock_params_vec)[input_id] = new ClockParams();

			    // set clock information
			    ClockParams &cur_clock_params = *((cur_cell._clock_params_vec)[input_id]);
			    inf >>  cur_str;
			    assert( cur_str == "falling" || cur_str == "rising" );
			    cur_clock_params.SetupEdgeType = (cur_str[0] == 'f')? ClockParams::FALLING: ClockParams::RISING;
			    inf >> cur_clock_params.FallSetupG >> cur_clock_params.FallSetupH >> cur_clock_params.FallSetupJ;
			    inf >> cur_clock_params.RiseSetupG >> cur_clock_params.RiseSetupH >> cur_clock_params.RiseSetupJ;
			    break;
			}
		    case 'h': // hold
			{
			    inf >> pin_name;
			    assert( pin_name == cur_cell.GetClockPinName() );
			    inf >> pin_name;
			    const int input_id = cur_cell.GetInputPinId(pin_name);

			    if( (cur_cell._clock_params_vec)[input_id] == NULL )
				(cur_cell._clock_params_vec)[input_id] = new ClockParams();

			    // set clock information
			    ClockParams &cur_clock_params = *((cur_cell._clock_params_vec)[input_id]);
			    inf >> cur_str;
			    assert( cur_str == "falling" || cur_str == "rising" );
			    cur_clock_params.HoldEdgeType = (cur_str[0] == 'f')? ClockParams::FALLING: ClockParams::RISING;
			    inf >> cur_clock_params.FallHoldM >> cur_clock_params.FallHoldN >> cur_clock_params.FallHoldP;
			    inf >> cur_clock_params.RiseHoldM >> cur_clock_params.RiseHoldN >> cur_clock_params.RiseHoldP;
			    break;
			}
		    case 'p': // preset <input pin name> <output pin name> <edge type> <slew> <delay>
			{
			    inf >> cur_str >> cur_str >>cur_str >>cur_str >>cur_str >>cur_str >>cur_str >>cur_str >> cur_str;
			    break;
			}
		    case 'c':  // clear <input pin name> <output pin name> <edge type> <slew> <delay>
			{
			    inf >> cur_str >> cur_str >>cur_str >>cur_str >>cur_str >>cur_str >>cur_str >>cur_str >> cur_str;
			    break;
			}
		    default:
			assert(0);
			break;
		}
	    } while( inf >> cur_str && cur_str != "cell" );
	} // end else

	_cell_ptr_vec.push_back(cell_ptr); // insert new cell to cell library
    } // end while( !inf.eof() )

    inf.close();

    return;
}

// display for checking
void CellLibrary::PrintCellLibraryData() const
{
    cout << "#.. Print current cell library" << endl;

    for( unsigned i=0; i<_cell_ptr_vec.size(); ++i )
	(*(_cell_ptr_vec[i])).PrintCellData();

    cout << "#.. Done library printing." << endl;

    return;
}
