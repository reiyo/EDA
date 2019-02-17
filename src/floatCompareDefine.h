/************************************************************************
 *   Define macros for float comparison to handle possible floating errors.
 *   (The content of this file is obtained from Xin-Wei)
 *
 *   Author      : Kuan-Hsien Ho (source was from X.-W. Shih)
************************************************************************/

#ifndef FLOAT_COMPARE_DEFINE_H
#define FLOAT_COMPARE_DEFINE_H

#include <cmath>

    //double (handle floating error) macro
    #define _epsilon 0.0000001

    //double equal
    #define DE(a,b) (std::abs((double)(a-b))<_epsilon)

    //double greater than
    #define DG(a,b) ((a>b)&&(!DE(a,b)))

    //double less than
    #define DL(a,b) ((a<b)&&(!DE(a,b)))

    //double greater than or equal
    #define DGE(a,b) ((a>b)||(DE(a,b)))

    //double less than or equal
    #define DLE(a,b) ((a<b)||(DE(a,b)))

#endif // FLOAT_COMPARE_DEFINE_H
