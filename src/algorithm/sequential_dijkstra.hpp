/*
 * sequential_dijkstra.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef SEQUENTIAL_DIJKSTRA_HPP_
#define SEQUENTIAL_DIJKSTRA_HPP_


#include <cstddef>
#include <limits>
#include <iostream> // debug only
#include <type_traits>
#include <vector>

#include "graph.hpp"
#include "queue/queue.hpp"
#include "query.hpp"

namespace monetdb {

template <typename V, typename W>
class SequentialDijkstra {
public:
	using vertex_t = V;
	using cost_t = typename Graph<V, W>::cost_t;
	using query_t = Query<vertex_t, cost_t>;
private:
	using queue_t = typename QueueDijkstra<V, W>::type;
	static const cost_t INFINITY = std::numeric_limits<cost_t>::max();

	vertex_t* parents;
	cost_t* distances;
	const Graph<V, W>& graph;
	queue_t queue;

	// Reset the state of the data structures and prepare for the execution using as
	// source the node `src'
	void init(vertex_t src) {
		queue.clear();
		parents[src] = src;
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
					Q.push(e.dest());
				}
			}
		}
	}

	/**
	 * @return true if i src[i] is connected to dst[j], false otherwise
	 */
	bool finish(query_t& q, std::size_t i, std::size_t j){
		auto dst = q.dest(j);

		// did we reach the destination?
		if(distances[dst] == INFINITY) return false; // no, we didn't

		q.join(i, j, distances[dst]);

		// TODO leave the computation of the final path for now
//        // report the output path
//        if(distances[dst] < INFINITY){
//            size_t len = 0;
//            for(V parent = parents[dst]; parent != src; parent = parents[parent]){
//                path.revpath.push_back(parent);
//                len++;
//            }
//            path.revlen.push_back(len);
//        } else {
//            path.revlen.push_back(0);
//        }
//
//        if(distance)
//            *distance = distances[dst]; // output weight

		return true;
	}


	// Single source single destination
	void sssd(query_t& q, std::size_t i, std::size_t j){
		init(q.source(i));
		execute(q.dest(j));
		finish(q, i, j);
	}

	// Single source, multi destination
	std::size_t ssmd(query_t& q, std::size_t i_src, std::size_t j_dst_first, std::size_t j_dst_last){
		std::size_t count = 0; // keep track of how many tuples have been reached

		init(q.source(i_src));
		for(std::size_t j = j_dst_first; j <= j_dst_last; j++){
			if(distances[q.dest(j)] == INFINITY){
				execute(q.dest(j));
			}

			bool connected = finish(q, i_src, j);
		    if(connected) count++;
		}

		return count;
	}

	// Try to re-use the same distance array among consecutive sources
	void filter(query_t& q){
		assert(q.size_left() == q.size_right());
		const std::size_t size = q.size_left();

		auto flush = [this, &q](std::size_t i, std::size_t n){
			if(n == 1){
				sssd(q, i, i);
			} else {
				ssmd(q, i, i, i+n -1);
			}
		};

		std::size_t contiguous_sources = 1;
		for(std::size_t i = 1; i < size; i++){
			if(q.source(i) == q.source(i-1)) {
				contiguous_sources++;
			} else {
				flush(i - contiguous_sources, contiguous_sources);
				contiguous_sources = 1;
			}
		}

		flush(size - contiguous_sources, contiguous_sources);
	}

	void join(query_t& q){
		std::size_t size_last_batch = 0;

		// perform the cross product
		for(std::size_t i = 0; i < q.size_left(); i++){
			// This is a small optimisation: if we have two consecutive sources, do not repeat the
			// computation of Dijkstra, just duplicate the result of the previous batch
			bool compute_dijkstra = (i == 0) || (q.source(i-1) != q.source(i));

			if(compute_dijkstra){
				// compute the shortest path
				size_last_batch = ssmd(q, i, 0, q.size_right() -1);
			} else {
				q.duplicate_tail(size_last_batch);
			}
		}
	}


public:
	SequentialDijkstra(const Graph<V, W>& G) : parents(new vertex_t[G.size()]), distances(new cost_t[G.size()]), graph(G), queue() {

	}

	~SequentialDijkstra(){
		delete[] parents;
		delete[] distances;
	}

	void operator()(query_t& q){
		if(q.empty()) return; // edge case

		// Here it seems we have two different use cases
		if(q.is_filter_only()){
			filter(q);
		} else {
			join(q);
		}
	}

};

} // namespace monetdb


#endif /* SEQUENTIAL_DIJKSTRA_HPP_ */
