#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <cassert>
#include <cstddef> // std::size_t
#include <iostream>

namespace graph {

	template<typename V, typename W>
	struct Edge {
		V v;
		W w;
	};

	template<typename V, typename W>
	class Graph {
	public:
		typedef Edge<V, W> edge_t;
		class iterator_fwd;
		class iterator_make;

	private:
		std::size_t vertex_count;
		V* __restrict vertices;
		V* __restrict edges;
		W* __restrict weights;

		Graph(const Graph&) = delete;
		Graph& operator=(Graph&) = delete;

	public:
		class iterator_fwd{
			friend class Graph;
		private:
			V* __restrict base_edges;
			W* __restrict base_weights;

			iterator_fwd(V* e, W* w) noexcept : base_edges(e), base_weights(w) { }

			V* base() const noexcept { return base_edges; }
		public:
			// access the current element
		    edge_t operator*() const noexcept { return edge_t{*base_edges, *base_weights}; }

			// move forward
		    void operator++() noexcept { ++base_edges; ++base_weights; }

			// whatever the shortcut impl. is using
			bool operator== (const iterator_fwd& rhs) const noexcept { return base() == rhs.base(); }
			bool operator!= (const iterator_fwd& rhs) const noexcept { return base() != rhs.base(); }
		};

		class iterator_make{
		private:
			friend class Graph;
			V* base_edges;
			W* base_weights;
			V* end_edges;

			iterator_make(V* e, W* w, V* end_edges) noexcept : base_edges(e), base_weights(w), end_edges(end_edges){ }

		public:
			iterator_fwd begin() noexcept { return iterator_fwd(base_edges, base_weights); }
			const iterator_fwd end() const noexcept { return iterator_fwd(end_edges, NULL); }
		};


		Graph(std::size_t size, V* vertices, V* edges, W* weights) noexcept :
			vertex_count(size), vertices(vertices), edges(edges), weights(weights){

		}

		~Graph() noexcept { /* nop */ }


		std::size_t num_vertices() const noexcept {
			return vertex_count;
		}
		std::size_t size() const noexcept { return num_vertices(); } // alias

		std::size_t num_edges() const noexcept {
			return vertex_count== 0 ? 0 : vertices[vertex_count -1];
		}

		iterator_make operator[] (V vertex_id) noexcept {
			assert(vertex_id < size());
			std::size_t offset = vertex_id == 0 ? 0 : vertices[vertex_id -1];
			return iterator_make(edges + offset, weights + offset, edges + vertices[vertex_id]);
		}

	};
}



#endif /* GRAPH_HPP_ */