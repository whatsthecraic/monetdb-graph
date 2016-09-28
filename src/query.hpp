/*
 * query.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef QUERY_HPP_
#define QUERY_HPP_

#include <cassert>


namespace monetdb{

template<typename V, typename W>
class Query{
    // input parameters
    const V* __restrict query_src; // source array
    const V* __restrict query_dst; // destination array
    const std::size_t query_size; // size of the arrays query_src and query_dst

    // query properties
    bool query_compute_cost; // do we want the cost of the final path?
    bool query_compute_path; // do we want to compute the actual path?

    // output parameters
    V* output_src;
    V* output_dst;
    W* output_cost;
    const std::size_t output_capacity; // size of the arrays src/dst/cost
    std::size_t output_last; // last position filled in the arrays src/dst/cost

public:

    Query(V* query_src, V* query_dst, std::size_t query_sz,
          V* output_src, V* output_dst, W* output_cost, std::size_t output_capacity) :
              query_src(query_src), query_dst(query_dst), query_size(query_sz),
              query_compute_cost(false), query_compute_path(false),
              output_src(output_src), output_dst(output_dst), output_cost(output_cost), output_capacity(output_capacity), output_last(-1){

    }

    V source(std::size_t i) const noexcept {
        assert(i < size() && "Index out of bounds");
        return query_src[i];
    }
    V dest(std::size_t i) const noexcept {
        assert(i < size() && "Index out of bounds");
        return query_dst[i];
    }
    std::size_t size() const noexcept {
        return query_size;
    }


    void join(std::size_t i){
        output_last++;
        assert(output_last < output_capacity);
        output_src[output_last] = source(i);
        output_dst[output_last] = dest(i);
    }

    void join(std::size_t i, W cost){
        join(i);
        output_cost[output_last] = cost;
    }

};

}



#endif /* QUERY_HPP_ */
