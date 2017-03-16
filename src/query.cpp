/*
 * query.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#include "query.hpp"

#include <algorithm> // for_each
#include <iterator>

#include "configuration.hpp"

using namespace gr8;
using namespace std;

// error handling
#define CHECK(condition, message) if (!(condition)) RAISE_EXCEPTION(gr8::Exception, message);

/******************************************************************************
 *                                                                            *
 *   Shortest paths                                                           *
 *                                                                            *
 ******************************************************************************/

ShortestPath::ShortestPath(Query* q, BatHandle&& weights, int pos_output, int pos_path) :
	_query(q), _initialised(false), _pos_output_cost(pos_output), _pos_output_path(pos_path), weights(move(weights)) {

}

void ShortestPath::initialise(std::size_t capacity){
	assert(!initialised());
	computed_cost = COLnew(0, !bfs() ? weights.type() : TYPE_lng /* keep in sync with edge::cost_t */, capacity, TRANSIENT);
	if(!computed_cost.initialised()){ RAISE_ERROR("Cannot initialized the array computed_cost with capacity: " << capacity); }

	if(compute_path()){
		computed_path = COLnew(0, configuration().type_nested_table(), capacity, TRANSIENT);
		if(!computed_path.initialised()){ RAISE_ERROR("Cannot initialized the array computed_path_lengths with capacity: " << capacity); }

		BAT* batptr = computed_path.get();
		batptr->tvarsized = 1;
		batptr->tkey = 1;
		batptr->tdense = batptr->tnodense = 0;
		batptr->tsorted = 1;
		batptr->trevsorted = batptr->tnorevsorted = 0;
		batptr->tnonil = 1; batptr->tnil = 0;
		batptr->batDirty = 1;
	}

	_initialised = true;
}

bool ShortestPath::initialised() const {
	return _initialised;
}

void ShortestPath::append_cost0(void* value){
	assert(initialised());
	BUNappend(computed_cost.get(), value, false);
}


void ShortestPath::append_path0(const vector<oid>& path, bool reversed){
	assert(initialised());
	assert(compute_path());
	BAT* output = computed_path.get();

//	// debug only
//	cout << "[append_path] size: " << path.size() << "; values: ";
//	for(oid o: path){
//		cout << o << " ";
//	}
//	cout << "\n";

	// append to the vheap
	Heap* vheap = output->T.vheap;
	const size_t sz = path.size();
	var_t offset = HEAP_malloc(vheap, (sz + 1) * sizeof(oid)) << GDK_VARSHIFT;
	if(!offset) MAL_ERROR(MAL_MALLOC_FAIL, "append_path: cannot allocate the space to store the path: " << ((sz + 1) * sizeof(oid)));
	oid* base = (oid*) (vheap->base + offset);

	auto do_append_path = [&base](oid value) {
		*(base++) = value;
	};

	do_append_path((oid) sz); // length of the path
	if(!reversed){
		for_each(begin(path), end(path), do_append_path);
	} else {
//		for_each(rbegin(path), rend(path), do_append_path); // C++14
		for_each(path.rbegin(), path.rend(), do_append_path);
	}

	// append to the theap
	Heap& /*t*/heap = output->T.heap; // theap is reserved, damn macros
	if(BATcapacity(output) < BATcount(output) +1){ // check we have enough space to append
		if(BUN_MAX == BATcapacity(output))
			MAL_ERROR(MAL_MALLOC_FAIL, "append_path: the heap is full and no more elements can be inserted (BUN_MAX)");

		auto rc = BATextend(output, BATgrows(output));
		if(rc != GDK_SUCCEED)
			MAL_ERROR(MAL_MALLOC_FAIL, "append_path: no available space to append the offset: " << BATcapacity(output));
	}
	var_t* pos = (var_t*) (heap.base + heap.free);
	*pos = offset;
	heap.free += sizeof(var_t);
	output->batCount++;
}

bool ShortestPath::bfs() const{ // do we need to perform a BFS visit?
//	return ((bool) weights) == false;
	return weights.initialised() == false;
}

bool ShortestPath::compute_path() const {
	return _pos_output_path != -1;
}

int ShortestPath::get_pos_cost() const {
	return _pos_output_cost;
}

int ShortestPath::get_pos_path() const {
	CHECK_EXCEPTION(Exception, compute_path(), "Path not required");
	return _pos_output_path;
}

/******************************************************************************
 *                                                                            *
 *   Query descriptor (or control block)                                      *
 *                                                                            *
 ******************************************************************************/

Query::Query() : _joined(false), _pos_output_left(0), _pos_output_right(1), operation(op_invalid){

}

Query::~Query() { }

void Query::request_shortest_path(BatHandle&& weights, int pos_output, int pos_path){
	shortest_paths.push_back( ShortestPath{this, move(weights), pos_output, pos_path} );
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
void Query::set_output_empty(){
	output_left = COLnew(0, TYPE_oid, 0, TRANSIENT);
	MAL_ASSERT(output_left.initialised(), MAL_MALLOC_FAIL);
	output_right = COLnew(0, TYPE_oid, 0, TRANSIENT);
	MAL_ASSERT(output_right.initialised(), MAL_MALLOC_FAIL);
}


