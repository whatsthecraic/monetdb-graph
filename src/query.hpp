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

template<typename vertex_t, typename cost_t>
class Query{
    // input parameters
    const vertex_t* query_src; // source array
    const vertex_t* query_dst; // destination array
    const std::size_t query_size; // size of the arrays query_src and query_dst

    // query properties
    bool query_compute_path; // do we want to compute the actual path?

    // output parameters
    vertex_t* output_filter;
    cost_t* output_cost;
    const std::size_t output_capacity; // size of the arrays src/dst/cost
    std::size_t output_last; // last position filled in the arrays src/dst/cost

public:
    Query(vertex_t* query_src, vertex_t* query_dst, std::size_t query_sz,
		  vertex_t* output_filter, cost_t* output_cost, std::size_t output_capacity) :
              query_src(query_src), query_dst(query_dst), query_size(query_sz),
              query_compute_path(false),
			  output_filter(output_filter), output_cost(output_cost), output_capacity(output_capacity), output_last(-1){

    }

    vertex_t source(std::size_t i) const noexcept {
        assert(i < size() && "Index out of bounds");
        return query_src[i];
    }
    vertex_t dest(std::size_t i) const noexcept {
        assert(i < size() && "Index out of bounds");
        return query_dst[i];
    }
    std::size_t size() const noexcept {
        return query_size;
    }

    // Number of tuples joined
    std::size_t count() const noexcept {
    	return output_last +1;
	}

    // Do we need to compute the cost of the path?
    bool compute_cost() const noexcept {
    	return output_cost != nullptr;
    }

    bool compute_path() const noexcept {
    	return query_compute_path;
    }



    void join(std::size_t i) noexcept {
        output_last++;
        assert(output_last < output_capacity);
        output_filter[output_last] = i;
    }

    void join(std::size_t i, cost_t cost) noexcept {
        join(i);
        set_cost(cost);
    }

    void set_cost(cost_t cost) noexcept {
    	if(compute_cost()){
    		output_cost[output_last] = cost;
    	}
    }
};

// Disable this type, something went wrong the in the type inference
template<typename vertex_t> class Query<vertex_t, void>{ };

} // namespace monetdb


#endif /* QUERY_HPP_ */
