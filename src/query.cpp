/*
 * query.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#include "query.hpp"

#include <algorithm> // for_each
#include <iterator>

using namespace gr8;
using namespace std;


/******************************************************************************
 *                                                                            *
 *   Shortest paths                                                           *
 *                                                                            *
 ******************************************************************************/

ShortestPath::ShortestPath(Query* q, BatHandle&& weights, int pos_output, int pos_pathlen, int pos_pathvalues) :
	_query(q), _initialized(false), _pos_output(pos_output), _pos_pathlen(pos_pathlen),_pos_pathvalues(pos_pathvalues), weights(move(weights)) {

}

void ShortestPath::initialize(std::size_t capacity){
	assert(!initialized());
	if(!bfs()){
		computed_cost = COLnew(0, TYPE_oid, capacity, TRANSIENT);
		if(!computed_cost){ RAISE_ERROR("Cannot initialized the array computed_cost with capacity: " << capacity); }

		if(compute_path()){
			computed_path_lengths = COLnew(0, TYPE_lng, capacity, TRANSIENT);
			if(!computed_path_lengths){ RAISE_ERROR("Cannot initialized the array computed_path_lengths with capacity: " << capacity); }

			computed_path_values = COLnew(0, TYPE_lng, capacity * 4, TRANSIENT);
			if(!computed_path_values){ RAISE_ERROR("Cannot initialized the array computed_path_lengths with capacity: " << capacity *4); }
		}
	}
	_initialized = true;
}

bool ShortestPath::initialized() const {
	return _initialized;
}

void ShortestPath::append_cost0(void* value){
	assert(initialized());
	assert(!bfs());
	BUNappend(computed_cost.get(), value, false);
}


void ShortestPath::append_path0(const vector<oid>& path, bool reversed){
	assert(initialized());
	assert(!bfs() && compute_path());
	lng sz = path.size();
	BUNappend(computed_path_lengths.get(), &sz, false);

	auto do_append_path = [this](oid value) {
		BUNappend(computed_path_values.get(), &value, false);
	};

	if(!reversed){
		for_each(begin(path), end(path), do_append_path);
	} else {
//		for_each(rbegin(path), rend(path), do_append_path); // C++14
		for_each(path.rbegin(), path.rend(), do_append_path);
	}
}

bool ShortestPath::bfs() const{ // do we need to perform a BFS visit?
	return ((bool) weights) == false;
}

bool ShortestPath::compute_path() const {
	return _pos_pathlen != -1 && _pos_pathvalues != -1;
}

int ShortestPath::get_pos_output() const {
	return _pos_output;
}

int ShortestPath::get_pos_pathlen() const {
	CHECK_EXCEPTION(Exception, compute_path(), "Path not required");
	return _pos_pathlen;
}

int ShortestPath::get_pos_pathvalues() const {
	CHECK_EXCEPTION(Exception, compute_path(), "Path not required");
	return _pos_pathvalues;
}

/******************************************************************************
 *                                                                            *
 *   Query descriptor (or control block)                                      *
 *                                                                            *
 ******************************************************************************/

Query::Query() : _joined(false), _pos_output_left(0), _pos_output_right(1), operation(op_invalid){

}

Query::~Query() { }

void Query::request_shortest_path(BatHandle&& weights, int pos_output, int pos_pathlen, int pos_pathvalues){
	shortest_paths.push_back( ShortestPath{this, move(weights), pos_output, pos_pathlen, pos_pathvalues} );
}

bool Query::is_joined() const {
	return _joined;
}

void Query::set_joined() {
	if(_joined) RAISE_ERROR("Already joined");
	_joined = true;
}

int Query::get_pos_output_left() const {
	return _pos_output_left;
}
void Query::set_pos_output_left(int value) {
	_pos_output_left = value;
}
int Query::get_pos_output_right() const {
	return _pos_output_right;
}
void Query::set_pos_output_right(int value){
	_pos_output_right = value;
}


