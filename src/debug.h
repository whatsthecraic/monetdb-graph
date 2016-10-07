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
void bat_debug0(const char* prefix, BAT* b);
#define bat_debug(b) bat_debug0("["#b"]",  b);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEBUG_H_ */
