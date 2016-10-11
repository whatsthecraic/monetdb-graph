/*
 * queue.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef QUEUE_HPP_
#define QUEUE_HPP_

#include "queue/fifo.hpp"
#include "queue/radixheap.hpp"

namespace monetdb {

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
struct QueueDijkstra<vertex_t, distance_t, typename std::enable_if<std::is_unsigned<distance_t>::value>::type >{
	using type = RadixHeap<vertex_t, distance_t>;
};


} // namespace monetdb
#endif /* QUEUE_HPP_ */
