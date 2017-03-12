/*
 * global_configuration.hpp
 *
 *  Created on: 8 Mar 2017
 *      Author: Dean De Leo
 */

#ifndef SRC_CONFIGURATION_HPP_
#define SRC_CONFIGURATION_HPP_

#include "errorhandling.hpp"

namespace gr8 {

DEFINE_EXCEPTION(ConfigurationError);

class Configuration{
private:
	bool _dump_parser; // whether the parser should dump to stdout the incoming request (for debug purposes)
	bool _initialised; // has the singleton instance been initialised?
	int _type_nested_table; // reference to the physical type representing a nested table

public:
	bool dump_parser() const {
		return _dump_parser;
	}

	bool initialised() const {
		return _initialised;
	}

	int type_nested_table() const{
		return _type_nested_table;
	}


private:
	// singleton interface
	static Configuration instance;

public:
	static void initialise();
	static Configuration& configuration();
};


// Alias
inline Configuration& configuration(){
	return Configuration::configuration();
}

} // namespace gr8



#endif /* SRC_CONFIGURATION_HPP_ */
