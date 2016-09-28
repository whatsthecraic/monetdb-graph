/*
 * fifo.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef FIFO_HPP_
#define FIFO_HPP_

#include <cassert>
#include <cstring> // memcpy

namespace monetdb {

namespace fifo_internal {
    template<typename V, typename W>
    struct pair{
        V v;
        W d;
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


template<typename vertex_t, typename distance_t>
class FIFO{
public:
    using value = typename fifo_internal::value<vertex_t, distance_t>::value_t;
private:
    static constexpr std::size_t INITIAL_CAPACITY = 1024; // arbitrary value

    value* queue;
    std::size_t startpos;
    std::size_t endpos;
    std::size_t capacity;

    // double the capacity of the queue
    void expand(){
        assert(endpos == startpos);

        // copy the content of the old container
        value* queue_cpy = new value[capacity*2];
        memcpy(queue_cpy, queue + startpos, (capacity - startpos) * sizeof(value));
        memcpy(queue_cpy, queue, startpos * sizeof(value));

        // update the internal indices
        startpos = 0;
        endpos = capacity;
        capacity *= 2;

        // switch to the new queue
        delete[] queue;
        queue = queue_cpy;
    }

public:
    FIFO() : queue(new value[INITIAL_CAPACITY]), startpos(0), endpos(0), capacity(INITIAL_CAPACITY) { }

    ~FIFO(){
        delete[] queue;
    }


    value front(){
        assert(!empty());
        return queue[startpos];
    }

    void pop(){
        assert(!empty());
        startpos = (startpos +1) % capacity;
    }

    void push(value v){
        queue[endpos] = v;
        endpos = (endpos +1) % capacity;
        if(endpos == startpos){ // relloc
            expand();
        }
    }


    bool empty() const noexcept {
        return startpos == endpos;
    }

    void clear() const{
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

} // namespace monetdb



#endif /* FIFO_HPP_ */
