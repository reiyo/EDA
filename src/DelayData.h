/************************************************************************
 *   Define the single input pin to single output pin gate delay. 
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef DELAY_DATA_H
#define DELAY_DATA_H

class DelayData
{
    public:
	enum Type
	{
	    UNATE,
	    NON_UNATE,
	};

    virtual Type GetType() = 0;
};

class UnateDelayData: public DelayData
{
    public:
        Type GetType() { return UNATE; }

	double DelayFromInputFall;
	double DelayFromInputRise;
};

class NonUnateDelayData: public DelayData
{
    public:
	Type GetType() { return NON_UNATE; }

	double InputFallOutputFallDelay; // positive unate
	double InputFallOutputRiseDelay; // negative unate
	double InputRiseOutputFallDelay;
	double InputRiseOutputRiseDelay;
};

#endif // DELAY_DATA_H
