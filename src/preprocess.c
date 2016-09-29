#include "monetdb_config.h" // this should be at the top

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

// MonetDB include files
#include "mal_exception.h"

#include "debug.h"

#if !defined(NDEBUG) /* debug only */
#define _CHECK_ERRLINE_EXPAND(LINE) #LINE
#define _CHECK_ERRLINE(LINE) _CHECK_ERRLINE_EXPAND(LINE)
#define _CHECK_ERRMSG(EXPR, ERROR) "[" __FILE__ ":" _CHECK_ERRLINE(__LINE__) "] " ERROR ": `" #EXPR "'"
#else /* release mode */
#define _CHECK_ERRMSG(EXPR, ERROR) ERROR
#endif
#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, _CHECK_ERRMSG( EXPR, ERROR ) ); goto error; }

// Get the value in position p from the bat b of type oid
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
	CHECK(input->T.type == TYPE_oid || input->T.type == TYPE_void, ILLEGAL_ARGUMENT);
	CHECK(BATtordered(input), ILLEGAL_ARGUMENT ": the input column is not sorted");

	// DEBUG ONLY
	printf("<<graph.prefixsum>>\n"); fflush(stdout);
	bat_debug(input);

	if(BATcount(input) == 0){ // edge case
		CHECK(output = COLnew(input->hseqbase /*=0*/, input->T.type, 0, TRANSIENT), MAL_MALLOC_FAIL);
		goto success;
	}

	if(input->T.type == TYPE_oid){
		// Standard case, the BAT contains a sorted sequence of OIDs
		capacity = *((oid*) Tloc(input, BUNlast(input)) -1); // BUNlast is the position after the last element!
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

	} else {
		assert(input->T.type == TYPE_void);
		// TYPE_void is an optimisation where the BAT holds a sequence of IDs [seqbase, seqbase+1, ..., seqbase+cnt -1]
		// This can happen in the extreme case where each vertex has only an outgoing edge. We just materialise the BAT
		// with the sequence of oids
		capacity = BATcount(input);
		CHECK(output = COLnew(input->hseqbase /*=0*/, TYPE_oid, capacity, TRANSIENT), MAL_MALLOC_FAIL);
		for(oid i = input->hseqbase; i < capacity; i++){
			BUNappend(output, &i, FALSE);
		}
	}

	bat_debug(output);

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
 *  Load a graph created with the app. graphgen into three bats from, to, weight.
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

		tempc = strtoll(p, &ptr, 10); // edge from
		CHECK(tempc > 0 || (tempc == 0 && p != ptr), RUNTIME_LOAD_ERROR);
		oid e_from = tempc;

		p = ptr; // move ahead
		while(*p && GDKisspace(*p)) p++; // skip empty spaces
		CHECK(p - buffer < buffer_sz, RUNTIME_LOAD_ERROR); // invalid format

		tempc = strtoll(p, &ptr, 10); // edge to
		CHECK(tempc > 0 || (tempc == 0 && p != ptr), RUNTIME_LOAD_ERROR);
		oid e_to = tempc;

		p = ptr; // move ahead
		while(*p && GDKisspace(*p)) p++;  // skip empty spaces
		CHECK(p - buffer < buffer_sz, RUNTIME_LOAD_ERROR); // invalid format

		tempc = strtoll(p, &ptr, 10); // edge weight
		CHECK(tempc > 0 || (tempc == 0 && p != ptr), RUNTIME_LOAD_ERROR);
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


/**
 *  Load a set of queries from the given text file
 *  Only required for debugging & testing purposes.
 */
mal_export str
GRAPHloadq(bat* ret_id_qfrom, bat* ret_id_qto, str* path) {
	// declarations
	const char* function_name = "graph.loadq";
	str rc = MAL_SUCCEED; // function return code
	BAT *qfrom = NULL, *qto = NULL; // created bats
	const BUN COLnew_initial_capacity = 1024; // arbitrary value
	FILE* file = NULL;
	const int buffer_sz = 1024; // arbitrary value
	char buffer[buffer_sz];
	char* ptr = NULL; // pointer in the buffer
	long long int tempc; // for converting input integers

	// create the new bats
	CHECK(qfrom = COLnew(0, TYPE_oid, COLnew_initial_capacity, TRANSIENT), MAL_MALLOC_FAIL);
	CHECK(qto = COLnew(0, TYPE_oid, COLnew_initial_capacity, TRANSIENT), MAL_MALLOC_FAIL);

	// open the input file
	file = fopen(*path, "r");
	CHECK(file != NULL, RUNTIME_FILE_NOT_FOUND);

	while(!feof(file)){
		ptr = fgets(buffer, buffer_sz, file);
		if(ptr == NULL && feof(file)) break; // no characters to extract
		CHECK(!ferror(file), RUNTIME_STREAM_INPUT);

		char* p = buffer;
		while(*p && GDKisspace(*p)) p++; // skip empty spaces
		if(p - buffer >= buffer_sz || *p == '\0') continue; // this line only contains spaces
		if(*p == '#') continue; // comment, skip this line

		tempc = strtoll(p, &ptr, 10); // source
		CHECK(tempc > 0 || (tempc == 0 && p != ptr), RUNTIME_LOAD_ERROR);
		oid src = (oid) tempc;

		p = ptr; // move ahead

		while(*p && GDKisspace(*p)) p++; // skip empty spaces
		CHECK(p - buffer < buffer_sz, RUNTIME_LOAD_ERROR); // invalid format

		tempc = strtoll(p, &ptr, 10); // to
		CHECK(tempc > 0 || (tempc == 0 && p != ptr), RUNTIME_LOAD_ERROR);
		oid dst = (oid) tempc;

		CHECK(BUNappend(qfrom, &src, 0) == GDK_SUCCEED, MAL_MALLOC_FAIL);
		CHECK(BUNappend(qto, &dst, 0) == GDK_SUCCEED, MAL_MALLOC_FAIL);
	}

	fclose(file);

	// set the bats as read-only. As we are going to scatter them with BATslice, this will allow
	// to create views instead of full partitioned copies
	qfrom->S.restricted = BAT_READ;
	qto->S.restricted = BAT_READ;

	BBPkeepref(qfrom->batCacheid);
	BBPkeepref(qto->batCacheid);

	// Return values
	*ret_id_qfrom = qfrom->batCacheid;
	*ret_id_qto = qto->batCacheid;

	return rc;
error:
	// remove the created bats
	if(qfrom) { BBPunfix(qfrom->batCacheid); }
	if(qto) { BBPunfix(qto->batCacheid); }
	if(file) { fclose(file); }

	return rc;
}


