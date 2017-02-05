/*
 * graph_descriptor.hpp
 *
 *  Created on: 3 Feb 2017
 *      Author: Dean De Leo
 */

#ifndef GRAPH_DESCRIPTOR_HPP_
#define GRAPH_DESCRIPTOR_HPP_

#include "bat_handle.hpp"
#include "compact_graph.hpp"

#include <memory>

namespace gr8 {

// Kind of graph contained
enum GraphDescriptorType {
	e_graph_columns, // bare columns
	e_graph_compact, // compact form using the prefix form
};

// Base class
class GraphDescriptor {
public:
	GraphDescriptor();
	virtual ~GraphDescriptor();

	virtual GraphDescriptorType get_type() const = 0;
};

// Entry class from MonetDB codegen
class GraphDescriptorColumns : public GraphDescriptor {
public:
	BatHandle edge_src;
	BatHandle edge_dst;

	GraphDescriptorColumns(BatHandle&& edge_src, BatHandle&& edge_dst);
	~GraphDescriptorColumns();

	GraphDescriptorType get_type() const;
};

// Compact representation
class GraphDescriptorCompact : public GraphDescriptor {
public:
	BatHandle edge_src;
	BatHandle edge_dst;
	std::size_t vertex_count;

	GraphDescriptorCompact(BatHandle&& edge_src, BatHandle&& edge_dst, std::size_t vertex_count);
	~GraphDescriptorCompact();

	GraphDescriptorType get_type() const;

	std::shared_ptr<CompactGraph<oid>> instantiate() {
		typedef std::shared_ptr<CompactGraph<oid>> pointer_t;

		// std::size_t size, vertex_t* vertices, vertex_t* edges, cost_t* weights
		return pointer_t { new CompactGraph<oid>(vertex_count, edge_src.array<oid>(), edge_dst.array<oid>(), nullptr) };
	}

	template <typename W>
	std::shared_ptr<CompactGraph<oid, W>> instantiate(BatHandle& weights) {
		typedef std::shared_ptr<CompactGraph<oid, W>> pointer_t;

		// std::size_t size, vertex_t* vertices, vertex_t* edges, cost_t* weights
		return pointer_t { new CompactGraph<oid, W>(vertex_count, edge_src.array<oid>(), edge_dst.array<oid>(), weights.array<W>()) };
	}

};

} /* namespace gr8 */



#endif /* GRAPH_DESCRIPTOR_HPP_ */
