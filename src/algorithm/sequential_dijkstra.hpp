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
#include <iostream>
#include <vector>

#include "graph.hpp"
#include "queue/queue.hpp"
#include "query.hpp"

namespace monetdb {

namespace sequential_dijkstra_internal {

    template<typename vertex_t, typename distance_t>
    struct cost {
        using type = distance_t;
    };

    template<typename vertex_t>
    struct cost<vertex_t, void> {
        using type = typename Graph<vertex_t>::cost_t;
    };

} // namespace sequential_dijkstra_internal


template <typename V, typename W>
class SequentialDijkstra {
public:
	using vertex_t = V;
    using cost_t = typename sequential_dijkstra_internal::cost<V, W>::type;
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
        queue.push({src, 0});
    }

    // FIXME: remove the computed cost iff this is a BFS
    void execute(vertex_t dst){
        const auto& G = this->graph;
        vertex_t* __restrict P = this->parents;
        cost_t* __restrict D = this->distances;
        queue_t& Q = this->queue;

        while(!Q.empty()){
            auto root = Q.front();
            if(root.dst == dst) break; // found

            queue.pop(); // remove min from the queue
            if(root.cost > D[root.dst]) continue; // we already considered this node, ignore

            // relax the edges
            for(const auto& e : G[root.dst]){
                cost_t td = D[root.dst] + e.cost();
                if(td < D[e.dest()]){
                    D[e.dest()] = td;
                    P[e.dest()] = root.dst;
                    queue.push({e.dest(), td});
                }
            }
        }
    }

    void finish(query_t& q, std::size_t i){
        auto dst = q.dest(i);

        // did we reach the destination?
        if(distances[dst] == INFINITY) return; // no, we didn't

        q.join(i, distances[dst]);

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
    }


    // Single source single destination
    void sssd(query_t& q, std::size_t i){
        init(q.source(i));
        execute(q.dest(i));
        finish(q, i);
    }

    // Single source, multi destination
    void ssmd(query_t& q, std::size_t first, std::size_t last){
        init(q.source(last));
        for(std::size_t i = first; i <= last; i++){
            if(distances[q.dest(i)] == INFINITY){
                execute(q.dest(i));
            }

            finish(q, i);
        }
    }

    // Multi source, multi destination
    void msmd(query_t& q, std::size_t first, std::size_t last){
        std::size_t size = last +1 - first;

        auto flush = [this, &q](std::size_t i, std::size_t n){
            if(n == 1){
                sssd(q, i);
            } else {
                ssmd(q, i, i+n -1);
            }
        };

        std::size_t contiguous_sources = 1;
        for(std::size_t i = first +1; i <= last; i++){
            if(q.source(i) == q.source(i-1)) {
                contiguous_sources++;
            } else {
                flush(i - contiguous_sources, contiguous_sources);
                contiguous_sources = 1;
            }
        }

        flush(size - contiguous_sources, contiguous_sources);
    }


public:
    SequentialDijkstra(const Graph<V, W>& G) : parents(new vertex_t[G.size()]), distances(new cost_t[G.size()]), graph(G), queue() {

    }

    ~SequentialDijkstra(){
        delete[] parents;
        delete[] distances;
    }

    void operator()(query_t& q){
        if(q.size() > 0)
            msmd(q, 0, q.size() -1);
    }

};

} // namespace monetdb


#endif /* SEQUENTIAL_DIJKSTRA_HPP_ */
