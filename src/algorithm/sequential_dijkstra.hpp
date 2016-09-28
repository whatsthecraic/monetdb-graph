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

//template<typename V>
//struct Path{
//    std::vector<V> revpath;
//    std::vector<std::size_t> revlen;
//};
//
//
//template <typename A, typename Q, typename V, typename W>
//class SequentialAlgorithm{
//public:
//    static const W Infinity = std::numeric_limits<W>::max();
//
//protected:
//    V* parents;
//    W* distances;
//    const Graph<V, W>& graph;
//    Q queue;
//
//
//private:
//
//    void execute(V dst){
//        static_cast<A*>(this)->implementation(dst);
//    }
//
//    void finish(V src, V dst, W* distance, Path<V>& path){
//        // report the output path
//        if(distances[dst] < Infinity){
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
//    }
//
//public:
//    SequentialAlgorithm(const Graph<V, W>& G) : parents(new V[G.size()]), distances(new W[G.size()]), graph(G), queue(){
//
//    }
//
//    ~SequentialAlgorithm(){
//        delete[] parents;
//        delete[] distances;
//    }
//
//    void sssd(V src, V dst, W* out_distance, Path<V>& out_path){
//        init(src);
//        execute(dst);
//        finish(src, dst, out_distance, out_path);
//    }
//
//    void ssmd(V src, V* dst, W* outdist, std::size_t size, Path<V>& path){
//        init(src);
//        for(std::size_t i = 0; i < size; i++){
//            if(distances[dst[i]] == Infinity){
//                execute(dst[i]);
//            }
//
//            finish(src, dst[i], outdist +i, path);
//        }
//    }
//
//    void msmd(V* src, V* dst, W* distances, std::size_t size, Path<V>& path){
//        if(size == 0) return; // edge case
//
//        auto flush = [this, src, dst, distances, &path](std::size_t i, std::size_t n){
//            if(n == 1){
//                sssd(src[i], dst[i], distances +i, path);
//            } else {
//                ssmd(src[i], dst + i, distances +i, n, path);
//            }
//        };
//
//        std::size_t contiguous_sources = 1;
//        for(std::size_t i = 1; i < size; i++){
//            if(src[i] == src[i-1]) {
//                contiguous_sources++;
//            } else {
//                flush(i - contiguous_sources, contiguous_sources);
//                contiguous_sources = 1;
//            }
//        }
//
//        flush(size - contiguous_sources, contiguous_sources);
//    }
//
//};

namespace sequential_dijkstra_internal {

    template<typename vertex_t, typename distance_t>
    struct distance {
        using type = distance_t;
    };

    template<typename vertex_t>
    struct distance<vertex_t, void> {
        using type = Graph<vertex_t>::weight_t;
    };

} // namespace sequential_dijkstra_internal


template <typename V, typename W>
class SequentialDijkstra {
public:
    using distance_t = typename sequential_dijkstra_internal::distance<V, W>::type;
private:
    using queue_t = typename QueueDijkstra<V, W>::type;
    static const distance_t INFINITY = std::numeric_limits<distance_t>::max();


    V* parents;
    distance_t* distances;
    const Graph<V, W>& graph;
    queue_t queue;

    // Reset the state of the data structures and prepare for the execution using as
    // source the node `src'
    void init(V src){
        queue.clear();
        parents[src] = src;
        distance_t* __restrict D = distances;
        for(std::size_t i = 0, sz = graph.size(); i < sz; i++){
            D[i] = INFINITY;
        }
        D[src] = 0;
        queue.push(src, 0);
    }

    void execute(V dst){
        const auto& G = this->graph;
        V* __restrict P = this->parents;
        distance_t* __restrict D = this->distances;
        queue_t& Q = this->queue;

        while(!Q.empty()){
            auto root = Q.front();
            if(root.v == dst) break; // found

            queue.pop(); // remove min from the queue
            if(root.d > D[root.v]) continue; // we already considered this node, ignore

            // relax the edges
            for(const auto& e : G[root.v]){
                W td = D[root.v] + e.w;
                if(td < D[e.v]){
                    D[e.v] = td;
                    P[e.v] = root.v;
                    queue.push({e.v, td});
                }
            }
        }
    }

    void finish(Query<V, W>& q, std::size_t i){
        // did we reach the destination?
        auto dst = q.dest(i);
        if(distances[dst] == INFINITY) return; // no, we didn't

        q.join(i, distances[dst]);

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


    void sssd(Query<V, W>& q, std::size_t i){
        init(q.source(i));
        execute(q.dest(i));
        finish(q, i);
    }

    void ssmd(Query<V, W>& q, std::size_t first, std::size_t last){
        init(q.source(last));
        for(std::size_t i = first; i <= last; i++){
            if(distances[q.dest(i)] == INFINITY){
                execute(q.dest(i));
            }

            finish(q, i);
        }
    }

    void msmd(Query<V, W>& q, std::size_t first, std::size_t last){
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
    SequentialDijkstra(const Graph<V, W>& G) : parents(new V[G.size()]), distances(new W[G.size()]), graph(G), queue() {

    }

    ~SequentialDijkstra(){
        delete[] parents;
        delete[] distances;
    }

    void operator()(Query<V, W>& q){
        if(q.size() > 0)
            msmd(q, 0, q.size() -1);
    }

};

} // namespace monetdb


#endif /* SEQUENTIAL_DIJKSTRA_HPP_ */
