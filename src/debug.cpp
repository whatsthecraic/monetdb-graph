/*
 * debug.cpp
 *
 *  Created on: 29 Sep 2016
 *      Author: Dean De Leo
 */
#include <iostream>

#undef ATOMIC_FLAG_INIT // make MonetDB happy
// MonetDB include files
extern "C" {
#include "monetdb_config.h"
#include "mal_exception.h"
}
#undef throw // this is a keyword in C++

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
    cout << "hseqbase: " << b->hseqbase << ", count: " << A_sz << "\n";
    for(size_t i = 0; i < A_sz; i++){
        cout << "[" << i << "] " << A[i] << '\n';
    }
}

template<>
void _bat_debug_T<void>(BAT* b){
    using std::cout;
    cout << "hseqbase: " << b->hseqbase << ", count: " << BATcount(b) << ", TYPE_void, start of the sequence: " << b->T.seq << "\n";
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

