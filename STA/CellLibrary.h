/************************************************************************
 *   Define a cell library.
 *
 *   Defined class: CellLibrary
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#ifndef CELL_LIBRARY_H
#define CELL_LIBRARY_H

#include <cassert>

#include "Cell.h"

//-----------------------------------------------------------------------
//    Declare classes
//-----------------------------------------------------------------------

class CellLibrary;

//-----------------------------------------------------------------------
//    Define classes
//-----------------------------------------------------------------------

class CellLibrary
{
    public:
	CellLibrary(const char *file_name) { Initialize(file_name); }

	unsigned GetCellNo() const { return _cell_ptr_vec.size(); }
	Cell *GetCellPtr(unsigned id) const { assert( id < _cell_ptr_vec.size() ); return _cell_ptr_vec[id]; }

        void Initialize(const char *file_name);

	void PrintCellLibraryData() const; // display data for checking

    private:
	std::vector<Cell*> _cell_ptr_vec;
};

#endif // CELL_LIBRARY_H
