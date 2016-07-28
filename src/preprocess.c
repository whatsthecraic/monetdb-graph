#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

// MonetDB include files
#include "monetdb_config.h"
#include "mal_exception.h"


#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, ERROR); goto error; }


static oid get(BAT* b, BUN p){
	return *((oid*)Tloc(b, p));
}

// MAL interface
mal_export str
GRAPHprefixsum(bat* id_output, bat* id_input) {
	const char* function_name = "graph.prefixsum";
	str rc = MAL_SUCCEED;
	BAT *input = NULL, *output = NULL;
	BUN capacity = 0; // the capacity of the new column
	BUN count = 0;
	BUN p = 0; // position in the bat
	oid base = 0, next = 0, sum = 0;

	*id_output = bat_nil;

	input = BATdescriptor(*id_input);

	CHECK(input != NULL, RUNTIME_OBJECT_MISSING);
	CHECK(input->T.nonil, ILLEGAL_ARGUMENT);
	CHECK(input->hseqbase == 0, ILLEGAL_ARGUMENT);
	CHECK(input->T.type == TYPE_oid, ILLEGAL_ARGUMENT);

	if(BATcount(input) == 0){ // edge case
		CHECK(output = COLnew(input->hseqbase /*=0*/, input->T.type, 0, TRANSIENT), MAL_MALLOC_FAIL);
		goto success;
	}

	capacity = *((oid*) Tloc(input, BUNlast(input)));
	CHECK(output = COLnew(input->hseqbase /*=0*/, input->T.type, capacity, TRANSIENT), MAL_MALLOC_FAIL);

	count = BATcount(input);
	do {
		next = get(input, p);

		// fill empty vertices
		for( ; base < next; base++ ) BUNappend(output, &sum, 0);

		base = next;
		// sum the number of equal src vertices
		for ( ; p < count && get(input, p) == base; p++ ) sum++;
		BUNappend(output, &sum, 0);

		base++; // move to the next value
	} while( p < count );

success:
	BBPunfix(input->batCacheid);
	*id_output = output->batCacheid;
	BBPkeepref(output->batCacheid);

	return rc;
error:
	if(output) { BBPunfix(output->batCacheid); }
	if(input) { BBPunfix(input->batCacheid); }

	return rc;
}

/**
 *  Load a graph created with the app. randomgen into three bats from, to, weight.
 *  Only required for debugging & testing purposes.
 */
mal_export str
GRAPHload(bat* ret_id_from, bat* ret_id_to, bat* ret_id_weights, str* path) {
	// declarations
	const char* function_name = "graph.load";
	str rc = MAL_SUCCEED; // function return code
	BAT *from = NULL, *to = NULL, *weights = NULL; // created bats
	const BUN COLnew_initial_capacity = 1024; // arbitrary value
	FILE* file = NULL;
	const int buffer_sz = 1024; // arbitrary value
	char buffer[buffer_sz];
	char* ptr = NULL; // pointer in the buffer
	long long int tempc; // for converting input integers

	// create the new bats
	CHECK(from = COLnew(0, TYPE_oid, COLnew_initial_capacity, TRANSIENT), MAL_MALLOC_FAIL);
	CHECK(to = COLnew(0, TYPE_oid, COLnew_initial_capacity, TRANSIENT), MAL_MALLOC_FAIL);
	CHECK(weights = COLnew(0, TYPE_lng, COLnew_initial_capacity, TRANSIENT), MAL_MALLOC_FAIL);

	// open the input file
	file = fopen(*path, "r");
	CHECK(file != NULL, RUNTIME_FILE_NOT_FOUND);

	while(!feof(file)){
		ptr = fgets(buffer, buffer_sz, file);
		if(ptr == NULL && feof(file)) break; // no characters to extract
		CHECK(!ferror(file), RUNTIME_STREAM_INPUT);

		char* p = buffer;
		while(*p && GDKisspace(*p)) p++; // skip empty spaces
		if(p - buffer >= buffer_sz) continue; // this line contains only spaces
		if(*p == '#') continue; // comment, skip this line

		tempc = strtoll(p, &p, 10); // edge from
		CHECK(tempc > 0, RUNTIME_LOAD_ERROR);
		oid e_from = tempc -1; // nodes start from 1 in the randomgen format

		while(*p && GDKisspace(*p)) p++; // skip empty spaces
		CHECK(p - buffer < buffer_sz, RUNTIME_LOAD_ERROR); // invalid format

		tempc = strtoll(p, &p, 10); // edge to
		CHECK(tempc > 0, RUNTIME_LOAD_ERROR);
		oid e_to = tempc -1;

		while(*p && GDKisspace(*p)) p++;  // skip empty spaces
		CHECK(p - buffer < buffer_sz, RUNTIME_LOAD_ERROR); // invalid format

		tempc = strtoll(p, &p, 10); // edge weight
		CHECK(tempc > 0, RUNTIME_LOAD_ERROR);
		lng e_weight = tempc;

		CHECK(BUNappend(from, &e_from, 0) == GDK_SUCCEED, MAL_MALLOC_FAIL);
		CHECK(BUNappend(to, &e_to, 0) == GDK_SUCCEED, MAL_MALLOC_FAIL);
		CHECK(BUNappend(weights, &e_weight, 0) == GDK_SUCCEED, MAL_MALLOC_FAIL);
	}

	fclose(file);

	BBPkeepref(from->batCacheid);
	BBPkeepref(to->batCacheid);
	BBPkeepref(weights->batCacheid);

	// Return values
	*ret_id_from = from->batCacheid;
	*ret_id_to = to->batCacheid;
	*ret_id_weights = weights->batCacheid;

	return rc;
error:
	// remove the created bats
	if(from) { BBPunfix(from->batCacheid); }
	if(to) { BBPunfix(to->batCacheid); }
	if(weights) { BBPunfix(weights->batCacheid); }
	if(file) { fclose(file); }

	return rc;
}
