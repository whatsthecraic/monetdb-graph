/*
 * debug.h
 *
 *  Created on: 29 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef __cplusplus
extern "C" {
#undef ATOMIC_FLAG_INIT // make MonetDB happy
#endif
#include "monetdb_config.h"
#ifdef __cplusplus
#undef throw // this is a keyword in C++
#endif

// Print the whole content of a BAT to stdout (similar to io.print);
void _bat_debug0(const char* prefix, BAT* b);
#define bat_debug(b) _bat_debug0("["#b"]",  b);

#ifdef __cplusplus
} /* extern "C" */
#endif


/**
 * In C++ use the single macro DEBUG_DUMP(x) to dump the content of any variable
 */
#ifdef __cplusplus
#include <iostream>
template <typename T>
void _debug_dump0(const char* prefix, T value){
	std::cout << prefix << ": " << value << std::endl;
}
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

#define DEBUG_DUMP(value) _debug_dump0("["#value"]",  value);
#endif

#endif /* DEBUG_H_ */
