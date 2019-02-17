/************************************************************************
 *   Define the most basic parent data structure for all the classes.
 *   (parent of classes in Gate.h and PinNode.h)
 *
 *   Defined classes: Element
 *
 *   Class inheritance hierarchy:
 *   (1) Element -> Gate -> SeqGate (defined in Gate.h)
 *   (2) Element -> GInPin, GOutPin (defined in Gate.h)
 *   (3) Element -> PinNode (defined in PinNode.h)
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef ELEMENT_H
#define ELEMENT_H

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class Element;

//-----------------------------------------------------------------------
//    Define classes
//-----------------------------------------------------------------------

class Element
{
    public:
	enum Type 
	{ 
	    GATE,      // combinational or sequential gate
	    GIN_PIN,   // gate input pin element
	    GOUT_PIN,  // gate output pin element
	    PIN_NODE   // pin node, either port, or tap
	};

	virtual Type GetType() = 0;
};

#endif // ELEMENT_H
