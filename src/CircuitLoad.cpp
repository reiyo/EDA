/************************************************************************
 *   Define member functions of class Circuit: Initialization()
 *
 *   Author      : Kuan-Hsien Ho
************************************************************************/

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include "Cell.h"
#include "CellLibrary.h"
#include "Circuit.h"
#include "parameterDefine.h"
#include "PinNode.h"

using namespace std;

//-----------------------------------------------------------------------
//    Declare auxiliary functions 
//-----------------------------------------------------------------------

bool cmpCellPtrNameSort( Cell *a_ptr, Cell *b_ptr );

void genSortedCellPtrVec( const CellLibrary &cell_library, vector<Cell*> &cell_ptr_vec );

bool isReservedWord( char *reserved_word ); // debug-only

//-----------------------------------------------------------------------
//    Define member functions
//-----------------------------------------------------------------------

inline void Circuit::LoadPrimaryInput( FILE *inf_ptr, const char *node_name, map<string, PinNode*> &pin_node_map )
{
    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name);
    PinNode *pin_node_ptr;

    if( mapIter == pin_node_map.end() )
    {
	pin_node_ptr = new PinNode(node_name);
	_PinNode_ptr_vec.push_back(pin_node_ptr);
	pin_node_map[node_name] = pin_node_ptr; // insert to map
    }
    else
	pin_node_ptr = (*mapIter).second;

    Gate *gate_ptr = new Gate(); // new a gate as primary input
    (*gate_ptr)._output_vec.push_back( GOutPin(0, gate_ptr, pin_node_ptr) );
    assert( (*gate_ptr)._output_vec.size() == 1 );
    pin_node_ptr->_fanin_ptr = &(((*gate_ptr)._output_vec)[0]);
    _PI_ptr_vec.push_back(gate_ptr); // insert to PI vector

    return;
}

inline void Circuit::LoadPrimaryOutput( FILE *inf_ptr, const char *node_name, map<string, PinNode*> &pin_node_map )
{
    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name);
    PinNode *pin_node_ptr;

    if( mapIter == pin_node_map.end() )
    {
        pin_node_ptr = new PinNode(node_name);
	_PinNode_ptr_vec.push_back(pin_node_ptr);
	pin_node_map[node_name] = pin_node_ptr; // insert to map
    }
    else
	pin_node_ptr = (*mapIter).second;

    Gate *gate_ptr = new Gate(); // new a gate as primary output
    (*gate_ptr)._input_vec.push_back( GInPin(0, gate_ptr, pin_node_ptr) );
    assert( (*gate_ptr)._input_vec.size() == 1 );
    pin_node_ptr->_fanout_ptr_vec.push_back( &(((*gate_ptr)._input_vec)[0]) );
    _PO_ptr_vec.push_back(gate_ptr); // insert to PO vector

    return;
}

inline void Circuit::LoadInstance( FILE *inf_ptr, map<string, PinNode*> &pin_node_map, char *pos_ptr, Cell *cur_cell_ptr )
{
    char *token_ptr = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)); 
    Gate *gate_ptr = new Gate(cur_cell_ptr);

    while( token_ptr != NULL )
    {
	string cur_str = token_ptr;
	const size_t pos = cur_str.find(':');
	const string cur_name = cur_str.substr(0, pos); // pin name in cur_name
	cur_str = cur_str.substr(pos+1);                // node name in cur_str
	map<string, PinNode*>::iterator mapIter = pin_node_map.find(cur_str);
	PinNode *pin_node_ptr = NULL;

	if( mapIter == pin_node_map.end() )
	{
	    pin_node_ptr = new PinNode(cur_str);
	    _PinNode_ptr_vec.push_back(pin_node_ptr);
	    pin_node_map[cur_str] = pin_node_ptr;
	}
	else
	    pin_node_ptr = (*mapIter).second;

	int pin_id = cur_cell_ptr->GetInputPinId(cur_name);
	assert( pin_id >= 0 || pin_id == -1 );

	if( pin_id >= 0 ) // input pin
	{
	    (gate_ptr->_input_vec)[pin_id].SetFaninPtr(pin_node_ptr);
	    pin_node_ptr->_fanout_ptr_vec.push_back( &((gate_ptr->_input_vec)[pin_id]) ); // a pin node may drive multiples 
	}
	else // output pin
	{ 
	    pin_id = cur_cell_ptr->GetOutputPinId(cur_name);
	    assert( pin_id >= 0 );
	    (gate_ptr->_output_vec)[pin_id].SetFanoutPtr(pin_node_ptr);
	    assert( pin_node_ptr->_fanin_ptr == NULL );
	    pin_node_ptr->_fanin_ptr = &((gate_ptr->_output_vec)[pin_id]);
	}

	token_ptr = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
    } // end while

    if( cur_cell_ptr->GetIsNonClocked() ) // combinational
    {
	assert( gate_ptr->_is_non_clocked == true );
	_ComGate_ptr_vec.push_back(gate_ptr);
    }
    else // sequential
    {
	gate_ptr->_is_non_clocked = false;
	_SeqGate_ptr_vec.push_back(gate_ptr);
    }

    return;
}

