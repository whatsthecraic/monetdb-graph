#include <cstddef> // std::size_t
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "algorithms.hpp"
#include "graph.hpp"
#include "radixheap.hpp"


#undef ATOMIC_FLAG_INIT // make MonetDB happy
// MonetDB include files
extern "C" {
#include "monetdb_config.h"
#include "mal_exception.h"
}
#undef throw // this is a keyword in C++

using std::size_t; // omnipresent

/******************************************************************************
 *                                                                            *
 *   C++ operator                                                             *
 *                                                                            *
 ******************************************************************************/

template<typename vertex_t, typename weight_t>
struct Input {
	vertex_t* src;
	vertex_t* dst;
	weight_t* weights;
	std::size_t size;
};

template<typename vertex_t, typename weight_t>
static void spfw(Input<vertex_t, weight_t>& query, Input<vertex_t, weight_t>& graph, monetdb::Path<vertex_t>& paths){
	// Graph
	monetdb::Graph<vertex_t, weight_t> G(graph.size, graph.src, graph.dst, graph.weights);

	// Queue
	using Queue = monetdb::RadixHeap<vertex_t, weight_t>;

	// Perform Dijkstra for each query
	monetdb::SequentialDijkstra<Queue, vertex_t, weight_t> algorithm(G);
	algorithm.msmd(query.src, query.dst, query.weights, query.size, paths);
}


/******************************************************************************
 *                                                                            *
 *   Convert BATs to Arrays                                                   *
 *                                                                            *
 ******************************************************************************/

template<typename vertex_t, typename weight_t>
static void trampoline(BAT* q_from, BAT* q_to, BAT* q_weights,
		BAT* r_oid_paths, BAT* r_paths,
		BAT* g_vertices, BAT* g_edges, BAT* g_weights) {
	// prepare the query params
	Input<vertex_t, weight_t> query = {
		 reinterpret_cast<vertex_t*>(q_from->theap.base),
		 reinterpret_cast<vertex_t*>(q_to->theap.base),
		 reinterpret_cast<weight_t*>(q_weights->theap.base),
		 BATcount(q_from)
	};

	// prepare the graph params
	Input<vertex_t, weight_t> graph = {
		reinterpret_cast<vertex_t*>(g_vertices->theap.base),
		reinterpret_cast<vertex_t*>(g_edges->theap.base),
		reinterpret_cast<weight_t*>(g_weights->theap.base),
		BATcount(g_vertices)
	};

	// prepare the paths
	monetdb::Path<vertex_t> paths;

	// jump to the C++ operator
	spfw(query, graph, paths);

	// almost done, we need to fill the shortest paths
	size_t revlen_sz = paths.revlen.size();
	size_t revpath_sz = paths.revpath.size();
	assert(revlen_sz == query.size);
	if ( BATextend(r_oid_paths, revpath_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
	if ( BATextend(r_paths, revpath_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
	oid* pos_oid = reinterpret_cast<oid*>(r_oid_paths->theap.base);
	vertex_t* pos_val = reinterpret_cast<vertex_t*>(r_paths->theap.base);
	vertex_t* revpath = paths.revpath.data();
	for(size_t i = 0; i < revlen_sz; i++){
		size_t pathlen_sz = paths.revlen[i];
		for(size_t j = 0; j < pathlen_sz; j++, pos_oid++, pos_val++){
			*pos_oid = i;
			*pos_val = revpath[pathlen_sz -1 -j]; // revert the path
		}
		revpath += pathlen_sz;
	}
	BATsetcount(r_oid_paths, revpath_sz);
	BATsetcount(r_paths, revpath_sz);
	BATsetcount(q_weights, revlen_sz);
}

/******************************************************************************
 *                                                                            *
 *   MonetDB interface                                                        *
 *                                                                            *
 ******************************************************************************/
extern "C" {

#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, ERROR); goto error; }


#define BATfree( b ) if(b) { BBPunfix(b->batCacheid); }

mal_export str
GRAPHspfw(bat* id_out_query_weight, bat* id_out_query_oid_path, bat* id_out_query_path,
		bat* id_in_vertices, bat* id_in_edges, bat* id_in_weights, bat* id_in_query_from, bat* id_in_query_to) noexcept {
	str rc = MAL_SUCCEED;
	const char* function_name = "graph.spfw";
	BAT* in_vertices = nullptr;
	BAT* in_edges = nullptr;
	BAT* in_weights = nullptr;
	BAT* in_query_from = nullptr;
	BAT* in_query_to = nullptr;
	BAT* out_query_weights = nullptr;
	BAT* out_query_oid_path = nullptr;
	BAT* out_query_path = nullptr;

	// retrieve the input bats
	in_vertices = BATdescriptor(*id_in_vertices);
	CHECK(in_vertices != nullptr, RUNTIME_OBJECT_MISSING);
	in_edges = BATdescriptor(*id_in_edges);
	CHECK(in_edges != nullptr, RUNTIME_OBJECT_MISSING);
	in_weights = BATdescriptor(*id_in_weights);
	CHECK(in_weights != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_from = BATdescriptor(*id_in_query_from);
	CHECK(in_query_from != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_to = BATdescriptor(*id_in_query_to);
	CHECK(in_query_to != nullptr, RUNTIME_OBJECT_MISSING);

	// validate input bats properties
	CHECK(BATttype(in_query_from) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_query_to) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_vertices) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_edges) == TYPE_oid, ILLEGAL_ARGUMENT);

	// allocate the output vectors
	out_query_weights = COLnew(in_query_from->hseqbase, in_weights->ttype, BATcount(in_query_from), TRANSIENT);
	CHECK(out_query_weights != nullptr, MAL_MALLOC_FAIL);
	out_query_oid_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0, TRANSIENT);
	CHECK(out_query_oid_path != nullptr, MAL_MALLOC_FAIL);
	out_query_path = COLnew(in_query_from->hseqbase, TYPE_oid, 0, TRANSIENT);
	CHECK(out_query_path != nullptr, MAL_MALLOC_FAIL);

	try {
		// assume vertices have type oid
		switch(BATttype(in_weights)){
		case TYPE_lng: // weights are long
			trampoline<oid, lng>(in_query_from, in_query_to, out_query_weights, out_query_oid_path, out_query_path,
					in_vertices, in_edges, in_weights);
			break;
		}
	} catch (std::bad_alloc& b){
		CHECK(0, MAL_MALLOC_FAIL);
	} catch (...){  // no exception shall pass!
		CHECK(0, OPERATION_FAILED); // generic error
	}


	BBPkeepref(*id_in_query_from); // TODO to be checked
	BBPkeepref(*id_in_query_to);
	BBPkeepref(out_query_weights->batCacheid);
	BBPkeepref(out_query_path->batCacheid);
	BBPkeepref(out_query_oid_path->batCacheid);
	*id_out_query_weight = out_query_weights->batCacheid;
	*id_out_query_path = out_query_path->batCacheid;
	*id_out_query_oid_path = out_query_oid_path->batCacheid;

	BATfree(in_vertices);
	BATfree(in_edges);
	BATfree(in_weights);

	return rc;
error:
	BATfree(in_query_from);
	BATfree(in_query_to);
	BATfree(in_vertices);
	BATfree(in_edges);
	BATfree(in_weights);
	BATfree(out_query_weights);
	BATfree(out_query_oid_path);
	BATfree(out_query_path);

	return rc;
}

} // extern "C"
