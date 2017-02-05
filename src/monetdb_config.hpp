/*
 * monetdb_config.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#ifndef MONETDB_CONFIG_HPP_
#define MONETDB_CONFIG_HPP_

// Include monetdb_config.h

#ifdef __cplusplus // it can also be included through common.h
	extern "C" {
#	undef ATOMIC_FLAG_INIT // make MonetDB happy
#endif
#include "monetdb_config.h"
#include "mal_exception.h"

// Defined in <mal_interpreter.h>
ptr getArgReference(MalStkPtr stk, InstrPtr pci, int k);

#ifdef __cplusplus
#	undef throw // this is a keyword in C++
#endif
#ifdef __cplusplus
	} /* extern "C" */
#endif

#endif /* MONETDB_CONFIG_HPP_ */
