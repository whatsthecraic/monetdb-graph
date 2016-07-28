#include <cstddef> // std::size_t
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

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
 *   Dijkstra SSSD                                                            *
 *                                                                            *
 ******************************************************************************/
template<typename vertex_t>
struct Paths{
	std::vector<vertex_t> revpath;
	std::vector<std::size_t> revlen;
};

template<typename Queue, typename V, typename W>
static void dijkstra_sssd(graph::Graph<V, W>& G, V src, V dst, W& dist, Paths<V>& paths){
	Queue Q;
	const W inf = std::numeric_limits<W>::max();

	// Distance vector
	W D[G.size()];
	for(std::size_t i = 0; i < G.size(); i++){ D[i] = inf; }
	D[src] = 0;

	// Parents
	V P[G.size()];
	P[src] = src;

	while(!Q.empty()){
		auto root = Q.pop();

		if(root.d > D[root.v]) continue; // we already considered this node, ignore
		if(root.v == dst) break; // found

		// relax the edges
		for(const auto& e : G[root.v]){
			W td = D[root.v] + e.w;
			if(td < D[e.v]){
				D[e.v] = td;
				P[e.v] = root.v;
				Q.push(e.v, td);
			}
		}
	}

	// report the output path
	if(D[dst] < inf){
		size_t len = 0;
		for(V parent = P[dst]; parent != src; parent = P[dst]){
			paths.revpath.push_back(parent);
			len++;
		}
		paths.revlen.push_back(len);
	} else {
		paths.revlen.push_back(0);
	}

	dist = D[dst]; // output weight
}

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
static void spfw(Input<vertex_t, weight_t>& query, Input<vertex_t, weight_t>& graph, Paths<vertex_t>& paths){
	// Graph
	graph::Graph<vertex_t, weight_t> G(graph.size, graph.src, graph.dst, graph.weights);

	// Queue
	using Queue = graph::RadixHeap<vertex_t, weight_t>;

	// Perform Dijkstra for each query
	for(std::size_t i = 0; i < query.size; i++){
		dijkstra_sssd<Queue>(G, query.src[i], query.dst[i], query.weights[i], paths);
	}
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
	Paths<vertex_t> paths;

	// jump to the C++ operator
	spfw(query, graph, paths);

	// almost done, we need to fill the shortest paths
	size_t revlen_sz = paths.revlen.size();
	assert(revlen_sz == query.size);
	if ( BATextend(r_oid_paths, revlen_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
	if ( BATextend(r_paths, revlen_sz) != GDK_SUCCEED ){ throw std::bad_alloc(); }
	oid* pos_oid = reinterpret_cast<oid*>(r_oid_paths->theap.base);
	vertex_t* pos_val = reinterpret_cast<vertex_t*>(r_paths->theap.base);
	for(size_t i = 0; i < revlen_sz; i++){
		size_t pathlen_sz = paths.revlen[i];
		for(size_t j = 0; j < pathlen_sz; j++, pos_oid++, pos_val++){
			*pos_oid = i;
			*pos_val = paths.revpath[pathlen_sz -1 -j]; // revert the path
		}
	}
	BATsetcount(r_oid_paths, revlen_sz);
	BATsetcount(r_paths, revlen_sz);
}

/******************************************************************************
 *                                                                            *
 *   MonetDB interface                                                        *
 *                                                                            *
 ******************************************************************************/
#define CHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, ERROR); goto error; }


#define BATfree( b ) if(b) { BBPunfix(b->batCacheid); }

mal_export str
GRAPHspfw(bat* id_out_query_from, bat* id_out_query_to, bat* id_out_query_weight, bat* id_out_query_oid_path, bat* id_out_query_path,
		bat* id_in_query_from, bat* id_in_query_to, bat* id_in_vertices, bat* id_in_edges, bat* id_in_weights) noexcept {
	str rc = MAL_SUCCEED;
	const char* function_name = "graph.spfw";
	BAT* in_query_from = nullptr;
	BAT* in_query_to = nullptr;
	BAT* in_vertices = nullptr;
	BAT* in_edges = nullptr;
	BAT* in_weights = nullptr;
	BAT* out_query_weights = nullptr;
	BAT* out_query_oid_path = nullptr;
	BAT* out_query_path = nullptr;

	// retrieve the input bats
	in_query_from = BATdescriptor(*id_in_query_from);
	CHECK(in_query_from != nullptr, RUNTIME_OBJECT_MISSING);
	in_query_to = BATdescriptor(*id_in_query_to);
	CHECK(in_query_to != nullptr, RUNTIME_OBJECT_MISSING);
	in_vertices = BATdescriptor(*id_in_vertices);
	CHECK(in_vertices != nullptr, RUNTIME_OBJECT_MISSING);
	in_edges = BATdescriptor(*id_in_edges);
	CHECK(in_edges != nullptr, RUNTIME_OBJECT_MISSING);
	in_weights = BATdescriptor(*id_in_weights);
	CHECK(in_weights != nullptr, RUNTIME_OBJECT_MISSING);

	// validate input bats properties
	CHECK(BATttype(in_query_from) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_query_to) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_vertices) == TYPE_oid, ILLEGAL_ARGUMENT);
	CHECK(BATttype(in_edges) == TYPE_oid, ILLEGAL_ARGUMENT);

	// allocate the output vectors
	out_query_weights = COLnew(in_query_from->hseqbase, in_weights->ttype, BATcount(in_query_from), TRANSIENT);
	CHECK(out_query_weights != nullptr, MAL_MALLOC_FAIL);
	out_query_oid_path = COLnew(0 /* TO BE CHECKED */, TYPE_oid, 0, TRANSIENT);
	CHECK(out_query_oid_path != nullptr, MAL_MALLOC_FAIL);
	out_query_path = COLnew(0 /* TO BE CHECKED */, in_query_from->batCacheid, 0, TRANSIENT);
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


	BBPkeepref(*id_in_query_from);
	BBPkeepref(*id_in_query_to);
	BBPkeepref(out_query_weights->batCacheid);
	BBPkeepref(out_query_path->batCacheid);
	BBPkeepref(out_query_oid_path->batCacheid);
	*id_out_query_from = *id_in_query_from;
	*id_out_query_to = *id_in_query_to;
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
