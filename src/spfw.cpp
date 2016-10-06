#include <cstddef> // std::size_t
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "algorithm/sequential_dijkstra.hpp"
#include "graph.hpp"
#include "query.hpp"


#undef ATOMIC_FLAG_INIT // make MonetDB happy
// MonetDB include files
extern "C" {
#include "monetdb_config.h"
#include "mal_exception.h"
}
#undef throw // this is a keyword in C++

#include "debug.h" // DEBUG ONLY

using namespace monetdb;

using std::size_t; // omnipresent

// assume vertex_t is `oid' from now on, no reason to make the code more generic
using vertex_t = oid; // oid id defined as size_t in monetdb_config.h

// graph representation
template<typename weight_t>
using graph_t = Graph<vertex_t, weight_t>;

// cost_t is the same of weight_t, except in case weights are not given. Here
// the cost represents the number of hops in a path, and each edge has cost 1.
template<typename weight_t>
using cost_t = typename graph_t<weight_t>::cost_t;

// I think I finally understood how these template aliases do work
template<typename weight_t>
using query_t = Query<vertex_t, cost_t<weight_t>>;


/******************************************************************************
 *                                                                            *
 *   C++ operator                                                             *
 *                                                                            *
 ******************************************************************************/

template<typename W>
static void execute(graph_t<W>& graph, query_t<W>& query){
	SequentialDijkstra<vertex_t, W> algorithm(graph);
	algorithm(query);
}


/******************************************************************************
 *                                                                            *
 *   Convert BATs to Arrays                                                   *
 *                                                                            *
 ******************************************************************************/
template<typename weight_t>
static void trampoline(
        BAT* out_query_filter, BAT* out_query_weights,
        BAT* out_oid_paths, BAT* out_paths,
		BAT* q_perm, BAT* q_from, BAT* q_to,
		BAT* g_vertices, BAT* g_edges, BAT* g_weights
) {
	using graph_t = graph_t<weight_t>;
    using cost_t = cost_t<weight_t>;
    using query_t = query_t<weight_t>;

    // prepare the graph
    graph_t graph(
        /* size = */ BATcount(g_vertices),
        /* vertices = */ reinterpret_cast<vertex_t*>(g_vertices->theap.base),
        /* edges = */ reinterpret_cast<vertex_t*>(g_edges->theap.base),
        /* weights = */ g_weights ? reinterpret_cast<cost_t*>(g_weights->theap.base) : nullptr
    );

	// prepare the query params
    assert(BATcapacity(out_query_filter) >= BATcount(q_from));
    assert(out_query_weights == nullptr || BATcapacity(out_query_weights) == BATcapacity(out_query_filter));
	query_t query (
		/* in_perm = */ BATttype(q_perm) == TYPE_oid ? reinterpret_cast<vertex_t*>(q_perm->theap.base) : /* TYPE_void */ nullptr,
        /* in_src = */ reinterpret_cast<vertex_t*>(q_from->theap.base),
        /* in_dst = */ reinterpret_cast<vertex_t*>(q_to->theap.base),
        /* in_size = */ BATcount(q_from),
        /* out_src = */ reinterpret_cast<vertex_t*>(out_query_filter->theap.base),
        /* out_cost = */ out_query_weights ? reinterpret_cast<cost_t*>(out_query_weights->theap.base) : nullptr,
        /* out_capacity */ BATcapacity(out_query_filter)
	);

	// execute the operator
	execute(graph, query);

	// report the number of values joined
	BATsetcount(out_query_filter, query.count());
	if(out_query_weights)
		BATsetcount(out_query_weights, query.count());

	// FIXME
//	// almost done, we need to fill the shortest paths
//	size_t cur_oid = q_from->hseqbase;
//	size_t revlen_sz = paths.revlen.size();
//	size_t revpath_sz = paths.revpath.size();
//	assert(revlen_sz == query.size);
//	if ( BATextend(r_oid_paths, revpath_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
//	if ( BATextend(r_paths, revpath_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
//	oid* pos_oid = reinterpret_cast<oid*>(r_oid_paths->theap.base);
//	vertex_t* pos_val = reinterpret_cast<vertex_t*>(r_paths->theap.base);
//	vertex_t* revpath = paths.revpath.data();
//	for(size_t i = 0; i < revlen_sz; i++){
//		size_t pathlen_sz = paths.revlen[i];
//		for(size_t j = 0; j < pathlen_sz; j++, pos_oid++, pos_val++){
//			*pos_oid = cur_oid;
//			*pos_val = revpath[pathlen_sz -1 -j]; // revert the path
//		}
//		cur_oid++;
//		revpath += pathlen_sz;
//	}
//	BATsetcount(r_oid_paths, revpath_sz);
//	BATsetcount(r_paths, revpath_sz);
//	BATsetcount(q_weights, revlen_sz);
}

