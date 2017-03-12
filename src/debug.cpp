/*
 * debug.cpp
 *
 *  Created on: 29 Sep 2016
 *      Author: Dean De Leo
 */
#include "debug.h"
#include "configuration.hpp"

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

static void _bat_debug_nested_table(BAT* b){
	var_t* A = reinterpret_cast<var_t*>(b->theap.base);
	const size_t A_sz = BATcount(b);

	cout << "id: " << b->batCacheid << ", hseqbase: " << b->hseqbase << ", count: " << A_sz << ", TYPE_nested_table\n";
	if(b->T.vheap == nullptr){
		cout << "--> vheap not allocated!\n";
		return;
	}
	char* H = b->T.vheap->base;
	for(size_t i = 0; i < A_sz; i++){
		var_t pos = A[i] << GDK_VARSHIFT;
		oid* entry = reinterpret_cast<oid*>(H + pos);
		size_t entry_count = *entry;
		cout << "[" << i << "] " << entry_count << " values: ";
		entry++;
		for(size_t j = 0; j < entry_count; j++){
			if(j>0){ cout << ", "; }
			cout << entry[j];
		}
		cout << '\n';
	}


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
        	// nested table ?
        	if(gr8::configuration().initialised()){
        		int nt_type = gr8::configuration().type_nested_table();
        		if(b->ttype == nt_type){
        			_bat_debug_nested_table(b);
        			return;
        		}
        	}

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
