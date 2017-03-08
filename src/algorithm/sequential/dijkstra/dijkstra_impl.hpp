/*
 * dijkstra_impl.hpp
 *
 *  Created on: 3 Feb 2017
 *      Author: Dean De Leo
 */

#ifndef ALGORITHM_SEQUENTIAL_DIJKSTRA_IMPL_HPP_
#define ALGORITHM_SEQUENTIAL_DIJKSTRA_IMPL_HPP_

#include <cstddef>
#include <memory>
#include <limits>
//#include <iostream> // debug only
#include <type_traits>
#include <vector>

#include "bat_handle.hpp"
#include "joiner.hpp"
#include "query.hpp"
#include "queue.hpp"

namespace gr8 { namespace algorithm { namespace sequential {

template <typename V, typename W, typename Graph>
class SequentialDijkstraImpl {
public:
	using vertex_t = V;
	using cost_t = typename Graph::cost_t;
//	using query_t = Query<vertex_t, cost_t>;
private:
	using queue_t = typename QueueDijkstra<V, W>::type;
	static const cost_t INFINITY = std::numeric_limits<cost_t>::max();

	vertex_t* parents;
	cost_t* distances;
	vertex_t* edge_ids;
	const Graph& graph;
	queue_t queue;

	// raw pointers, do not delete them
	ShortestPath* shortest_path_cb; // current shortest path we are computing
	std::unique_ptr<Joiner> joiner; // current joiner
	std::vector<vertex_t> revpath; // to report a path

	// Reset the state of the data structures and prepare for the execution using as
	// source the node `src'
	void init(vertex_t src) {
		queue.clear();
		parents[src] = src;
		edge_ids[src] = oid_nil;
		cost_t* __restrict D = distances;
		for(std::size_t i = 0, sz = graph.size(); i < sz; i++){
			D[i] = INFINITY;
		}
		D[src] = 0;
		set_root(src);
	}


	template <typename W_t = W>
	typename std::enable_if<!std::is_void<W_t>::value>::type set_root(vertex_t src){
		queue.push({src, 0});
	}

	template <typename W_t = W>
	typename std::enable_if<std::is_void<W_t>::value>::type set_root(vertex_t src){
		queue.push(src);
	}

	// Dijkstra implementation
	template <typename W_t = W>
	typename std::enable_if<!std::is_void<W_t>::value>::type execute(vertex_t dst){
		const auto& G = this->graph;
		vertex_t* __restrict P = this->parents;
		cost_t* __restrict D = this->distances;
		vertex_t* __restrict E = this->edge_ids;
		queue_t& Q = this->queue;

		while(!Q.empty()){
			auto root = Q.front();
			if(root.dst == dst) break; // found

			Q.pop(); // remove min from the queue
			if(root.cost > D[root.dst]) continue; // we already considered this node, ignore

			// relax the edges
			for(const auto& e : G[root.dst]){
				cost_t td = D[root.dst] + e.cost();
				if(td < D[e.dest()]){
					D[e.dest()] = td;
					P[e.dest()] = root.dst;
					E[e.dest()] = e.id();
					Q.push({e.dest(), td});
				}
			}
		}
	}

	// BFS implementation
	template <typename W_t = W>
	typename std::enable_if<std::is_void<W_t>::value>::type execute(vertex_t dst){
		const auto& G = this->graph;
		vertex_t* __restrict P = this->parents;
		cost_t* __restrict D = this->distances;
		vertex_t* __restrict E = this->edge_ids;
		queue_t& Q = this->queue;

		while(!Q.empty()){
			auto root = Q.front();
			if(root == dst) break; // found

			Q.pop(); // remove min from the queue

			// relax the edges
			for(const auto& e : G[root]){
				cost_t td = D[root] + e.cost();
				if(td < D[e.dest()]){
					D[e.dest()] = td;
					P[e.dest()] = root;
					E[e.dest()] = e.id();
					Q.push(e.dest());
				}
			}
		}
	}

	/**
	 * @return true if src is connected to dst, false otherwise
	 */
	bool finish(vertex_t src, vertex_t dst, std::size_t i, std::size_t j){
		// did we reach the destination?
		if(distances[dst] == INFINITY) return false; // no, we didn't

		if(joiner)
			joiner->join(i, j);

		if(shortest_path_cb){
			shortest_path_cb->append(distances[dst]);

			if(shortest_path_cb->compute_path()){
	    		for(vertex_t parent = parents[dst]; parent != src; parent = parents[parent]){
	    			revpath.push_back(edge_ids[parent]);
	    		}
	    		shortest_path_cb->append(revpath, true);
	    		revpath.clear();
			}
		}

		return true;
	}