/**
 * Store the result of the shortest paths to a file in the disk, to be validated externally.
 * Only required for debugging & testing purposes.
 */
mal_export str
GRAPHsave(void* dummy, str* path, bat* id_qfrom, bat* id_qto, bat* id_weights, bat* id_poid, bat* id_ppath){
	// declarations
	const char* function_name = "graph.loadq";
	str rc = MAL_SUCCEED; // function return code
	BAT *qfrom = NULL, *qto = NULL, *weights = NULL, *poid = NULL, *ppath = NULL;
	// array accessors
	oid* __restrict aqfrom;
	oid* __restrict aqto;
	lng* __restrict aqweights;
	oid* __restrict apoid;
	oid* __restrict appath;
	BUN q_sz = 0, poid_sz = 0, poid_cur = 0;
	oid cur_oid; // current value
	bool first_value; // flag to print a comma at the end of each value
	FILE* file = NULL;

	// silent the warning from the compiler for the unused parameter
	// I cannot believe in C they still don't allow nameless parameters, so old
	(void) dummy;

	// access the bat descriptors
	qfrom = BATdescriptor(*id_qfrom);
	CHECK(qfrom != NULL, RUNTIME_OBJECT_MISSING);
	qto = BATdescriptor(*id_qto);
	CHECK(qto != NULL, RUNTIME_OBJECT_MISSING);
	weights = BATdescriptor(*id_weights);
	CHECK(weights != NULL, RUNTIME_OBJECT_MISSING);
	poid = BATdescriptor(*id_poid);
	CHECK(poid != NULL, RUNTIME_OBJECT_MISSING);
	ppath = BATdescriptor(*id_ppath);
	CHECK(ppath != NULL, RUNTIME_OBJECT_MISSING);

	// sanity checks
	assert(BATcount(qfrom) == BATcount(qto) && "Size mismatch: |qfrom| != |qto|");
	assert(BATcount(qfrom) == BATcount(weights) && "Size mismatch: |qfrom| != |weights|");

	file = fopen(*path, "w");
	CHECK(file != NULL, "Cannot open the output file");

	aqfrom = (oid*) qfrom->theap.base;
	aqto = (oid*) qto->theap.base;
	aqweights = (lng*) weights->theap.base;
	apoid = (oid*) poid->theap.base;
	appath = (oid*) ppath->theap.base;
	q_sz = BATcount(qfrom);
	poid_sz = BATcount(poid);

	cur_oid = qfrom->hseqbase;

	for(BUN i = 0; i < q_sz; i++){
		fprintf(file, "%zu -> %zu [%lld]: ", aqfrom[i], aqto[i], aqweights[i]);

		first_value = true;
		// move to the current oid
		for( /* resume from the previous run */ ; poid_cur < poid_sz && apoid[poid_cur] < cur_oid; poid_cur++);
		// print the path
		for( /* resume from the previous run */ ; poid_cur < poid_sz && apoid[poid_cur] == cur_oid; poid_cur++) {\
			// print the separator
			if(first_value){
				first_value = false;
			} else {
				fprintf(file, ", ");
			}

			// print the value
			fprintf(file, "%zu", appath[poid_cur]);
		}

		fprintf(file, "\n");
		cur_oid++;
	}

error:
	if(qfrom) { BBPunfix(qfrom->batCacheid); }
	if(qto) { BBPunfix(qto->batCacheid); }
	if(weights) { BBPunfix(weights->batCacheid); }
	if(poid) { BBPunfix(poid->batCacheid); }
	if(ppath) { BBPunfix(ppath->batCacheid); }
	if(file) { fclose(file); }

	return rc;
}
