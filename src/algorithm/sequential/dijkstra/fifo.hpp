/*
 * fifo.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef ALGORITHM_SEQUENTIAL_DIJKSTRA_FIFO_HPP_
#define ALGORITHM_SEQUENTIAL_DIJKSTRA_FIFO_HPP_

#include <cassert>
#include <cstring> // memcpy

namespace gr8 { namespace algorithm { namespace sequential {

namespace fifo_internal {
    template<typename V, typename W>
    struct pair{
        V dst;
        W cost;
    };

    template<typename V, typename W>
    struct value{
        using value_t = pair<V, W>;
    };

    template<typename V>
    struct value<V, void>{
        using value_t = V;
    };
}


template<typename vertex_t, typename distance_t = void>
class FIFO{
public:
    using value_t = typename fifo_internal::value<vertex_t, distance_t>::value_t;
private:
    static constexpr std::size_t INITIAL_CAPACITY = 1024; // arbitrary value

    value_t* queue;
    std::size_t startpos;
    std::size_t endpos;
    std::size_t capacity;

    // double the capacity of the queue
    void expand(){
        assert(endpos == startpos);

        // copy the content of the old container
        value_t* queue_cpy = new value_t[capacity*2];
        auto first_chunk_length = capacity - startpos;
        memcpy(queue_cpy, queue + startpos, first_chunk_length * sizeof(value_t));
        memcpy(queue_cpy + first_chunk_length, queue, startpos * sizeof(value_t));

        // update the internal indices
        startpos = 0;
        endpos = capacity;
        capacity *= 2;

        // switch to the new queue
        delete[] queue;
        queue = queue_cpy;
    }

public:
    FIFO() : queue(new value_t[INITIAL_CAPACITY]), startpos(0), endpos(0), capacity(INITIAL_CAPACITY) { }

    ~FIFO(){
        delete[] queue;
    }


    value_t front(){
        assert(!empty());
        return queue[startpos];
    }

    void pop(){
        assert(!empty());
        startpos = (startpos +1) % capacity;
    }

    void push(value_t v){
        queue[endpos] = v;
        endpos = (endpos +1) % capacity;
        if(endpos == startpos){ // relloc
            expand();
        }
    }


    bool empty() const noexcept {
        return startpos == endpos;
    }

    void clear() {
        startpos = endpos = 0;

        // shall we resize the queue to the initial capacity?
//        delete[] queue;
//        capacity = INITIAL_CAPACITY;
//        queue = new value[capacity];
    }

    std::size_t size() const {
        if(startpos <= endpos){
            return endpos - startpos;
        } else {
            return (capacity - startpos) + endpos;
        }
    }

};

}}} // namespace gr8::algorithm::sequential



#endif /* ALGORITHM_SEQUENTIAL_DIJKSTRA_FIFO_HPP_ */