	/**
	 * @return true if src[i] is connected to dst[j], false otherwise
	 */
	bool finish(Query& q, std::size_t i, std::size_t j){
		auto dst = q.qdst(j);

		// did we reach the destination?
		if(distances[dst] == INFINITY) return false; // no, we didn't

		auto src = q.qsrc(i);

		if(joiner)
			joiner->join(i, j);

		if(shortest_path_cb){
			shortest_path_cb->append(distances[dst]);

			if(shortest_path_cb->compute_path()){
	    		for(vertex_t parent = parents[dst]; parent != src; parent = parents[parent]){
	    			revpath.push_back(parent);
	    		}
	    		shortest_path_cb->append(revpath, true);
	    		revpath.clear();
			}
		}

		return true;
	}

	// Single source single destination
	void sssd(Query& q, std::size_t i, std::size_t j){
		init(q.qsrc(i));
		execute(q.qdst(j));
		finish(q, i, j);
	}

	// Single source, multi destination
	std::size_t ssmd(Query& q, std::size_t i_src, std::size_t j_dst_first, std::size_t j_dst_last){
		std::size_t count = 0; // keep track of how many tuples have been reached

		init(q.qsrc(i_src));
		for(std::size_t j = j_dst_first; j <= j_dst_last; j++){
			if(distances[q.qdst(j)] == INFINITY){
				execute(q.qdst(j));
			}

			bool connected = finish(q, i_src, j);
		    if(connected) count++;
		}

		return count;
	}

	void filter(Query& q){
		assert(q.query_src.size() == q.query_dst.size() && q.query_dst.size()  == q.candidates_left.size());
//		oid* __restrict candidates = q.candidates_left.array<oid>();
		vertex_t* __restrict src = q.query_src.array<vertex_t>();
//		vertex_t* dst = q.query_dst.array<oid>();
		const std::size_t size = q.query_src.size();


		auto flush = [this, &q](std::size_t i, std::size_t n){
			if(n == 1){
				sssd(q, i, i);
			} else {
				ssmd(q, i, i, i+n -1);
			}
		};

		std::size_t contiguous_sources = 1;
		for(std::size_t i = 1; i < size; i++){
			if(src[i] == src[i-1] /*&& candidates[i] == candidates[i-1]*/) {
				contiguous_sources++;
			} else {
				flush(i - contiguous_sources, contiguous_sources);
				contiguous_sources = 1;
			}
		}

		flush(size - contiguous_sources, contiguous_sources);
	}

	void join(Query &q){
		for(std::size_t i = 0; i < q.query_src.size(); i++){
			ssmd(q, i, 0, q.query_dst.size() -1);
		}
	}

//	void join(query_t& q){
//		std::size_t size_last_batch = 0;
//
//		// perform the cross product
//		for(std::size_t i = 0; i < q.size_left(); i++){
//			// This is a small optimisation: if we have two consecutive sources, do not repeat the
//			// computation of Dijkstra, just duplicate the result of the previous batch
//			bool compute_dijkstra = (i == 0) || (q.source(i-1) != q.source(i));
//
//			if(compute_dijkstra){
//				// compute the shortest path
//				size_last_batch = ssmd(q, i, 0, q.size_right() -1);
//			} else {
//				q.duplicate_tail(size_last_batch);
//			}
//		}
//	}


public:
	SequentialDijkstraImpl(const Graph& graph, ShortestPath* sp) :
		parents(new vertex_t[graph.size()]), distances(new cost_t[graph.size()]),
		graph(graph), queue(), shortest_path_cb(sp), joiner(nullptr) {

	}

	~SequentialDijkstraImpl(){
		delete[] parents;
		delete[] distances;
	}

	// Connect only
	void operator()(Query& query, bool join_results){
		if(query.empty()) return; // edge case

		try {
			if(join_results) joiner.reset(new Joiner(query));

			if(query.is_filter_semantics()){
				filter(query);
			} else {
				join(query);
			}

		} catch(...) {
			joiner.reset(nullptr);
			throw; // propagate the exception
		}
		joiner.reset(nullptr);
	}

};

} } } // namespace gr8::algorithm::sequential



#endif /* ALGORITHM_SEQUENTIAL_DIJKSTRA_IMPL_HPP_ */
