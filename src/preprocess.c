#include "monetdb_config.h" // this should be at the top

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// MonetDB include files
#include "mal_exception.h"

#include "debug.h"

// if set, sort the inputs if they are not already sorted. Otherwise assume they are already sorted, i.e. codegen
// should ensure this property.
//#define GRAPHinterjoinlist_SORT // set from the Makefile

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
GRAPHprefixsum(bat* id_output, const bat* id_input, const lng* ptr_domain_cardinality) {
	const char* function_name = "graph.prefixsum";
	str rc = MAL_SUCCEED;
	BAT *input = NULL, *output = NULL;
	*id_output = bat_nil;
	BUN cardinality = 0;

	CHECK(id_input != NULL, ILLEGAL_ARGUMENT);
	CHECK(ptr_domain_cardinality != NULL, ILLEGAL_ARGUMENT);

	input = BATdescriptor(*id_input);
	cardinality = (BUN) (*ptr_domain_cardinality);

	CHECK(input != NULL, RUNTIME_OBJECT_MISSING);
	CHECK(input->T.nonil, ILLEGAL_ARGUMENT);
	CHECK(input->hseqbase == 0, ILLEGAL_ARGUMENT);
	CHECK(ATOMtype(BATttype(input)) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATtordered(input), ILLEGAL_ARGUMENT ": the input column is not sorted");

//	// DEBUG ONLY
//	printf("<<graph.prefixsum>>\n"); fflush(stdout);
//	bat_debug(input);

	if(cardinality == 0){ // edge case
		CHECK(output = COLnew(input->hseqbase /*=0*/, TYPE_oid, 0, TRANSIENT), MAL_MALLOC_FAIL);
		goto success;
	}

	// initialise the result
	CHECK(output = COLnew(input->hseqbase /*=0*/, TYPE_oid, cardinality, TRANSIENT), MAL_MALLOC_FAIL);

	if(input->T.type == TYPE_oid){
		// Standard case, the BAT contains a sorted sequence of OIDs
		BUN p = 0; // position in the bat
		BUN count = BATcount(input);
		oid base = 0, next = 0, sum = 0;

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
		// FIXME: this branch should be unreachable now
		// TYPE_void is an optimisation where the BAT holds a sequence of IDs [seqbase, seqbase+1, ..., seqbase+cnt -1]
		// This can happen in the extreme case where all vertices have only one outgoing edge. We just materialise the BAT
		// with the sequence of oids
		BUN count = BATcount(input);
		oid value = input->T.seq;

		assert(input->T.type == TYPE_void);

		for(BUN i = 0; i < count; i++){
			BUNappend(output, &value, FALSE);
			value++;
		}
	}

	// fill the remaining vertices in the domain with no outgoing edges
	assert(BATcount(output) <= cardinality);
	if(BATcount(output) < cardinality){
		// BUNlast is the position after the last element
		oid value = BATcount(output) > 0 ? *((oid*) Tloc(output, BUNlast(output)) -1) : 0;
		BUN count = cardinality - BATcount(output);

		for(BUN i = 0; i < count; i++){
			BUNappend(output, &value, FALSE);
		}
	}

//	bat_debug(output);

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

// transform a column from type VOID to type OID
mal_export str
GRAPHvoid2oid(bat* id_output, bat* id_input){
	const char* function_name = "graph.void2oid";
	str rc = MAL_SUCCEED;
	BAT *input = NULL, *output = NULL;
//	*id_output = bat_nil; // suble bug: GRAPHvoid2oid might be invoked with the same args for input/output

	input = BATdescriptor(*id_input);
	CHECK(input != NULL, RUNTIME_OBJECT_MISSING);
	CHECK(ATOMtype(BATttype(input)) == TYPE_oid, ILLEGAL_ARGUMENT);

	// nothing to do, the input is already of type oid
	if(input->T.type == TYPE_oid){
		BBPkeepref(input->batCacheid);
		*id_output = *id_input;
	}
	// otherwise generate a new bat of type oid and copy all values
	else {
		size_t output_sz = (size_t) BATcount(input);
		size_t base = input->T.seq;
		CHECK(output = COLnew(input->hseqbase /*=0*/, TYPE_oid, output_sz, TRANSIENT), MAL_MALLOC_FAIL);
		for(size_t i = 0; i < output_sz; i++){
			((oid*) output->T.heap.base)[i] = base + i;
		}
		BATsetcount(output, output_sz);

		BBPunfix(input->batCacheid);
		BBPkeepref(output->batCacheid);
		*id_output = output->batCacheid;
	}

	return rc;
error:
	if(output) { BBPunfix(output->batCacheid); }
	if(input) { BBPunfix(input->batCacheid); }

	return rc;
}



// helper method for GRAPHinterjoinlist
// initialize the bats for candidates, edge_src and edge_dst and copies the fields
// that are already equal
static str /* PRIVATE FUNCTION */
GRAPHinterjoinlist_init_changes(BAT** out_candidates, BAT** out_edge_src, BAT** out_edge_dst,
		size_t bat_capacity, size_t end_index,
		oid* __restrict in_candidates, oid* __restrict in_edge_src, oid* __restrict in_edge_dst
){
	const char* function_name = "graph.intersect_join_lists";
	str rc = MAL_SUCCEED;
	BAT* candidates = *out_candidates = COLnew(0, TYPE_oid, bat_capacity, TRANSIENT);
	BAT* edge_src = *out_edge_src = COLnew(0, TYPE_oid, bat_capacity, TRANSIENT);
	BAT* edge_dst = *out_edge_dst = COLnew(0, TYPE_oid, bat_capacity, TRANSIENT);

	CHECK(candidates != NULL, MAL_MALLOC_FAIL);
	CHECK(edge_src != NULL, MAL_MALLOC_FAIL);
	CHECK(edge_dst != NULL, MAL_MALLOC_FAIL);

	for(size_t i = 0; i < end_index; i++){
		BUNappend(edge_src, in_edge_src + i, false);
		BUNappend(edge_dst, in_edge_dst + i, false);
		BUNappend(candidates, in_candidates + i, false);
	}

error: // nop, in case of error the bats are going to be release by the invoker
	return rc;
}

#if defined(GRAPHinterjoinlist_SORT)
// prototypes, core functions of MonetDB
str ALGsort12(bat *result, bat *norder, const bat *bid, const bit *reverse, const bit *stable);
str ALGprojection(bat *result, const bat *lid, const bat *rid);

// sort the inputs according to candidates
static str /* private function, do not export as it does not respect the MAL API  */
GRAPHinterjoinlist_sort(bat* candidates, bat* edges){
	str rc = MAL_SUCCEED;
	bat perm = -1;
	bit reverse = false;
	bit stable = false;

	rc = ALGsort12(candidates, &perm, candidates, &reverse, &stable);
	if (rc != NULL) goto error;
	rc = ALGprojection(edges, &perm, edges);
	if (rc != NULL) goto error;

	// remove the voids
	rc = GRAPHvoid2oid(candidates, candidates);
	if (rc != NULL) goto error;
	rc = GRAPHvoid2oid(edges, edges);
	if (rc != NULL) goto error;
error:
	return rc;
}
#endif

/**
 * Remove tuples with values marked as NULL
 */
mal_export str
GRAPHinterjoinlist(bat* out_candidates, bat* out_edge_src, bat* out_edge_dst,
		bat* in_a, bat* in_b, bat* in_c, bat* in_d){
	const char* function_name = "graph.intersect_join_lists";
	str rc = MAL_SUCCEED;
	BAT *a = NULL, *b = NULL, *c = NULL, *d = NULL; // input
	BAT *candidates = NULL, *edge_src = NULL, *edge_dst = NULL; // output
	bool changes = false; // did we remove or alter any element
	oid* __restrict left = NULL; // left candidate list (array pointer)
	oid* __restrict right = NULL; // right candidate list (array pointer)
	oid* __restrict left_src = NULL; // src edges
	oid* __restrict right_dst = NULL; // dst edges
	size_t i = 0, j = 0, left_sz = 0, right_sz = 0, min_sz = 0;

/* Macro to acquire a BAT* from a bat */
#define BATACQUIRE(variable, batptr) \
    variable = BATdescriptor(*batptr); \
	CHECK(variable != NULL, RUNTIME_OBJECT_MISSING); \
	CHECK(ATOMtype(BATttype(variable)) == TYPE_oid, ILLEGAL_ARGUMENT);
/* End of the macro def */

	// acquire the input candidates
	BATACQUIRE(a, in_a);
	BATACQUIRE(b, in_b);

#if !defined(NDEBUG) // DEBUG ONLY: just to check codegen is not messing up with us
	BATACQUIRE(c, in_c);
	BBPunfix(c->batCacheid); c = NULL;
	BATACQUIRE(d, in_d);
	BBPunfix(d->batCacheid); d = NULL;
#endif // DEBUG ONLY

#if defined(GRAPHinterjoinlist_SORT)
	// sort the inputs (if they are not already sorted)
	if(!a->T.sorted){
		BBPunfix(a->batCacheid); a = NULL;

		rc = GRAPHinterjoinlist_sort(in_a, in_c);
		if(rc != NULL) goto error;

		// re-acquire the bat
		a = BATdescriptor(*in_a);
		CHECK(a != NULL, RUNTIME_OBJECT_MISSING);
		CHECK(ATOMtype(BATttype(a)) == TYPE_oid, ILLEGAL_ARGUMENT);
		assert(a->T.sorted);
	}
	if(!b->T.sorted){
		BBPunfix(b->batCacheid); b = NULL;

		rc = GRAPHinterjoinlist_sort(in_b, in_d);
		if(rc != NULL) goto error;

		// re-acquire the bat
		b = BATdescriptor(*in_b);
		CHECK(b != NULL, RUNTIME_OBJECT_MISSING);
		CHECK(ATOMtype(BATttype(b)) == TYPE_oid, ILLEGAL_ARGUMENT);
		assert(b->T.sorted);
	}
#else
	// a is the left candidate list, b is the right candidate list
	// at this point both input should be sorted
	CHECK(a->T.sorted, ILLEGAL_ARGUMENT);
	CHECK(b->T.sorted, ILLEGAL_ARGUMENT);
#endif

	// acquire the input edges
	BATACQUIRE(c, in_c);
	BATACQUIRE(d, in_d);

	left = (oid*) a->T.heap.base;
	right = (oid*) b->T.heap.base;
	left_src = (oid*) c->T.heap.base;
	right_dst = (oid*) d->T.heap.base;
	left_sz = BATcount(a);
	right_sz = BATcount(b);
	min_sz = left_sz < right_sz ? left_sz : right_sz;

	// first loop, skip equal values at the begin
	for(i = 0; i < min_sz && left[i] == right[i]; i++) /* nop */;
	j = i;

	// are there still elements to inspect ?
	if(i < left_sz || j < right_sz) { // uh oh, we need to remove items from the candidate lists
		rc = GRAPHinterjoinlist_init_changes(&candidates, &edge_src, &edge_dst, min_sz, i, left, left_src, right_dst);
		if(rc) goto error;
		assert(candidates != NULL && edge_src != NULL && edge_dst != NULL &&
				"The BATs should have been initialized by GRAPHinterjoinlist_init_changes");
		changes = true;

		// perform a merge scan
		while(i < left_sz && j < right_sz) {
			if(left[i] == right[j]){
				BUNappend(edge_src, left_src + i, false);
				BUNappend(edge_dst, right_dst + j, false);
				BUNappend(candidates, left + i, false);
				i++; j++;
			} else if (left[i] < right[j]) {
				i++;
			} else { // left[i] > right[j]
				j++;
			}
		}
	} // else we are done!

#undef BATACQUIRE

//success:
	// use the materialized columns as output
	if(changes){
		assert(candidates != NULL && edge_src != NULL && edge_dst != NULL);

//		bat_debug(candidates); // DEBUG ONLY
//		bat_debug(edge_src);
//		bat_debug(edge_dst);

		// output
		*out_candidates = candidates->batCacheid;
		*out_edge_src = edge_src->batCacheid;
		*out_edge_dst = edge_dst->batCacheid;
		BBPkeepref(candidates->batCacheid);
		BBPkeepref(edge_src->batCacheid);
		BBPkeepref(edge_dst->batCacheid);

		// release the input
		BBPunfix(a->batCacheid);
		BBPunfix(b->batCacheid);
		BBPunfix(c->batCacheid);
		BBPunfix(d->batCacheid);
	} else { // no changes, keep the same input
		*out_candidates = a->batCacheid;
		BBPkeepref(a->batCacheid);
		BBPunfix(b->batCacheid); // this is equal to 'a'
		*out_edge_src = c->batCacheid;
		BBPkeepref(c->batCacheid);
		*out_edge_dst = d->batCacheid;
		BBPkeepref(d->batCacheid);
	}

	return rc;
error:
	if(a != NULL) { BBPunfix(a->batCacheid); }
	if(b != NULL) { BBPunfix(b->batCacheid); }
	if(c != NULL) { BBPunfix(c->batCacheid); }
	if(d != NULL) { BBPunfix(d->batCacheid); }
	if(candidates != NULL) { BBPunfix(candidates->batCacheid); };
	if(edge_src != NULL) { BBPunfix(edge_src->batCacheid); };
	if(edge_dst != NULL) { BBPunfix(edge_dst->batCacheid); };

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
		BUN e_weight = tempc;

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
	BUN* __restrict aqweights;
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
	aqweights = (BUN*) weights->theap.base;
	apoid = (oid*) poid->theap.base;
	appath = (oid*) ppath->theap.base;
	q_sz = BATcount(qfrom);
	poid_sz = BATcount(poid);

	cur_oid = qfrom->hseqbase;

	for(BUN i = 0; i < q_sz; i++){
		fprintf(file, "%zu -> %zu [%lu]: ", aqfrom[i], aqto[i], aqweights[i]);

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
