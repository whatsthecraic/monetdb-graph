#ifndef ALGORITHMS_HPP_
#define ALGORITHMS_HPP_

#include <cstddef>
#include <limits>
#include <iostream>
#include <vector>

#include "graph.hpp"
#include "radixheap.hpp"

namespace monetdb {

template<typename V>
struct Path{
	std::vector<V> revpath;
	std::vector<std::size_t> revlen;
};


template <typename A, typename Q, typename V, typename W>
class SequentialAlgorithm{
public:
	static const W Infinity = std::numeric_limits<W>::max();

protected:
	V* parents;
	W* distances;
	const Graph<V, W>& graph;
	Q queue;


private:
	void init(V src){
		queue.clear();
		parents[src] = src;
		W* __restrict D = distances;
		for(std::size_t i = 0, sz = graph.size(); i < sz; i++){
			D[i] = Infinity;
		}
		D[src] = 0;
		queue.push(src, 0);
	}

	void execute(V dst){
		static_cast<A*>(this)->implementation(dst);
	}

	void finish(V src, V dst, W* distance, Path<V>& path){
		// report the output path
		if(distances[dst] < Infinity){
			size_t len = 0;
			for(V parent = parents[dst]; parent != src; parent = parents[parent]){
				path.revpath.push_back(parent);
				len++;
			}
			path.revlen.push_back(len);
		} else {
			path.revlen.push_back(0);
		}

		if(distance)
			*distance = distances[dst]; // output weight
	}

public:
	SequentialAlgorithm(const Graph<V, W>& G) : parents(new V[G.size()]), distances(new W[G.size()]), graph(G), queue(){

	}

	~SequentialAlgorithm(){
		delete[] parents;
		delete[] distances;
	}

	void sssd(V src, V dst, W* out_distance, Path<V>& out_path){
		init(src);
		execute(dst);
		finish(src, dst, out_distance, out_path);
	}

	void ssmd(V src, V* dst, W* outdist, std::size_t size, Path<V>& path){
		init(src);
		for(std::size_t i = 0; i < size; i++){
			if(distances[dst[i]] == Infinity){
				execute(dst[i]);
			}

			finish(src, dst[i], outdist +i, path);
		}
	}

	void msmd(V* src, V* dst, W* distances, std::size_t size, Path<V>& path){
		if(size == 0) return; // edge case

		auto flush = [this, src, dst, distances, &path](std::size_t i, std::size_t n){
			if(n == 1){
				sssd(src[i], dst[i], distances +i, path);
			} else {
				ssmd(src[i], dst + i, distances +i, n, path);
			}
		};

		std::size_t contiguous_sources = 1;
		for(std::size_t i = 1; i < size; i++){
			if(src[i] == src[i-1]) {
				contiguous_sources++;
			} else {
				flush(i - contiguous_sources, contiguous_sources);
				contiguous_sources = 1;
			}
		}

		flush(size - contiguous_sources, contiguous_sources);
	}

};


template <typename Q, typename V, typename W>
class SequentialDijkstra : public SequentialAlgorithm<SequentialDijkstra<Q, V, W>, Q, V, W>{
	friend class SequentialAlgorithm<SequentialDijkstra<Q, V, W>, Q, V, W>;

	void implementation(V dst){
		const auto& G = this->graph;
		V* P = this->parents;
		W* D = this->distances;
		Q& queue = this->queue;

		while(!queue.empty()){
			auto root = queue.front();
			if(root.v == dst) break; // found

			queue.pop(); // remove min from the queue
			if(root.d > D[root.v]) continue; // we already considered this node, ignore

			// relax the edges
			for(const auto& e : G[root.v]){
				W td = D[root.v] + e.w;
				if(td < D[e.v]){
					D[e.v] = td;
					P[e.v] = root.v;
					queue.push(e.v, td);
				}
			}
		}
	}

public:
	SequentialDijkstra(const Graph<V, W>& G) : SequentialAlgorithm<SequentialDijkstra, Q, V, W>(G) { }
};



} /* namespace graph */

#endif /* ALGORITHMS_HPP_ */
