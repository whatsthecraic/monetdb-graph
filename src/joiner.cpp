/*
 * joiner.cpp
 *
 *  Created on: 4 Feb 2017
 *      Author: Dean De Leo
 */


#include "joiner.hpp"

#include <cassert>

#include "debug.h"
#include "errorhandling.hpp"
#include "monetdb_config.hpp"

using namespace gr8;

Joiner::Joiner(Query& q) : query(q),
		cl0(q.candidates_left.array<oid>()),
		cr0(q.is_join_semantics()?q.candidates_right.array<oid>():nullptr),
		el0(q.query_src.array<oid>()),
		er0(q.query_dst.array<oid>()),
		changes(false), finalized(false), is_join_semantics(q.is_join_semantics()), last(0), multiple_aggregates(query.shortest_paths.size() > 1) {
	if(is_join_semantics){
		initchg();
	}
}

Joiner::~Joiner(){
	if(!finalized)
		finalize();
}

void Joiner::initchg(){
	if(changes) RAISE_ERROR("Already initialized");

	// initialize the internal vectors
	jl = COLnew(query.candidates_left.get()->hseqbase, TYPE_oid, query.candidates_left.size(), TRANSIENT);
	MAL_ASSERT(jl.initialised(), MAL_MALLOC_FAIL);
	if(multiple_aggregates){
		el = COLnew(query.query_src.get()->hseqbase, TYPE_oid, query.query_src.size(), TRANSIENT);
		MAL_ASSERT(el.initialised(), MAL_MALLOC_FAIL);
		er = COLnew(query.query_dst.get()->hseqbase, TYPE_oid, query.query_dst.size(), TRANSIENT);
		MAL_ASSERT(er.initialised(), MAL_MALLOC_FAIL);
	}
	if(is_join_semantics){
		jr = COLnew(query.candidates_right.get()->hseqbase, TYPE_oid, query.candidates_right.size(), TRANSIENT);
		MAL_ASSERT(jr.initialised(), MAL_MALLOC_FAIL);
	} else {
		// in case of filter semantics, copy the previous tuples

		BAT* bat_cl = jl.get();
		for(size_t i = 0; i < last; i++){
			BUNappend(bat_cl, cl0 + i, false);
		}

		if(multiple_aggregates){
			BAT* bat_el = el.get();
			BAT* bat_er = er.get();

			for(size_t i = 0; i < last; i++){
				BUNappend(bat_el, el0 + i, false);
				BUNappend(bat_er, er0 + i, false);
			}
		}
	}

	changes = true;
}


void Joiner::join(oid i, oid j){
//	std::cout << "[join] " << i << ", " << j << ", last:" <<  last << std::endl;

	assert(!finalized);

	if(!is_join_semantics){
//		assert(i == j || query.candidates_left.array<oid>()[i] == query.candidates_left.array<oid>()[j]);
		i = j;
		if(j == last && !changes){
			last++;
		} else {
			initchg(); // set `changes' to true
		}
	}

	if(changes){
		BUNappend(jl.get(), cl0 + i, false);
		if(is_join_semantics){
			assert(cr0 != nullptr);
			BUNappend(jr.get(), cr0 + j, false);
		}
		if(multiple_aggregates){
			BUNappend(el.get(), el0 + i, false);
			BUNappend(er.get(), er0 + j, false);
		}
	}
}

void Joiner::finalize(){
	if(finalized) RAISE_ERROR("Already finalized");

	// edge case
	if(!changes && last != query.candidates_left.size()){
		initchg();
	}

	if(changes){
		query.candidates_left = jl;
		query.candidates_right = jr;
		if(multiple_aggregates){
			query.query_src = el;
			query.query_dst = er;
		}
		query.output_left = jl;
		query.output_right = jr;
	} else {
		query.output_left = query.candidates_left;
		query.output_right = query.candidates_right;
	}

	query.set_joined();
}
