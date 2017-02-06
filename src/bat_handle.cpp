/*
 * bat_handle.cpp
 *
 *  Created on: 4 Feb 2017
 *      Author: Dean De Leo
 */

#include "bat_handle.hpp"

#include <atomic>
#include <cassert>
#include <iostream>

#include "errorhandling.hpp"

using namespace gr8;

// Error handling
#ifdef NDEBUG
	#define CHECK(condition, message) CHECK_EXCEPTION(Exception, condition, message)
#else
	#define CHECK(condition, message) assert(condition && message)
#endif

/******************************************************************************
 *                                                                            *
 *  Internal counter                                                          *
 *                                                                            *
 ******************************************************************************/

namespace {
	struct Counter {
	public:
		BAT* content;
		std::atomic_int counter;

		Counter(BAT* content = nullptr) : content(content), counter(1) { }

		~Counter() { release(); }

		bat release(bool logical = false){
			assert(counter >= 0);

			bat id = -1;
			if(content) {
				id = content->batCacheid;
				if(logical){
					BBPkeepref(id);
				} else {
					BBPunfix(id);
				}
			}
			content = nullptr;

			return id;
		}

		void incref() {
			assert(counter >= 0);
			counter++;
		}

		int decref() {
			assert(counter >= 1);
			return --counter;
		}
	};

}


/******************************************************************************
 *                                                                            *
 *  BatHandle                                                                 *
 *                                                                            *
 ******************************************************************************/

#define counter() reinterpret_cast<Counter*>(shared_ptr)
#define incref() counter()->incref()

BatHandle::BatHandle() : shared_ptr(new Counter()){ }

BatHandle::BatHandle(BAT* b, bool allow_null) : shared_ptr(new Counter()){
	initialize(b, allow_null);
}

BatHandle::BatHandle(bat* b, bool allow_null) : shared_ptr(new Counter()){
	initialize(b, allow_null);
}

BatHandle::BatHandle(bat b, bool allow_null) : shared_ptr(new Counter()){
	initialize(b, allow_null);
}

void BatHandle::initialize(BAT* b, bool allow_null) {
	MAL_ASSERT(allow_null || b != nullptr, RUNTIME_OBJECT_MISSING);
	counter()->content = b;

}
void BatHandle::initialize(bat* bat_id, bool allow_null){
	MAL_ASSERT(allow_null || bat_id != nullptr, RUNTIME_OBJECT_MISSING);
	if(allow_null && bat_id == nullptr){
		initialize((BAT*) nullptr, allow_null);
	} else {
		initialize(*bat_id, allow_null);
	}
}

void BatHandle::initialize(bat bat_id, bool allow_null){
	MAL_ASSERT(allow_null || bat_id != -1, RUNTIME_OBJECT_MISSING);
	if(allow_null && bat_id == -1){
		initialize((BAT*) nullptr, allow_null);
	} else {
		auto content = BATdescriptor(bat_id);
		if(content == nullptr){ // to ease debugging
			MAL_ASSERT_MSG(false, RUNTIME_OBJECT_MISSING, "Invalid bat id: " << bat_id);
		}
		counter()->content = content;
	}
}

void BatHandle::decref(){
	if(shared_ptr) {
		auto value = counter()->decref();
		assert(value >= 0);
		if(value == 0){
			delete counter(); shared_ptr = nullptr;
		}
	}
}

BatHandle::BatHandle(const BatHandle& h) : shared_ptr(h.shared_ptr){
	incref();
}
BatHandle& BatHandle::operator= (const BatHandle& h) {
	decref();
	shared_ptr = h.shared_ptr;
	incref();
	return *this;
}
BatHandle& BatHandle::operator= (BatHandle&& h){
	decref();
	shared_ptr = h.shared_ptr;
	h.shared_ptr = nullptr;
	return *this;
}

BatHandle::~BatHandle(){
	decref();
}

bool BatHandle::initialized () const{
	return shared_ptr != nullptr && counter()->content != nullptr;
}

bool BatHandle::empty() const {
	return !initialized() || size() == 0;
}

size_t BatHandle::size() const {
	return (size_t) BATcount(get());
}

BAT* BatHandle::get(){
	CHECK(shared_ptr != nullptr && counter()->content != nullptr, "Empty handle");
	return counter()->content;
}
BAT* BatHandle::get() const{
	CHECK(shared_ptr != nullptr && counter()->content != nullptr, "Empty handle");
	return counter()->content;
}

bat BatHandle::release(bool logical){
	return counter()->release(logical);
}
