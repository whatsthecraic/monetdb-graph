/*
 * parse_request.hpp
 *
 *  Created on: 5 Feb 2017
 *      Author: Dean De Leo
 */

#ifndef PARSE_REQUEST_HPP_
#define PARSE_REQUEST_HPP_

#include "monetdb_config.hpp"
#include "query.hpp"

namespace gr8 {

DEFINE_EXCEPTION(ParseError);

void parse_request(Query& query, MalStkPtr stackPtr, InstrPtr instrPtr);

}



#endif /* PARSE_REQUEST_HPP_ */
