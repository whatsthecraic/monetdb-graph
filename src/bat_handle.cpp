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
 *  Internal content                                                          *
 *                                                                            *
 ******************************************************************************/
BatHandle::content_t::content_t(BAT* handle): handle(handle), ref_logical(false) { }
BatHandle::content_t::~content_t(){
	if(!handle) return;
	if(ref_logical){
		BBPkeepref(handle->batCacheid);
	} else {
		BBPunfix(handle->batCacheid);
	}
}


/******************************************************************************
 *                                                                            *
 *  BatHandle                                                                 *
 *                                                                            *
 ******************************************************************************/
BatHandle::BatHandle() : shared_ptr(){ }
BatHandle::~BatHandle(){ }

BatHandle::BatHandle(BAT* b, bool allow_null) : shared_ptr(){
	initialize(b, allow_null);
}

BatHandle::BatHandle(bat* b, bool allow_null) : shared_ptr(){
	initialize(b, allow_null);
}

BatHandle::BatHandle(bat b, bool allow_null) : shared_ptr(){
	initialize(b, allow_null);
}

void BatHandle::initialize(BAT* b, bool allow_null) {
	MAL_ASSERT(allow_null || b != nullptr, RUNTIME_OBJECT_MISSING);
	shared_ptr.reset(new content_t(b));

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
		initialize(BATdescriptor(bat_id), allow_null);
	}
}

BatHandle::BatHandle(const BatHandle& h) : shared_ptr(h.shared_ptr){

}

BatHandle& BatHandle::operator= (const BatHandle& h) {
	shared_ptr = h.shared_ptr;
	return *this;
}

BatHandle& BatHandle::operator= (BatHandle&& h){
	shared_ptr = h.shared_ptr;
	h.shared_ptr = nullptr;
	return *this;
}


bool BatHandle::initialised () const{
	return shared_ptr.get() != nullptr && shared_ptr->handle != nullptr;
}

bool BatHandle::empty() const {
	return !initialised() || size() == 0;
}

size_t BatHandle::size() const {
	return (size_t) BATcount(get());
}

bat BatHandle::release(bool value){
	if(shared_ptr.get() != nullptr){
		shared_ptr->ref_logical = value;
		return shared_ptr->handle->batCacheid;
	} else {
		return -1;
	}
}
