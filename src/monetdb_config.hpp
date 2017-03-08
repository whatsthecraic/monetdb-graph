/*
 * monetdb_config.hpp
 * Include monetdb_config.h from the C/C++ environment
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#ifndef MONETDB_CONFIG_HPP_
#define MONETDB_CONFIG_HPP_

#ifdef __cplusplus // it can also be included through common.h
// make MonetDB happy
#	ifdef ATOMIC_FLAG_INIT
#		define ATOMIC_FLAG_INIT_OLD_VALUE ATOMIC_FLAG_INIT
#		undef ATOMIC_FLAG_INIT
#	endif

extern "C" {
#endif

#include "monetdb_config.h"
#include "mal_exception.h"

// Defined in <mal_interpreter.h>
ptr getArgReference(MalStkPtr stk, InstrPtr pci, int k);

#ifdef __cplusplus
} /* extern "C" */

#	undef throw // this is a keyword in C++

#	ifdef ATOMIC_FLAG_INIT_OLD_VALUE
#		undef ATOMIC_FLAG_INIT
#		define ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT_OLD_VALUE
#		undef ATOMIC_FLAG_INIT_OLD_VALUE
#	endif
#endif

#endif /* MONETDB_CONFIG_HPP_ */
