/*
 * bat_handle.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: dleo@cwi.nl
 */

#ifndef BAT_HANDLE_HPP_
#define BAT_HANDLE_HPP_

#include <stdexcept>

#include "errorhandling.hpp"
#include "monetdb_config.hpp"

namespace gr8  {

// Shared pointer to a MonetDB BAT
class BatHandle {
private:
	void* shared_ptr;

	void initialize(bat bat_id, bool allow_null);
	void initialize(bat* bat_id, bool allow_null);
	void initialize(BAT* bat, bool allow_null);

	void decref();

public:
	// constructors
	BatHandle();
	BatHandle(BAT* b, bool allow_null = false);
	BatHandle(bat* bat_id, bool allow_null = false);
	BatHandle(bat bat_id, bool allow_null = false);

	// copy & move semantics
	BatHandle(const BatHandle& h);
	BatHandle(BatHandle&& h){ shared_ptr = h.shared_ptr; h.shared_ptr = nullptr; }
	BatHandle& operator= (const BatHandle& h);
	BatHandle& operator= (BatHandle&& h);

	// destructor
	~BatHandle();

	// does it contain a BAT?
//	operator bool () const; // evil
	bool initialized() const;
	bool empty() const;
	std::size_t size() const;

	// ask to release the BAT (shared among all pointers)
	bat release(bool logical);
	bat release_logical(){ return release(true); }
	void release_physical(){ release(false); }

	// retrieve the held BAT
	BAT* get();
	BAT* get() const;

	bat id() const {
		return get()->batCacheid;
	}


	template<typename T>
	T* array(){
		return reinterpret_cast<T*>(get()->T.heap.base);
	}

	template<typename T>
	T* array() const{
		return reinterpret_cast<T*>(get()->T.heap.base);
	}

	template<typename T>
	T at(size_t i) const{
		CHECK_EXCEPTION(Exception, !empty(), "The BAT is empty");
		return array<T>()[i];
	}
	template<typename T>
	T first() const{
		return at<T>(0);
	}

	template<typename T>
	T last() const{
		return at<T>(size() -1);
	}

	int type() const {
		return ATOMtype(get()->T.type);
	}
};


}

#endif /* BAT_HANDLE_HPP_ */
