/*
 * debug.h
 *
 *  Created on: 29 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "monetdb_config.hpp"
#ifdef __cplusplus
extern "C" {
#endif

// Print the whole content of a BAT to stdout (similar to io.print);
void _bat_debug0(const char* prefix, BAT* b);
#define bat_debug(b) _bat_debug0("["#b"]",  b);


void _debug_bat_state0(const char* prefix, bat* id);
#define dump_bat_state(bid) _debug_bat_state0("["#bid"]",  bid);
void _debug_BAT_state0(const char* prefix, BAT* b);
#define dump_BAT_state(b) _debug_BAT_state0("["#b"]",  b);

void dump_bat_arguments(MalBlkPtr mb, MalStkPtr stackPtr, InstrPtr instrPtr);

#ifdef __cplusplus
} /* extern "C" */

// C++ overloads for dump_bat_state(bid)
inline void _debug_bat_state0(const char* prefix, bat id){ _debug_bat_state0(prefix, &id); }
inline void _debug_bat_state0(const char* prefix, BAT* bat){ _debug_BAT_state0(prefix, bat); }

/**
 * In C++ use the single macro DEBUG_DUMP(x) to dump the content of any variable
 */
#include <iostream>
#include "bat_handle.hpp"

template <typename T>
void _debug_dump0(const char* prefix, T value){
	std::cout << prefix << ": " << value << std::endl;
}

// specializations
template<> void _debug_dump0<BAT*>(const char* prefix, BAT* value);
template<> void _debug_dump0<bool>(const char* prefix, bool value);
template<> void _debug_dump0<char*>(const char* prefix, char* value);
template<> void _debug_dump0<gr8::BatHandle>(const char* prefix, gr8::BatHandle handle);

#define DEBUG_DUMP(value) _debug_dump0("["#value"]",  value);

#endif /* __cplusplus */

#endif /* DEBUG_H_ */
