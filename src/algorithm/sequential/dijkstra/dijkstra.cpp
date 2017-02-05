/*
 * dijkstra.cpp
 *
 *  Created on: 3 Feb 2017
 *      Author: Dean De Leo
 */


#include "dijkstra.hpp"
#include "dijkstra_impl.hpp"

#include <cassert>
#include <memory>

#include "errorhandling.hpp"

using namespace gr8;
using namespace gr8::algorithm::sequential;



template <typename V, typename W, typename G>
static void execute_dijkstra0(Query& query, G& graph, ShortestPath* sp, bool join_results){
	// initialize
	SequentialDijkstraImpl<V, W, G> impl{graph, sp};

	// fire the algorithm
	impl(query, join_results);
}

template <typename W>
static void execute_dijkstra(Query& query, ShortestPath* sp, bool join_results){
	GraphDescriptorCompact* gdc = dynamic_cast<GraphDescriptorCompact*>(query.graph.get());
	assert(gdc != nullptr);
	typedef CompactGraph<oid, W> graph_t;

	std::shared_ptr<graph_t> graph_ptr = gdc->instantiate<W>(sp->weights);
	graph_t& graph = *(graph_ptr.get());

	execute_dijkstra0<oid, W, graph_t>(query, graph, sp, join_results);
}

template <>
void execute_dijkstra<void>(Query& query, ShortestPath* sp, bool join_results){
	GraphDescriptorCompact* gdc = dynamic_cast<GraphDescriptorCompact*>(query.graph.get());
	assert(gdc != nullptr);
	typedef CompactGraph<oid> graph_t;

	std::shared_ptr<graph_t> graph_ptr = gdc->instantiate();
	graph_t& graph = *(graph_ptr.get());

	execute_dijkstra0<oid, void, graph_t>(query, graph, sp, join_results);
}

void SequentialDijkstra::execute(Query& query, ShortestPath* sp, bool join_results){

	if(!sp || sp->bfs()){
		execute_dijkstra<void>(query, sp, join_results);
	} else {
		auto bat_type = sp->weights.get()->ttype;
		switch(ATOMtype(bat_type)){
		case TYPE_int:
			execute_dijkstra<int>(query, sp, join_results);
			break;
		case TYPE_lng:
			execute_dijkstra<lng>(query, sp, join_results);
			break;
		case TYPE_oid:
			execute_dijkstra<oid>(query, sp, join_results);
			break;
		default:
			RAISE_ERROR("Type not supported: " << bat_type);
		}
	}

}
