/************************************************************************
 *   Define member functions of class Cell: PrintCellData()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <cassert>
#include <iostream>

#include "Cell.h"

using namespace std;

// display for checking
void Cell::PrintCellData() const
{
   cout << "cell " << _name << endl;
   assert( _input_pin_name_vec.size() == _input_fall_cap_vec.size() &&
	   _input_pin_name_vec.size() == _input_rise_cap_vec.size() );

   // show input pin data
   for( unsigned i=0; i<_input_pin_name_vec.size(); ++i )
   {
       if( _is_non_clocked )
	   cout << "  pin " << _input_pin_name_vec[i] << " input ";
       else
       {
	   if( i != _clock_pin_id )
	       cout << "  pin " << _input_pin_name_vec[i] << " input ";
	   else
	       cout << "  pin " << _input_pin_name_vec[i] << " clock ";
       }

       cout << _input_fall_cap_vec[i] << " " << _input_rise_cap_vec[i] << endl;
   }

   // show output pin data
   for( unsigned i=0; i<_output_pin_name_vec.size(); ++i )
       cout << "  pin " << _output_pin_name_vec[i] << " output" << endl;

   for( unsigned i=0; i<_input_timing_vec.size(); ++i )
   {
       for( unsigned j=0; j<_input_timing_vec[i].size(); ++j )
       {
	   const InputTimingTable &cur_timing = _input_timing_vec[i][j];

           if( cur_timing.PinTimingSense == InputTimingTable::UNKNOWN_UNATE )
	       continue;

	   cout << "  timing " << _input_pin_name_vec[i] << " " << _output_pin_name_vec[j] << " ";

	   if( cur_timing.PinTimingSense == InputTimingTable::POSITIVE_UNATE )
	       cout << "positive_unate ";
	   else if( cur_timing.PinTimingSense == InputTimingTable::NEGATIVE_UNATE )
	       cout << "negative_unate ";
	   else
	       cout << "non_unate ";

	   cout << cur_timing.FallSlewX << " " << cur_timing.FallSlewY << " " << cur_timing.FallSlewZ << " ";
	   cout << cur_timing.RiseSlewX << " " << cur_timing.RiseSlewY << " " << cur_timing.RiseSlewZ << " ";
	   cout << cur_timing.FallDelayA << " " << cur_timing.FallDelayB << " " << cur_timing.FallDelayC << " ";
	   cout << cur_timing.RiseDelayA << " " << cur_timing.RiseDelayB << " " << cur_timing.RiseDelayC << endl;
       }
   }

   if( !_is_non_clocked )
   {
       for( unsigned i=0; i<_clock_params_vec.size(); ++i )
       {
	   if( _clock_params_vec[i] == NULL )
	       continue;

	   ClockParams &cur_clock_params = *(_clock_params_vec[i]);
	   cout << "  setup " << _input_pin_name_vec[_clock_pin_id] << " " << _input_pin_name_vec[i] << " ";

	   if( cur_clock_params.SetupEdgeType == ClockParams::FALLING )
	       cout << "falling ";
	   else
	       cout << "rising ";

	   cout << cur_clock_params.FallSetupG << " " << cur_clock_params.FallSetupH << " " << cur_clock_params.FallSetupJ << " ";
	   cout << cur_clock_params.RiseSetupG << " " << cur_clock_params.RiseSetupH << " " << cur_clock_params.RiseSetupJ << endl;

           cout << "  hold " << _input_pin_name_vec[_clock_pin_id] << " " << _input_pin_name_vec[i] << " ";

	   if( cur_clock_params.HoldEdgeType == ClockParams::FALLING )
	       cout << "falling ";
	   else
	       cout << "rising ";

	   cout << cur_clock_params.FallHoldM << " " << cur_clock_params.FallHoldN << " " << cur_clock_params.FallHoldP << " ";
	   cout << cur_clock_params.RiseHoldM << " " << cur_clock_params.RiseHoldN << " " << cur_clock_params.RiseHoldP << endl;
       }
   }

   return;
}

