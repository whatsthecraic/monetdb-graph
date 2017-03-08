/*
 * debug.cpp
 *
 *  Created on: 29 Sep 2016
 *      Author: Dean De Leo
 */
#include "debug.h"

using namespace std;

/******************************************************************************
 *                                                                            *
 *  Print the content of a BAT to stdout                                      *
 *                                                                            *
 ******************************************************************************/

template<typename T>
static void _bat_debug_T(BAT* b){
    using std::cout;

    T* A = reinterpret_cast<T*>(b->theap.base);
    size_t A_sz = (size_t) BATcount(b);
    // BATatoms is defined in monetdb/gdk/gdk_atoms.c
    atomDesc* type = BATatoms + (b->ttype);

    cout << "id: " << b->batCacheid << ", hseqbase: " << b->hseqbase << ", count: " << A_sz << ", type: " << type->name << " (" << (int) b->ttype << ")\n";
    for(size_t i = 0; i < A_sz; i++){
        cout << "[" << i << "] " << A[i] << '\n';
    }
}

template<>
void _bat_debug_T<void>(BAT* b){
    using std::cout;
    cout << "id: " << b->batCacheid << ", hseqbase: " << b->hseqbase << ", count: " << BATcount(b) << ", TYPE_void, start of the sequence: " << b->T.seq << "\n";
}


extern "C" { // disable name mangling

void _bat_debug0(const char* prefix, BAT* b){
    using std::cout;

    cout << prefix << " ";
    if(b == nullptr){
        std::cout << "NULL\n";
    } else {
        switch(b->ttype){
        case TYPE_void:
            _bat_debug_T<void>(b);
            break;
        case TYPE_int:
        	_bat_debug_T<int>(b);
        	break;
        case TYPE_lng:
        	_bat_debug_T<lng>(b);
			break;
        case TYPE_oid:
            _bat_debug_T<oid>(b);
            break;
        default:
            cout << "Type not handled (" << b->ttype << ")\n";
        }
    }

    std::flush(cout);
}

} /* extern "C" */


// __debug_dump0 specializations
template <>
void _debug_dump0<BAT*>(const char* prefix, BAT* value){
	_bat_debug0(prefix, value);
}
template <>
void _debug_dump0<bool>(const char* prefix, bool value){
	std::cout << prefix << ": " << std::boolalpha << value << std::endl;
}
template <>
void _debug_dump0<char*>(const char* prefix, char* value){
	std::cout << value << std::endl;
}
template<>
void _debug_dump0<gr8::BatHandle>(const char* prefix, gr8::BatHandle handle){
	if(!handle.initialised()){
		std::cout << prefix << ": <<handle not initialized>>" << std::endl;
	} else if (handle.empty()){
		std::cout << prefix << ": <<empty>>" << std::endl;
	} else {
		_debug_dump0<BAT*>(prefix, handle.get());
	}
}