/******************************************************************************
 *                                                                            *
 *  Access the input BATs and instantiate the output BATs                     *
 *                                                                            *
 ******************************************************************************/

#if !defined(NDEBUG) /* debug only */
#define _CHECK_ERRLINE_EXPAND(LINE) #LINE
#define _CHECK_ERRLINE(LINE) _CHECK_ERRLINE_EXPAND(LINE)
#define _CHECK_ERRMSG(EXPR, ERROR) "[" __FILE__ ":" _CHECK_ERRLINE(__LINE__) "] " ERROR ": `" #EXPR "'"
#else /* release mode */
#define _CHECK_ERRMSG(EXPR, ERROR) ERROR
#endif
#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, _CHECK_ERRMSG( EXPR, ERROR ) ); goto error; }

#define BATfree( b ) if(b) { BBPunfix(b->batCacheid); }

//// True iff the given BAT is of type `type' or is empty, false otherwise.
//static bool BATempty_or_T(BAT* b, bte type){
//	return BATttype(b) == type || BATcount(b) == 0;
//}

static str
handle_request(
	   bat* id_out_filter_src,  bat* id_out_filter_dst,
	   bat* id_out_query_weight, bat* id_out_query_oid_path, bat* id_out_query_path,
	   bat* id_in_query_from_perm, bat* id_in_query_to_perm, bat* id_in_query_from_values, bat* id_in_query_to_values,
	   bat* id_in_vertices, bat* id_in_edges, bat* id_in_weights
	   ) noexcept {
	str rc = MAL_SUCCEED;
	const char* function_name = "graph.spfw";
	BAT* in_vertices = nullptr;
	BAT* in_edges = nullptr;
	BAT* in_weights = nullptr;
	BAT* in_query_from_perm = nullptr;
	BAT* in_query_to_perm = nullptr;
	BAT* in_query_from_values = nullptr;
	BAT* in_query_to_values = nullptr;
	BAT* out_query_filter_src = nullptr;
	BAT* out_query_filter_dst = nullptr;
	BAT* out_query_weights = nullptr;
	BAT* out_query_oid_path = nullptr;
	BAT* out_query_path = nullptr;
	const bool graph_has_weights = id_in_weights != nullptr && *id_in_weights != 0;
	const bool compute_path_cost = id_out_query_weight != nullptr;
	size_t output_max_size = 0;

	// retrieve the input bats
	in_vertices = BATdescriptor(*id_in_vertices);
	CHECK(in_vertices != nullptr, RUNTIME_OBJECT_MISSING);
	in_edges = BATdescriptor(*id_in_edges);
	CHECK(in_edges != nullptr, RUNTIME_OBJECT_MISSING);
	if(graph_has_weights){
	    in_weights = BATdescriptor(*id_in_weights);
        CHECK(in_weights != nullptr, RUNTIME_OBJECT_MISSING);
	}
	in_query_from_perm = BATdescriptor(*id_in_query_from_perm);
	CHECK(in_query_from_perm != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_to_perm = BATdescriptor(*id_in_query_to_perm);
	CHECK(in_query_to_perm != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_from_values = BATdescriptor(*id_in_query_from_values);
	CHECK(in_query_from_values != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_to_values = BATdescriptor(*id_in_query_to_values);
	CHECK(in_query_to_values != nullptr, RUNTIME_OBJECT_MISSING);

	// validate the properties of the input BATs
	CHECK(ATOMtype(BATttype(in_query_from_perm)) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(ATOMtype(BATttype(in_query_to_perm)) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_query_from_values) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_query_to_values) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_vertices) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_edges) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATcount(in_query_from_perm) == BATcount(in_query_from_values), ILLEGAL_ARGUMENT ": `src' columns size mismatch");
	CHECK(BATcount(in_query_to_perm) == BATcount(in_query_to_values), ILLEGAL_ARGUMENT ": `dest' columns size mismatch");

	// allocate the output vectors
	output_max_size = BATcount(in_query_from_values) * BATcount(in_query_to_values); // TODO this value might explode
	out_query_filter_src = COLnew(in_query_from_perm->hseqbase, TYPE_oid, output_max_size, TRANSIENT);
    CHECK(out_query_filter_src != nullptr, MAL_MALLOC_FAIL);
	out_query_filter_dst = COLnew(in_query_to_perm->hseqbase, TYPE_oid, output_max_size, TRANSIENT);
    CHECK(out_query_filter_dst != nullptr, MAL_MALLOC_FAIL);
	if(compute_path_cost){
        out_query_weights = COLnew(0, in_weights->ttype, output_max_size, TRANSIENT);
        CHECK(out_query_weights != nullptr, MAL_MALLOC_FAIL);
	}

	// TODO Disabled for the time being
//	out_query_oid_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0, TRANSIENT);
//	CHECK(out_query_oid_path != nullptr, MAL_MALLOC_FAIL);
//	out_query_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0	BATsetcount(out_query_to, query.count());, TRANSIENT);
//	CHECK(out_query_path != nullptr, MAL_MALLOC_FAIL);

	// DEBUG ONLY
	printf("<<graph.spfw>>\n"); fflush(stdout);
	bat_debug(in_query_from_perm);
	bat_debug(in_query_to_perm);
	bat_debug(in_query_from_values);
	bat_debug(in_query_to_values);
	bat_debug(in_vertices);
	bat_debug(in_edges);

	// if the graph is empty, there is not much to do
	if(BATcount(in_vertices) == 0 || BATcount(in_edges) == 0) goto success;

	try {
	    if(!graph_has_weights){
//            trampoline<void>(out_query_filter, out_query_weights, out_query_oid_path, out_query_path,
//                    in_query_perm, in_query_from, in_query_to, in_vertices, in_edges, in_weights);
	    } else {
	        CHECK(0, "NOT IMPLEMENTED");
	    }
	} catch (std::bad_alloc& b){
		CHECK(0, MAL_MALLOC_FAIL);
	} catch (...){  // no exception shall pass!
		CHECK(0, OPERATION_FAILED); // generic error
	}

success:
	// report the joined columns
	BBPkeepref(out_query_filter_src->batCacheid);
	*id_out_filter_src = out_query_filter_src->batCacheid;
	BBPkeepref(out_query_filter_dst->batCacheid);
	*id_out_filter_dst = out_query_filter_dst->batCacheid;

	// does the user want the final weight?
	if(compute_path_cost) {
		BBPkeepref(out_query_weights->batCacheid);
		*id_out_query_weight = out_query_weights->batCacheid;
	}

	// FIXME disabled for the time being
//	BBPkeepref(out_query_path->batCacheid);
//	BBPkeepref(out_query_oid_path->batCacheid);
//	*id_out_query_path = out_query_path->batCacheid;
//	*id_out_query_oid_path = out_query_oid_path->batCacheid;

	BATfree(in_query_from_perm);
	BATfree(in_query_to_perm);
	BATfree(in_query_from_values);
	BATfree(in_query_to_values);
	BATfree(in_vertices);
	BATfree(in_edges);
	BATfree(in_weights);

	return rc;
error:
	BATfree(in_query_from_perm);
	BATfree(in_query_to_perm);
	BATfree(in_query_from_values);
	BATfree(in_query_to_values);
	BATfree(in_vertices);
	BATfree(in_edges);
	BATfree(in_weights);
	BATfree(out_query_filter_src);
	BATfree(out_query_filter_dst);
	BATfree(out_query_weights);
	BATfree(out_query_oid_path);
	BATfree(out_query_path);

	return rc;
}

/******************************************************************************
 *                                                                            *
 *   MonetDB interface                                                        *
 *                                                                            *
 ******************************************************************************/
extern "C" {

// Only check which qfrom are connected to qto
str GRAPHconnected(
	bat* id_out_left, bat* id_out_right,
	bat* id_in_query_from_p, bat* id_in_query_to_p, bat* id_in_query_from_v, bat* id_in_query_to_v,
	bat* id_in_vertices, bat* id_in_edges
) noexcept {
	return handle_request(
			// Output parameters
			/* id_out_filter_src = */ id_out_left,
			/* id_out_filter_dst = */ id_out_right,
			/* id_out_query_weight = */ nullptr,
			/* id_out_query_oid_path = */ nullptr,
			/* id_out_query_path = */ nullptr,
			// Input parameters
			/* id_in_query_from_perm = */ id_in_query_from_p,
			/* id_in_query_to_perm = */ id_in_query_to_p,
			/* id_in_query_from_values = */ id_in_query_from_v,
			/* id_in_query_to_values = */ id_in_query_to_v,
			/* id_in_vertices = */ id_in_vertices,
			/* id_in_edges = */ id_in_edges,
			/* id_in_weights = */ nullptr
	);
}

} // extern "C"
