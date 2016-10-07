/**
 * Set of utility functions
 */

#include "common.h"

// System includes
#include <math.h>

// MonetDB includes
#include <gdk.h>

// Defined in <mal_interpreter.h>
ptr getArgReference(MalStkPtr stk, InstrPtr pci, int k);

// Graph library includes
#include "debug.h"

/*
 * Split the given input BAT into multiple bats of equal sizes. The number of BATs that should be created
 * is defined by the number of variables expected in the output
 */
mal_export str
GRAPHslicer(void* cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr p){
	const char* function_name = "graph.slicer";
	str rc = MAL_SUCCEED;
	BAT* input = NULL;
	bte input_type;
	BUN num_partitions, input_sz, step_sz;
	BUN pstart; // start of the current partition
	BAT** container = NULL;

	// validate input parameters
	CHECK(p->argc >= 2, ILLEGAL_ARGUMENT ": Wrong number of arguments (< 2)");
	CHECK(p->retc >= 1, ILLEGAL_ARGUMENT ": Expected at least one return parameter");
	CHECK(p->argc - p->retc == 1, ILLEGAL_ARGUMENT ": Expected exactly one input parameter");

	// retrieve the input
	input = BATdescriptor(*((bat*)getArgReference(stk, p, p->retc)));
	CHECK(input != NULL, RUNTIME_OBJECT_MISSING);
	input_type = ATOMtype(input->T.type);

	// check the types for the output bats is the same as the input
	for(BUN i = 0; i < p->retc; i++){
		int type = getArgType(mb, p, i);
		int bat_type = -1;

		CHECK(isaBatType(type), ILLEGAL_ARGUMENT);
		bat_type = getBatType(type);
		if(bat_type == TYPE_any) {
			setArgType(mb, p, i, newBatType(input_type)); // FIXME not sure it has any effect at this point
		} else {
			CHECK(bat_type == input_type || (bat_type == TYPE_oid && input_type == TYPE_void), ILLEGAL_ARGUMENT);
		}
	}

	// compute the size of the partitions
	input_sz = BATcount(input);
	num_partitions = p->retc;
	step_sz = ceil(((double) input_sz) / num_partitions);
	// MonetDB defines its own version of calloc
	container = calloc(num_partitions, sizeof(BAT*));

	// Create the slices
	pstart = 0;
	for(BUN i = 0; i < num_partitions; i++){
		BUN low = pstart;
		BUN high = low + step_sz;
		BAT* slice = container[i] = BATslice(input, low, high);
//		bat_debug(slice);
		CHECK(slice != NULL, MAL_MALLOC_FAIL);
		slice->hseqbase = 0; // reset the partition no.

		if(slice->T.type == TYPE_void){
			 // the output has type TYPE_void, materialize as TYPE_oid
			oid value = slice->T.seq;
			const size_t matslice_sz = BATcount(slice);
			BAT* matslice = COLnew(0, TYPE_oid, matslice_sz, TRANSIENT);

			CHECK(matslice != NULL, MAL_MALLOC_FAIL);

			for(size_t j = 0; j < matslice_sz; j++){
				((oid*) matslice->T.heap.base)[j] = value;
				value++;
			}
			BATsetcount(matslice, matslice_sz);

			container[i] = matslice;
			BBPunfix(slice->batCacheid);
		}

		pstart = high; // next iteration
	}

	BATfree(input);

	// Return the slices
	for(BUN i = 0; i < num_partitions; i++){
//		bat_debug(container[i]);
		BBPkeepref(container[i]->batCacheid);
		stk->stk[p->argv[i]].val.bval = container[i]->batCacheid;
	}

	free(container); // gdk.h

	return rc;
error:
	BATfree(input);

	if(container){
		for(BUN i = 0; i < num_partitions; i++){
			BATfree(container[i]);
		}
		free(container);
	}

	return rc;
}
