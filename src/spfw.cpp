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
	if(query.empty()) return; // edge case

	SequentialDijkstra<vertex_t, W> algorithm(graph);
	algorithm(query);
}


/******************************************************************************
 *                                                                            *
 *   Convert BATs to Arrays                                                   *
 *                                                                            *
 ******************************************************************************/

template<typename T>
static T* convert2array(BAT* b){
	if (b == nullptr || BATttype(b) == TYPE_void) // edge case
		return nullptr;
	else
		return reinterpret_cast<T*>(b->theap.base);
}

// The arguments for the interface
namespace {
struct spfw_params {
    BAT *out_query_left, *out_query_right, *out_query_weights;
    BAT *out_oid_paths,  *out_paths;
	BAT *q_perm_left, *q_perm_right, *q_from, *q_to;
	BAT *g_vertices, *g_edges, *g_weights;
	bool is_filter_only, compute_cost;
};
}

template<typename weight_t>
static void trampoline(spfw_params& params) {
	using graph_t = graph_t<weight_t>;
    using cost_t = cost_t<weight_t>;
    using query_t = query_t<weight_t>;

    // prepare the graph
    graph_t graph(
        /* size = */ BATcount(params.g_vertices),
        /* vertices = */ convert2array<vertex_t>(params.g_vertices),
        /* edges = */ convert2array<vertex_t>(params.g_edges),
        /* weights = */ convert2array<cost_t>(params.g_weights)
    );

	// prepare the query params
    assert(BATcapacity(params.out_query_left) >= BATcount(params.q_from));
    assert(BATcapacity(params.out_query_left) == BATcapacity(params.out_query_right));
    assert(params.out_query_weights == nullptr || BATcapacity(params.out_query_weights) == BATcapacity(params.out_query_left));
	query_t query (
	    /* in_perm_left = */  convert2array<vertex_t>(params.q_perm_left),
	    /* in_perm_right = */ convert2array<vertex_t>(params.q_perm_right),
        /* in_src = */ convert2array<vertex_t>(params.q_from),
        /* in_dst = */ convert2array<vertex_t>(params.q_to),
        /* in_left_sz = */ BATcount(params.q_from),
		/* in_right_sz = */ BATcount(params.q_to),
		/* out_perm_left = */ convert2array<vertex_t>(params.out_query_left),
		/* out_perm_right = */ convert2array<vertex_t>(params.out_query_right),
		/* out_cost = */ params.compute_cost ? convert2array<cost_t>(params.out_query_weights) : nullptr,
        /* out_capacity */ BATcapacity(params.out_query_left)
	);
	query.set_filter_only(params.is_filter_only);

	// execute the operator
	execute(graph, query);

	// report the number of values joined
	BATsetcount(params.out_query_left, query.count());
	BATsetcount(params.out_query_right, query.count());
	if(params.out_query_weights)
		BATsetcount(params.out_query_weights, query.count());

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
	   bat* id_in_vertices, bat* id_in_edges, bat* id_in_weights,
	   bit* cross_product, bit* compute_shortest_path
	   ) noexcept {
	str rc = MAL_SUCCEED;
	const char* function_name = "graph.spfw";
	spfw_params params = {0};
	params.compute_cost = *compute_shortest_path;
	params.is_filter_only = !(*cross_product);
	const bool graph_has_weights = id_in_weights != nullptr && *id_in_weights != 0 && *id_in_weights != bat_nil;
	size_t output_max_size = 0;

	// retrieve the input bats
	params.g_vertices = BATdescriptor(*id_in_vertices);
	CHECK(params.g_vertices != nullptr, RUNTIME_OBJECT_MISSING);
	params.g_edges = BATdescriptor(*id_in_edges);
	CHECK(params.g_edges != nullptr, RUNTIME_OBJECT_MISSING);
	if(graph_has_weights){
	    params.g_weights = BATdescriptor(*id_in_weights);
        CHECK(params.g_weights != nullptr, RUNTIME_OBJECT_MISSING);
	}
	params.q_perm_left = BATdescriptor(*id_in_query_from_perm);
	CHECK(params.q_perm_left != nullptr, RUNTIME_OBJECT_MISSING);
	params.q_perm_right = BATdescriptor(*id_in_query_to_perm);
	CHECK(params.q_perm_right != nullptr, RUNTIME_OBJECT_MISSING);
	params.q_from = BATdescriptor(*id_in_query_from_values);
	CHECK(params.q_from != nullptr, RUNTIME_OBJECT_MISSING);
	params.q_to = BATdescriptor(*id_in_query_to_values);
	CHECK(params.q_to != nullptr, RUNTIME_OBJECT_MISSING);

	// validate the properties of the input BATs
	CHECK(ATOMtype(BATttype(params.q_perm_left)) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(ATOMtype(BATttype(params.q_perm_right)) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(params.q_from) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(params.q_to) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(params.g_vertices) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(params.g_vertices) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATcount(params.q_perm_left) == BATcount(params.q_from), ILLEGAL_ARGUMENT ": `src' columns size mismatch");
	CHECK(BATcount(params.q_perm_right) == BATcount(params.q_to), ILLEGAL_ARGUMENT ": `dest' columns size mismatch");
	if(params.is_filter_only){
		CHECK(BATcount(params.q_from) == BATcount(params.q_to),
				ILLEGAL_ARGUMENT ": filter operation with `src' and `dst' having different sizes");
	}

	// allocate the output vectors
	if(!params.is_filter_only){
		output_max_size = BATcount(params.q_from) * BATcount(params.q_to); // TODO this value might explode
	} else {
		output_max_size = BATcount(params.q_from);
	}
	params.out_query_left = COLnew(params.q_perm_left->hseqbase, TYPE_oid, output_max_size, TRANSIENT);
	CHECK(params.out_query_left != nullptr, MAL_MALLOC_FAIL);
	params.out_query_right = COLnew(params.q_perm_right->hseqbase, TYPE_oid, output_max_size, TRANSIENT);
	CHECK(params.out_query_right != nullptr, MAL_MALLOC_FAIL);
	if(params.compute_cost){
        params.out_query_weights = COLnew(0, params.g_weights ? params.g_weights->ttype : TYPE_lng, output_max_size, TRANSIENT);
        CHECK(params.out_query_weights != nullptr, MAL_MALLOC_FAIL);
	}

	// TODO Disabled for the time being
//	out_query_oid_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0, TRANSIENT);
//	CHECK(out_query_oid_path != nullptr, MAL_MALLOC_FAIL);
//	out_query_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0	BATsetcount(out_query_to, query.count());, TRANSIENT);
//	CHECK(out_query_path != nullptr, MAL_MALLOC_FAIL);

	// DEBUG ONLY
	DEBUG_DUMP("<<graph.spfw>>\n");
	DEBUG_DUMP(params.q_perm_left);
	DEBUG_DUMP(params.q_perm_right);
	DEBUG_DUMP(params.q_from);
	DEBUG_DUMP(params.q_to);
	DEBUG_DUMP(params.g_vertices);
	DEBUG_DUMP(params.g_edges);
	DEBUG_DUMP(params.g_weights);
	DEBUG_DUMP(params.is_filter_only);
	DEBUG_DUMP(params.compute_cost);

	// if the graph is empty, there is not much to do
	if(BATcount(params.g_vertices) == 0 || BATcount(params.g_edges) == 0) goto success;

	try {

	    if(!graph_has_weights){
	    	trampoline<void>(params);
	    } else {
	    	switch(params.g_weights->ttype){
	    	case TYPE_int:
	    		trampoline<std::make_unsigned<int>::type>(params);
	    		break;
	    	case TYPE_lng:
	    		trampoline<std::make_unsigned<lng>::type>(params);
	    		break;
	    	default:
	    		CHECK(0, "Type not implemented");
	    	}
	    }
		; // nop
	} catch (std::bad_alloc& b){
		CHECK(0, MAL_MALLOC_FAIL);
	} catch (...){  // no exception shall pass!
		CHECK(0, OPERATION_FAILED); // generic error
	}

success:
	// Debug only
	DEBUG_DUMP(params.out_query_left);
	DEBUG_DUMP(params.out_query_right);
	DEBUG_DUMP(params.out_query_weights);


	// report the joined columns
	BBPkeepref(params.out_query_left->batCacheid);
	*id_out_filter_src = params.out_query_left->batCacheid;
	BBPkeepref(params.out_query_right->batCacheid);
	*id_out_filter_dst = params.out_query_right->batCacheid;

	// does the user want the final weight?
	if(params.compute_cost) {
		BBPkeepref(params.out_query_weights->batCacheid);
		*id_out_query_weight = params.out_query_weights->batCacheid;
	} else {
		*id_out_query_weight = 0;
	}

	// TODO disabled for the time being
//	BBPkeepref(out_query_path->batCacheid);
//	BBPkeepref(out_query_oid_path->batCacheid);
//	*id_out_query_path = out_query_path->batCacheid;
//	*id_out_query_oid_path = out_query_oid_path->batCacheid;

	BATfree(params.q_perm_left);
	BATfree(params.q_perm_right);
	BATfree(params.q_from);
	BATfree(params.q_to);
	BATfree(params.g_vertices);
	BATfree(params.g_edges);
	BATfree(params.g_weights);

	return rc;
error:
	// input
	BATfree(params.q_perm_left);
	BATfree(params.q_perm_right);
	BATfree(params.q_from);
	BATfree(params.q_to);
	BATfree(params.g_vertices);
	BATfree(params.g_edges);
	BATfree(params.g_weights);
	// output
	BATfree(params.out_query_left);
	BATfree(params.out_query_right);
	BATfree(params.out_query_weights);
	BATfree(params.out_oid_paths);
	BATfree(params.out_paths);

	return rc;
}

/******************************************************************************
 *                                                                            *
 *   MonetDB interface                                                        *
 *                                                                            *
 ******************************************************************************/
extern "C" {

// spfw(qfperm:bat[:oid], qtperm:bat[:oid], qfval:bat[:oid], qtval:bat[:oid], V:bat[:oid], E:bat[:oid], W:bat[:any], crossproduct:bit, shortestpath:bit) (:bat[:oid], :bat[:oid], :bat[:any])
str GRAPHspfw1(
	bat* id_out_left, bat* id_out_right, bat* id_out_cost,
	bat* id_in_query_from_p, bat* id_in_query_to_p, bat* id_in_query_from_v, bat* id_in_query_to_v,
	bat* id_in_vertices, bat* id_in_edges, bat* id_in_weights,
	bit* cross_product, bit* compute_shortest_path
) noexcept {
	return handle_request(
			// Output parameters
			/* id_out_filter_src = */ id_out_left,
			/* id_out_filter_dst = */ id_out_right,
			/* id_out_query_weight = */ id_out_cost,
			/* id_out_query_oid_path = */ nullptr,
			/* id_out_query_path = */ nullptr,
			// Input parameters
			/* id_in_query_from_perm = */ id_in_query_from_p,
			/* id_in_query_to_perm = */ id_in_query_to_p,
			/* id_in_query_from_values = */ id_in_query_from_v,
			/* id_in_query_to_values = */ id_in_query_to_v,
			/* id_in_vertices = */ id_in_vertices,
			/* id_in_edges = */ id_in_edges,
			/* id_in_weights = */ id_in_weights,
			/* cross_product = */ cross_product,
			/* shortest_path = */ compute_shortest_path
	);
}

} // extern "C"
