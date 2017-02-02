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
	// disable copy ctors
	Query(const Query& q) = delete;
	Query& operator= (const Query& q) = delete;

	// input parameters
	const vertex_t* query_perm_left; // projection ids for the source tuples
	const vertex_t* query_perm_right; // projection ids for the destionation tuples
	const vertex_t* query_src; // source array
	const vertex_t* query_dst; // destination array
	const std::size_t query_left_sz; // size of the arrays query_perm_left and query_src
	const std::size_t query_right_sz; // size of the arrays query_perm_right and query_dst

	// query properties
	bool query_filter_only; // is this a filter or a join (cross-product + filter)?
	bool query_compute_path; // do we want to compute the actual path?

	// output parameters
	vertex_t* output_perm_left;
	vertex_t* output_perm_right;
	cost_t* output_cost;
	const std::size_t output_capacity; // size of the arrays src/dst/cost
	std::size_t output_last; // last position filled in the arrays src/dst/cost

public:
	Query(vertex_t* query_perm_left, vertex_t* query_perm_right,
		  vertex_t* query_src, vertex_t* query_dst,
		  std::size_t query_left_sz, std::size_t query_right_sz,
		  vertex_t* output_perm_left, vertex_t* output_perm_right, cost_t* output_cost,
		  std::size_t output_capacity
	) :
		query_perm_left(query_perm_left), query_perm_right(query_perm_right),
		query_src(query_src), query_dst(query_dst),
		query_left_sz(query_left_sz), query_right_sz(query_right_sz),
		output_perm_left(output_perm_left), output_perm_right(output_perm_right),
		output_cost(output_cost), output_capacity(output_capacity),
		output_last(-1)
	{
		query_filter_only = false;
		query_compute_path = false;
	}


	void set_filter_only(bool value){ query_filter_only = value; }
	bool is_filter_only() const { return query_filter_only; }

	// Do we need to compute the cost of the path?
	bool compute_cost() const noexcept {
		return output_cost != nullptr;
	}

	vertex_t source(std::size_t i) const noexcept {
		assert(i < query_left_sz && "Index out of bounds");
		return query_src[i];
	}
	vertex_t dest(std::size_t i) const noexcept {
		assert(i < query_right_sz && "Index out of bounds");
		return query_dst[i];
	}

	// Number of tuples joined
	std::size_t count() const noexcept {
		return output_last +1;
	}

	bool compute_path() const noexcept {
		return query_compute_path;
	}

	void join(std::size_t i, std::size_t j) noexcept {
		assert(i < query_left_sz && "Index out of bounds");
		assert(j < query_right_sz && "Index out of bounds");
		output_last++;
		assert(output_last < output_capacity);
		output_perm_left[output_last] = query_perm_left ? query_perm_left[i] : i;
		output_perm_right[output_last] = query_perm_right ? query_perm_right[j] : j;
	}

	void join(std::size_t i, std::size_t j, cost_t cost) noexcept {
		join(i, j);
		set_cost(cost);
	}

	void set_cost(cost_t cost) noexcept {
		if(compute_cost()){
			output_cost[output_last] = cost;
		}
	}

	bool empty() const noexcept { return query_left_sz == 0 || query_right_sz == 0; }

	std::size_t size_left() const { return query_left_sz;  }
	std::size_t size_right() const { return query_right_sz; }


	/**
	 * This is a small optimization. Duplicate the last N matches.
	 */
	void duplicate_tail(std::size_t n){
		assert(count() > n && "Size of the output less than n");
		assert(output_last + n < output_capacity && "Not enough space");

		memcpy(output_perm_left + output_last, output_perm_left + output_last - n, n * sizeof(vertex_t));
		memcpy(output_perm_right + output_last, output_perm_right + output_last - n, n * sizeof(vertex_t));
		if(compute_cost()){
			memcpy(output_cost + output_last, output_cost + output_last - n, n * sizeof(cost_t));
		}

		output_last += n;
	}
};

// Disable this type, something went wrong the in the type inference
template<typename vertex_t> class Query<vertex_t, void>{ };

} // namespace monetdb


#endif /* QUERY_HPP_ */
