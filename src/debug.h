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

#ifdef __cplusplus
} /* extern "C" */


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
