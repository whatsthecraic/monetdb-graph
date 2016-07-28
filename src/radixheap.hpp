#ifndef RADIXHEAP_HPP_
#define RADIXHEAP_HPP_

#include <cassert>
#include <cstring>
#include <limits>

namespace graph {

// Compute the most significant distinguishing index
#if defined(__GNUG__) or defined(__clang__) // gcc & clang only
namespace internal {
	template<typename T>
	typename std::enable_if<sizeof(T) == 4, int>::type MSD(T a, T b){
		if(a == b) return 0; // edge case
		// T. Akiba's trick, from https://github.com/iwiwi/radix-heap/blob/master/radix_heap.h
		return 32 - __builtin_clz(a ^ b);

	}

	template<typename T>
	typename std::enable_if<sizeof(T) == 8, int>::type MSD(T a, T b){
		if(a == b) return 0; // edge case
		return 64 - __builtin_clzll(a ^ b);
	}

	template<typename T>
	typename std::enable_if<sizeof(T) != 8 && sizeof(T) != 4, int>::type MSD(T a, T b){
		if(a == b) return 0; // edge case
		return floor(log2(a ^ b)) +1; // text book implementation
	}
#else // other compilers
	template<typename T>
	typename int MSD(T a, T b){
		if(a == b) return 0; // edge case
		// text book implementation
		return floor(log2(a ^ b)) +1;
	}
#endif

} /* namespace internal */

template<typename vertex_t, typename distance_t>
class RadixHeap {
public:
	struct pair { vertex_t v; distance_t d; };
private:
	const static std::size_t bsize = std::numeric_limits<distance_t>::digits + 1;
	const static std::size_t capmin = 1024; // arbitrary value
	const static distance_t inf = std::numeric_limits<distance_t>::max(); // infinity

	std::size_t capacity[bsize];
	std::size_t size[bsize];
	distance_t bmin[bsize];
	pair* buckets[bsize];
	distance_t lastmin;
	std::size_t hsize;


	int get_bucket_index(distance_t key){
		return internal::MSD(lastmin, key);
	}

	// append the given item into the bucket
	void append(int i, vertex_t v, distance_t d){
		if(size[i] >= capacity[i]){ // realloc
			capacity[i] *= 2;
			pair* temp = new pair[capacity[i]];
			memcpy(temp, buckets[i], sizeof(pair) * size[i]);
			delete[] buckets[i];
			buckets[i] = temp;
		}
		buckets[i][size[i]++] = pair{v, d};
	}


	void push0(vertex_t v, distance_t d){
		assert(d >= lastmin && "Key must be non decreasing");
		int bucketno = get_bucket_index(d);
		append(bucketno, v, d);
		if(d < bmin[bucketno]) bmin[bucketno] = d;
	}

public:
	RadixHeap() : lastmin(0), hsize(0) {
		// initialise the buckets
		for(std::size_t i = 0; i < bsize; i++){
			capacity[i] = 16; // arbitrary value
			size[i] = 0;
			bmin[i] = inf; // infinity
			buckets[i] = new pair[capacity[i]]();
		}
	}

	~RadixHeap(){
		for(std::size_t i = 0; i < bsize; i++){
			delete[] buckets[i];
		}
	}

	void push(vertex_t v, distance_t d){
		push0(v, d);
		hsize++;
	}

	pair pop(){
		assert(!empty());
		// find the first non empty bucket
		int imin = 0;
		while(size[imin] == 0) imin++;

		// re-balance the heap
		if(imin > 0){
			lastmin = bmin[imin];
			pair* __restrict bucket = buckets[imin];
			for(std::size_t j = 0, sz = size[imin]; j < sz; j++){
				assert(get_bucket_index(bucket[j].d) < imin);
				push0(bucket[j].v, bucket[j].d);
			}

			// update the control variables
			bmin[imin] = inf;
			size[imin] = 0;
			if(capacity[imin] > capmin){ // shrink the bucket
				delete[] bucket;
				buckets[imin] = new pair[capmin];
				capacity[imin] = capmin;
			}
		}

		assert(size[0] > 0);
		size[0]--;
		hsize--;
		return buckets[0][size[0]];
	}

	bool empty() const noexcept {
		return hsize == 0;
	}

};

}

#endif /* RADIXHEAP_HPP_ */
