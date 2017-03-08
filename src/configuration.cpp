/*
 * configuration.cpp
 *
 *  Created on: 8 Mar 2017
 *      Author: Dean De Leo
 */

#include "configuration.hpp"

#include <cstring>

#include "errorhandling.hpp"
#include "monetdb_config.hpp"

using namespace gr8;

// error handling
#define CHECK(condition, message) if(!(condition)) RAISE_EXCEPTION(gr8::ConfigurationError, message);

/******************************************************************************
 *                                                                            *
 *   Singleton interface                                                      *
 *                                                                            *
 ******************************************************************************/
Configuration Configuration::instance;

void Configuration::initialise(){
	CHECK(!instance.initialised(), "Configuration already initialized");

	// dump_parser
	char* env_debug = getenv("GRAPH_DUMP_PARSER");
	instance._dump_parser = env_debug != nullptr && (strcmp(env_debug, "1") == 0 || strcmp(env_debug, "true") == 0);

	// nested table type
	int TYPE_nested_table = ATOMindex("nestedtable");
	CHECK(TYPE_nested_table >= 0, "Type 'nestedtable' not found");
	instance._type_nested_table = TYPE_nested_table;

	instance._initialised = true;
}

Configuration& Configuration::configuration(){
	CHECK(instance.initialised(), "Configuration not initialised");
	return instance;
}
