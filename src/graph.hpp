#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <cassert>
#include <cstddef> // std::size_t
#include <iostream>

namespace monetdb {

	// Edges description

	template<typename V, typename W = void>
	class Edge {
	public:
	    using vertex_t = V;
	    using cost_t = W;
	private:
		const vertex_t v;
		const cost_t  w;
	public:
		Edge(V dst, W cost) : v(dst), w(cost) { };

		vertex_t dest() const { return v; }
		cost_t cost() const { return w; }
	};

    template<typename V>
    class Edge<V, void> {
    public:
        using vertex_t = V;
        using cost_t = std::size_t;

    private:
        const vertex_t v;

    public:
        Edge(V dst) : v(dst) { };

        vertex_t dest() const { return v; }
        cost_t cost() const { return 1; }
    };

    // Graph representation

	template<typename V, typename W = void>
	class Graph {
	public:
	    using edge_t = Edge<V, W>;
	    using vertex_t = typename edge_t::vertex_t;
	    using cost_t = typename edge_t::cost_t;

	private:
		std::size_t vertex_count;
		vertex_t* __restrict vertices;
		vertex_t* __restrict edges;
		cost_t* __restrict weights;

		Graph(const Graph&) = delete;
		Graph& operator=(Graph&) = delete;

	public:
		template<typename type = W, bool dummy = true>
		class iterator_fwd{
			friend class Graph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			vertex_t* __restrict base_edges;
			cost_t* __restrict base_weights;

			iterator_fwd(V* e, W* w) noexcept : base_edges(e), base_weights(w) { }

			V* base() const noexcept { return base_edges; }
		public:
			// access the current element
		    edge_t operator*() const noexcept { return edge_t{*base_edges, *base_weights}; }

			// move forward
		    void operator++() noexcept { ++base_edges; ++base_weights; }

			// whatever the shortcut impl. is using
			bool operator== (const fwd_t& rhs) const noexcept { return base() == rhs.base(); }
			bool operator!= (const fwd_t& rhs) const noexcept { return base() != rhs.base(); }
		};

		template<bool dummy>
		class iterator_fwd<void, dummy>{
			friend class Graph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			vertex_t* __restrict base_edges;

			iterator_fwd(V* e) noexcept : base_edges(e) { }

			V* base() const noexcept { return base_edges; }
		public:
			// access the current element
		    edge_t operator*() const noexcept { return edge_t{*base_edges}; }

			// move forward
		    void operator++() noexcept { ++base_edges; }

			// whatever the shortcut impl. is using
			bool operator== (const fwd_t& rhs) const noexcept { return base() == rhs.base(); }
			bool operator!= (const fwd_t& rhs) const noexcept { return base() != rhs.base(); }
		};

		template<typename type = W, bool dummy = true>
		class iterator_make{
		public:
			friend class Graph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			V* base_edges;
			W* base_weights;
			V* end_edges;

			iterator_make(V* e, W* w, V* end_edges) noexcept : base_edges(e), base_weights(w), end_edges(end_edges){ }

		public:
			fwd_t begin() noexcept { return fwd_t(base_edges, base_weights); }
			fwd_t end() const noexcept { return fwd_t(end_edges, NULL); }
		};

		template<bool dummy>
		class iterator_make<void, dummy>{
		private:
			friend class Graph;
			using fwd_t = iterator_fwd<W, dummy>;

			V* base_edges;
			V* end_edges;

			iterator_make(V* e, void*, V* end_edges) noexcept : base_edges(e), end_edges(end_edges){ }

		public:
			fwd_t begin() noexcept { return fwd_t(base_edges); }
			const fwd_t end() const noexcept { return fwd_t(end_edges); }
		};



		Graph(std::size_t size, vertex_t* vertices, vertex_t* edges, cost_t* weights) noexcept :
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

		iterator_make<W> operator[] (vertex_t vertex_id) const noexcept {
			assert(vertex_id < size());

			std::size_t offset = vertex_id == 0 ? 0 : vertices[vertex_id -1];
			return iterator_make<W>(edges + offset, weights + offset, edges + vertices[vertex_id]);
		}

	};
}



#endif /* GRAPH_HPP_ */
