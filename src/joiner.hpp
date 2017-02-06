/*
 * joiner.hpp
 *
 *  Created on: 3 Feb 2017
 *      Author: Dean De Leo
 */

#ifndef JOINER_HPP_
#define JOINER_HPP_

#include "bat_handle.hpp"
#include "monetdb_config.hpp"
#include "query.hpp"

namespace gr8 {

class Joiner {
	Query& query;
	const oid* __restrict cl0; // candidate array (left)
	const oid* __restrict cr0; // candidate array (right)
	const oid* __restrict el0; // current edges src
	const oid* __restrict er0; // current edges src
	BatHandle jl; // temporary left (only valid in case of changes)
	BatHandle jr; // temporary right (only valid in case of changes)
	BatHandle el; // temporary edges src
	BatHandle er; // temporary edges dst
	bool changes;
	bool finalized;
	const bool is_join_semantics;
	std::size_t last;
	const bool multiple_aggregates;

	void initchg();

public:
	Joiner(Query& query);
	~Joiner();

	void join(oid i, oid j);

	void finalize();
};

}



#endif /* JOINER_HPP_ */
