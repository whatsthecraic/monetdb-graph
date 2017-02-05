/*
 * queue.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef ALGORITHM_SEQUENTIAL_DIJKSTRA_QUEUE_HPP_
#define ALGORITHM_SEQUENTIAL_DIJKSTRA_QUEUE_HPP_

#include "fifo.hpp"
#include "radixheap.hpp"

namespace gr8 { namespace algorithm { namespace sequential {

// Primary template
template<typename vertex_t, typename distance_t, typename _enable = void>
struct QueueDijkstra {
    // empty
};

// Circular buffer
template<typename vertex_t>
struct QueueDijkstra<vertex_t, void>{
    using type = FIFO<vertex_t>;
};

// Radix heap
template<typename vertex_t, typename distance_t>
struct QueueDijkstra<vertex_t, distance_t, typename std::enable_if<std::is_integral<distance_t>::value>::type >{
	using type = RadixHeap<vertex_t, distance_t>;
};


}}} // namespace gr8::algorithm::sequential

#endif /* ALGORITHM_SEQUENTIAL_DIJKSTRA_QUEUE_HPP_ */
