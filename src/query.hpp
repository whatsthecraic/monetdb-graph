/*
 * query.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#ifndef QUERY_HPP_
#define QUERY_HPP_

#include <cassert>
//#include <iostream> // debug only
#include <memory>
#include <vector>

#include "bat_handle.hpp"
#include "graph_descriptor.hpp"


namespace gr8 {

// Forward declarations
class Query;

/******************************************************************************
 *                                                                            *
 *   Shortest path descriptor                                                 *
 *                                                                            *
 ******************************************************************************/

struct ShortestPath {
private:
	friend class Query;

	Query* _query;
	bool _initialised;
	int _pos_output_cost;
	int _pos_output_path;

	ShortestPath(Query* q, BatHandle&& weights, int pos_output_cost, int pos_output_path);

	void append_cost0(void* value);
	void append_path0(const std::vector<oid>& path, bool reversed);

public:
	BatHandle weights;
	BatHandle computed_cost;
	BatHandle computed_path;

	void initialise(std::size_t capacity);
	bool initialised() const;

	// do we need to perform a BFS visit?
	bool bfs() const;

	// do we need to report the path ?
	bool compute_path() const;

	int get_pos_cost() const;
	int get_pos_path() const;

	template <typename T>
	void append(T value){
//		std::cout << "append_value: " <<  value << std::endl; // debug only
		append_cost0(&value);
	}

	void append(const std::vector<oid>& path, bool reversed = true){
		append_path0(path, reversed);
	}
};

/******************************************************************************
 *                                                                            *
 *   Query descriptor (or control block)                                      *
 *                                                                            *
 ******************************************************************************/

enum OperationType {
	op_invalid,
	op_join,
	op_filter,
};


class Query {
private: // do not copy this object
	Query(const Query&) = delete;
	Query& operator=(const Query&) = delete;

	bool _joined; // have been the candidate lists already joined?
	int _pos_output_left; // expected position for the output columns (jl)
	int _pos_output_right; // jr

public:
	OperationType operation;

	// handles
	BatHandle candidates_left;
	BatHandle candidates_right;
	BatHandle query_src;
	BatHandle query_dst;
	std::unique_ptr<GraphDescriptor> graph;
	std::vector<ShortestPath> shortest_paths;
	BatHandle output_left;
	BatHandle output_right;

	Query();
	~Query();

	bool is_join_semantics() const { return operation == op_join && !is_joined(); }
	bool is_filter_semantics() const { return operation == op_filter || is_joined(); }

	bool empty() const {
		return query_src.empty();
	}

	void request_shortest_path(BatHandle&& weights, int pos_output, int pos_path);

	oid qsrc(std::size_t index) const{
		return query_src.array<oid>()[index];
	}

	oid qdst(std::size_t index) const{
		return query_dst.array<oid>()[index];
	}

	bool is_joined() const;
	void set_joined();

	int get_pos_output_left() const;
	void set_pos_output_left(int value);
	int get_pos_output_right() const;
	void set_pos_output_right(int value);
};


}


#endif /* QUERY_HPP_ */
