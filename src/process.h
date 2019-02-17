/************************************************************************
 *   Define main processes of this program, i.e., static timing analysis.
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef PROCESS_H
#define PROCESS_H

#include "Circuit.h"
#include "Gate.h"
#include "PinNode.h"

void backtraceSignal( Gate &cur_gate ); // backtraceFastSignal() + backtraceSlowSignal()

void backtraceFastSignal( Gate &cur_gate );

void backtraceSlowSignal( Gate &cur_gate );

void injectWiringEffects( Circuit &circuit ); // inject all pin-node loads and Elmore delays 

void propagateSignal( Gate *gate_ptr ); // propagateFastSignal() + propagateSlowSignal()

void propagateSignal( const unsigned &input_pin_id, Gate *gate_ptr ); // clock to Q and QN

void resistDefectPinNodes( Circuit &circuit );

void runComBackwardSTA( Circuit &circuit );

void runComForwardSTA( Circuit &circuit );

void runSTA( Circuit &circuit );

void runSeqBackwardSTA( Circuit &circuit );

void runSeqForwardSTA( Circuit &circuit );

#endif // PROCESS_H
