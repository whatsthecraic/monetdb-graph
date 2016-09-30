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

template<typename vertex_t, typename distance_t>
struct QueueDijkstra {
    // empty
};

// circular buffer
template<typename vertex_t>
struct QueueDijkstra<vertex_t, void>{
    using type = FIFO<vertex_t>;
};

} // namespace monetdb

#endif /* QUEUE_HPP_ */