inline bool Circuit::LoadWire( FILE *inf_ptr, map<string, PinNode*> &pin_node_map, char **posPtr_ptr, char **reservedWord_ptr )
{
    char *pos_ptr = *(posPtr_ptr);
    char *token_ptr = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)); // get root name

    map<string, PinNode*>::iterator mapIter = pin_node_map.find(token_ptr);
    PinNode* root_ptr;

    if( mapIter != pin_node_map.end() )
    {
	root_ptr = (*mapIter).second;
	assert( ((*mapIter).second)->_fanin_ptr != NULL && (((*mapIter).second)->_fanout_ptr_vec).size() == 0 );
    }
    else
    {
	root_ptr = new PinNode(token_ptr);
	_PinNode_ptr_vec.push_back(root_ptr);
	pin_node_map[token_ptr] = root_ptr;
    }

    vector<RCTreeNode> &rc_tree = root_ptr->_fanout_rc_tree;
    assert( rc_tree.size() == 0 );
    rc_tree.push_back( RCTreeNode(root_ptr->_name, root_ptr) );
    token_ptr = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));

    while( token_ptr != NULL ) // get tap nodes
    {
	mapIter = pin_node_map.find(token_ptr);

	if( mapIter != pin_node_map.end() )
	{
	    PinNode *pin_node_ptr = (*mapIter).second;
	    assert( pin_node_ptr->_fanin_ptr == NULL && (pin_node_ptr->_fanout_ptr_vec).size() == 1 );
	    pin_node_ptr->_fanin_ptr = root_ptr;
	    (root_ptr->_fanout_ptr_vec).push_back(pin_node_ptr);
	    rc_tree.push_back( RCTreeNode( pin_node_ptr->_name, pin_node_ptr ) );
	}
	else
	{
	    PinNode *pin_node_ptr = new PinNode(token_ptr);
	    _PinNode_ptr_vec.push_back(pin_node_ptr);
	    pin_node_map[token_ptr] = pin_node_ptr;
	    pin_node_ptr->_fanin_ptr = root_ptr;
	    (root_ptr->_fanout_ptr_vec).push_back(pin_node_ptr);
	    rc_tree.push_back( RCTreeNode( token_ptr, pin_node_ptr ) );
	}

	token_ptr = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
    }

    (root_ptr->_fanout_pin_node_no) = rc_tree.size()-1; // record number of leaf pin nodes for very special case
    assert( (1 + (root_ptr->_fanout_ptr_vec).size()) == rc_tree.size() ); 
    char line[LINE_LEN];

    do
    {
	unsigned length = 1;

	do
	{
	    if( fgets(line, LINE_LEN, inf_ptr) != NULL ) // get next line
		length = strlen(line);
	    else
		return true; // end of file
	} while( length == 1 );

	pos_ptr = NULL;
	line[length-1] = '\0';
	char *reserved_word = static_cast<char *>(strtok_r(line, " ", &pos_ptr));

	if( *(reserved_word+1) == 'e' )
	{
	    unsigned end1_id = root_ptr->GrabFanoutRCTreeNodeId(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
	    unsigned end2_id = root_ptr->GrabFanoutRCTreeNodeId(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
	    double resistance = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
	    // double link, but we will decompose them to fanin, fanout sets after injecting wiring effects later
	    rc_tree[end1_id]._fanout_id_res_list.push_back( pair<unsigned, double>(end2_id, resistance) );
	    rc_tree[end2_id]._fanout_id_res_list.push_back( pair<unsigned, double>(end1_id, resistance) );
	}
	else if( *(reserved_word) == 'c' && *(reserved_word+1) == 'a' )
	{
	    string end_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
	    double capacitance = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
	    const unsigned end_id = root_ptr->GrabFanoutRCTreeNodeId( end_name );
	    (rc_tree[end_id]._cap) = capacitance;
	}
	else
	{
	    *(posPtr_ptr) = pos_ptr;
	    *(reservedWord_ptr) = reserved_word;
	    return false;
	}
    } while(true);

    return false;
}

void Circuit::LoadRATData( map<string, PinNode*> &pin_node_map, char *pos_ptr )
{
    char *node_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name);
    PinNode* pnode_ptr;

    if( mapIter != pin_node_map.end() )
	pnode_ptr = (*mapIter).second;
    else
    {
	pnode_ptr = new PinNode(node_name);
	_PinNode_ptr_vec.push_back(pnode_ptr);
	pin_node_map[node_name] = pnode_ptr;
    }

    char *mode = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
    assert( !strcmp(mode, "early") || !strcmp(mode, "late") );
    double required_fall_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
    double required_rise_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));

    if( (*mode) == 'e' )
    {
	vector<RATData>::iterator vecIter=_RATData_vec.begin();

        for( ; vecIter!=_RATData_vec.end(); ++vecIter )
	    if( pnode_ptr == (*vecIter).PinNodePtr ) // same address, i.e., same pin node
	    {
		assert( (*vecIter).Mode == RATData::SLOW );
		(*vecIter).Mode = RATData::BOTH;
		(*vecIter).FastFallTime = required_fall_time;
		(*vecIter).FastRiseTime = required_rise_time;
		break;
	    }

	if( vecIter == _RATData_vec.end() )
	    _RATData_vec.push_back( RATData(pnode_ptr, RATData::FAST, 0.0, 0.0, required_fall_time, required_rise_time) );
    }
    else
    {
	vector<RATData>::iterator vecIter=_RATData_vec.begin();

        for( ; vecIter!=_RATData_vec.end(); ++vecIter )
	    if( pnode_ptr == (*vecIter).PinNodePtr ) // same address, i.e., same pin node
	    {
		assert( (*vecIter).Mode == RATData::FAST );
		(*vecIter).Mode = RATData::BOTH;
		(*vecIter).SlowFallTime = required_fall_time;
		(*vecIter).SlowRiseTime = required_rise_time;
		break;
	    }

	if( vecIter == _RATData_vec.end() )
	    _RATData_vec.push_back( RATData(pnode_ptr, RATData::SLOW, required_fall_time, required_rise_time, 0.0, 0.0) );
    }

    return;
}

