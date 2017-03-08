#include <iostream>
#include <string>

#include "debug.h"
#include "errorhandling.hpp"
#include "monetdb_config.hpp"
#include "parse_request.hpp"
#include "prepare.hpp"
#include "query.hpp"

#include "algorithm/sequential/dijkstra/dijkstra.hpp"
#include "third-party/tinyxml2.hpp"

using namespace gr8;
using namespace std;



/******************************************************************************
 *                                                                            *
 *   Process a request                                                        *
 *                                                                            *
 ******************************************************************************/


static void handle_request(Query& query){
	reorder_computations(query);
	prepare_graph(query);

	algorithm::sequential::SequentialDijkstra algo;

	if(query.shortest_paths.empty()){ // do not compute any shortest path
		algo.execute(query, nullptr, true);
	} else { // we need to compute a shortest path
		size_t capacity = 0; // initial capacity of the output columns related to the shortest paths

		{ // first time, execute join the tuples
			ShortestPath* sp = &(query.shortest_paths.front());
			capacity = query.candidates_left.size();
			if(query.is_join_semantics()) capacity *= query.candidates_right.size();
			sp->initialise(capacity);
			algo.execute(query, &(query.shortest_paths.front()), true);
		}

		capacity = query.shortest_paths.front().computed_cost.size();

		// next iterations, only compute a shortest path
		for(size_t i = 1, sz = query.shortest_paths.size(); i < sz; i++){
			ShortestPath* sp = &query.shortest_paths[i];
			sp->initialise(capacity);
			algo.execute(query, sp, false);
		}
	}
}


/******************************************************************************
 *                                                                            *
 *   MonetDB interface                                                        *
 *                                                                            *
 ******************************************************************************/
extern "C" {

// C error handling
#undef CHECK // in case already exists
#if !defined(NDEBUG) /* debug only */
#define _CHECK_ERRLINE_EXPAND(LINE) #LINE
#define _CHECK_ERRLINE(LINE) _CHECK_ERRLINE_EXPAND(LINE)
#define _CHECK_ERRMSG(EXPR, ERROR) "[" __FILE__ ":" _CHECK_ERRLINE(__LINE__) "] " ERROR ": `" #EXPR "'"
#else /* release mode */
#define _CHECK_ERRMSG(EXPR, ERROR) ERROR
#endif
#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, _CHECK_ERRMSG( EXPR, ERROR ) ); goto error; }

// Entry point from the MAL layer
// We have our little API convention with the codegen...
str GRAPHspfw(void* cntxt, MalBlkPtr mb, MalStkPtr stackPtr, InstrPtr instrPtr) noexcept {
	str rc = MAL_SUCCEED;
	const char* function_name = "graph.spfw";

	auto set_arg = [stackPtr, instrPtr](int index, bat value) { stackPtr->stk[instrPtr->argv[index]].val.bval = value; };

	try {
		// init the query
		Query query;
		parse_request(query, stackPtr, instrPtr);

//		DEBUG_DUMP(query.candidates_left);
//		DEBUG_DUMP(query.candidates_right);
//		DEBUG_DUMP(query.query_src);
//		DEBUG_DUMP(query.query_dst);
//		DEBUG_DUMP(((GraphDescriptorColumns*) query.graph.get())->edge_src)
//		DEBUG_DUMP(((GraphDescriptorColumns*) query.graph.get())->edge_dst)

		// execute the query
		handle_request(query);

		// output
		// candidate ids
//		DEBUG_DUMP(query.output_left);
		set_arg(query.get_pos_output_left(), query.output_left.release_logical()); // jl
		if(query.output_right.initialised()) {
//			DEBUG_DUMP(query.output_right);
			set_arg(query.get_pos_output_right(), query.output_right.release_logical()); // jr
		}
		// shortest paths
		for(auto& sp : query.shortest_paths){
//			DEBUG_DUMP(sp.computed_cost);
			set_arg(sp.get_pos_output(), sp.computed_cost.release_logical() );
			if(sp.compute_path()){
				set_arg(sp.get_pos_path(), sp.computed_path.release_logical());
			}
		}

	} catch(gr8::Exception& e){ // internal exception
		if(dynamic_cast<gr8::MalException*>(&e)){
			rc = createException(MAL, function_name, "%s", ((MalException*) &e)->get_mal_error());
		} else {
			rc = createException(MAL, function_name, OPERATION_FAILED);
		}

		cerr << ">> Exception " << e.getExceptionClass() << " raised at " << e.getFile() << ", line: " << e.getLine() << "\n";
		cerr << ">> Cause: " << e.what() << "\n";
		cerr << ">> Operation failed!" << endl;

		goto error;
	} catch(std::bad_alloc& b) {
		CHECK(0, MAL_MALLOC_FAIL);
	} catch(...) { // no exception shall pass
		CHECK(0, OPERATION_FAILED); // generic error
	}

error:
	return rc;
}
} // extern "C"