// parse inputs/outputs, instances, wires, at/rat in order
// a circuit must contain at least an instance, a wire, an at setting; list of <pin name>:<node> must be in a line
void Circuit::Initialize( const char *file_name, const CellLibrary &cell_library )
{
    FILE *inf_ptr = fopen( file_name, "r" );  

    if( inf_ptr == NULL )
    {
	printf( "Error in opening %s for input\n", file_name );
	printf( "  Exiting...\n" );
	exit(-1);
    }

    std::map<std::string, PinNode*> pin_node_map; // map pin node name and its pointer
    vector<Cell*> cell_ptr_vec;                   // to efficiently link instances and cell pointers
    genSortedCellPtrVec( cell_library, cell_ptr_vec );
    Cell *cur_cell_ptr = new Cell();              // variables for instance parsing
    char line[LINE_LEN];

    {
	unsigned length = 1;

	do
	{
	    if( fgets(line, LINE_LEN, inf_ptr) != NULL ) // get next line
		length = strlen(line);
	    else
	    {
		fclose(inf_ptr);
		return;
	    }
	} while( length == 1 );

	line[length-1] = '\0';
    }

    bool unEOF_flag = true;
    char *pos_ptr = NULL;
    char *reserved_word = static_cast<char *>(strtok_r(line, " ", &pos_ptr));

    while( unEOF_flag )
    {
	//printf( "%s\n", reserved_word );
	assert( isReservedWord(reserved_word) );

	switch( *(reserved_word) )
	{
	    case 'i':
		switch( *(reserved_word+2) )
		{
		    case 'p': // input
			    LoadPrimaryInput( inf_ptr, static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)), pin_node_map );
			    break;
		    case 's': // instance
			{
			    const char *inst_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
			    (*cur_cell_ptr).SetName(inst_name);
			    vector<Cell*>::iterator cellPtrVecIter = lower_bound( cell_ptr_vec.begin(), cell_ptr_vec.end(), 
				                                                  cur_cell_ptr, cmpCellPtrNameSort );
			    assert( cellPtrVecIter != cell_ptr_vec.end() );
			    assert( (**cellPtrVecIter).GetName() == (*cur_cell_ptr).GetName() );
			    LoadInstance( inf_ptr, pin_node_map, pos_ptr, (*cellPtrVecIter) );

			    break;
		        }
		    default:
			assert( 0 );
			break;
		}

		break;
	    case 'o': // output
		LoadPrimaryOutput( inf_ptr, static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)), pin_node_map );
		break;
	    case 'w':
		if( LoadWire( inf_ptr, pin_node_map, &pos_ptr, &reserved_word ) )
		    unEOF_flag = false; // end of file

		continue;
	    case 's':
		{
		    string node_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
		    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name); // node name
		    PinNode *pin_node_ptr;

                    if( mapIter != pin_node_map.end() )
			pin_node_ptr = (*mapIter).second;
		    else
		    {
			pin_node_ptr = new PinNode(node_name);
			_PinNode_ptr_vec.push_back(pin_node_ptr);
			pin_node_map[node_name] = pin_node_ptr;
		    }

		    pin_node_ptr->_fast_fall_slew = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
		    pin_node_ptr->_fast_rise_slew = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
		    pin_node_ptr->_slow_fall_slew = pin_node_ptr->_fast_fall_slew;
		    pin_node_ptr->_slow_rise_slew = pin_node_ptr->_fast_rise_slew;

		    break;
		}
	    case 'a':
		{
		    string node_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
		    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name); // node name
		    PinNode *pin_node_ptr;

                    if( mapIter != pin_node_map.end() )
			pin_node_ptr = (*mapIter).second;
		    else
		    {
			pin_node_ptr = new PinNode(node_name);
			_PinNode_ptr_vec.push_back(pin_node_ptr);
			pin_node_map[node_name] = pin_node_ptr;
		    }

		    pin_node_ptr->_fast_fall_arr_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
                    pin_node_ptr->_slow_fall_arr_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
		    pin_node_ptr->_fast_rise_arr_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
		    pin_node_ptr->_slow_rise_arr_time = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));

		    break;
		}
	    case 'r':
		LoadRATData(pin_node_map, pos_ptr);
		break;
	    case 'c':
		{
		    string node_name = static_cast<char *>(strtok_r(NULL, " ", &pos_ptr));
		    map<string, PinNode*>::iterator mapIter = pin_node_map.find(node_name); // node name

                    if( mapIter != pin_node_map.end() )
			_clock_ptr = (*mapIter).second;
		    else
		    {
			_clock_ptr = new PinNode(node_name);
			_PinNode_ptr_vec.push_back(_clock_ptr);
			pin_node_map[node_name] = _clock_ptr;
		    }

		    _clock_period = atof(static_cast<char *>(strtok_r(NULL, " ", &pos_ptr)));
		    break;
		}
	    default:
		assert(0);
		break;
        }

	unsigned length = 1;

	do
	{
	    if( fgets(line, LINE_LEN, inf_ptr) != NULL ) // get next line
		length = strlen(line);
	    else
		unEOF_flag = false;
	} while( length == 1 && unEOF_flag );

	if( unEOF_flag )
	{
	    line[length-1] = '\0';
	    pos_ptr = NULL;
	    reserved_word = static_cast<char *>(strtok_r(line, " ", &pos_ptr));
	}
    }

    // set sequential circuit or not
    _is_sequential = ( _SeqGate_ptr_vec.size() > 0 )? true: false;

    delete cur_cell_ptr;
    fclose(inf_ptr);

    return;
}

//-----------------------------------------------------------------------
//    Define auxiliary functions 
//-----------------------------------------------------------------------

bool cmpCellPtrNameSort( Cell *a_ptr, Cell *b_ptr ) 
{ 
    return ( (*a_ptr).GetName() < (*b_ptr).GetName() );
}

void genSortedCellPtrVec( const CellLibrary &cell_library, vector<Cell*> &cell_ptr_vec )
{
    cell_ptr_vec.resize( cell_library.GetCellNo() );

    for( unsigned i=0; i<cell_ptr_vec.size(); ++i )
	cell_ptr_vec[i] = cell_library.GetCellPtr(i);

    sort( cell_ptr_vec.begin(), cell_ptr_vec.end(), cmpCellPtrNameSort );

    return;
}

bool isReservedWord( char *reserved_word )
{
    if( !strcmp(reserved_word, "input") || !strcmp(reserved_word, "instance") || !strcmp(reserved_word, "output") ||
	!strcmp(reserved_word, "wire") || !strcmp(reserved_word, "slew") || !strcmp(reserved_word, "at") ||
	!strcmp(reserved_word, "rat") || !strcmp(reserved_word, "clock") )
	return true;
    else
	return false;
}
