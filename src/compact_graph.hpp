#ifndef COMPACT_GRAPH_HPP_
#define COMPACT_GRAPH_HPP_

#include <cassert>
#include <cstddef> // std::size_t
#include <iostream>

namespace gr8 {

	// Edges description
	template<typename V, typename W = void>
	class CompactEdge {
	public:
	    using vertex_t = V;
	    using cost_t = W;
	private:
		const vertex_t v;
		const cost_t  w;
		const vertex_t edge_id;
	public:
		CompactEdge(V dst, W cost, V id) : v(dst), w(cost), edge_id(id) { };

		vertex_t dest() const { return v; }
		cost_t cost() const { return w; }
		vertex_t id() const { return edge_id; }
	};

    template<typename V>
    class CompactEdge<V, void> {
    public:
        using vertex_t = V;
        using cost_t = std::size_t;

    private:
        const vertex_t v;
        const vertex_t edge_id;

    public:
        CompactEdge(V dst, V id) : v(dst), edge_id(id) { };

        vertex_t dest() const { return v; }
        cost_t cost() const { return 1; }
        vertex_t id() const { return edge_id; }
    };

    // Graph representation

	template<typename V, typename W = void>
	class CompactGraph {
	public:
	    using edge_t = CompactEdge<V, W>;
	    using vertex_t = typename edge_t::vertex_t;
	    using cost_t = typename edge_t::cost_t;

	private:
		std::size_t vertex_count;
		vertex_t* __restrict vertices;
		vertex_t* __restrict edges;
		cost_t* __restrict weights;
		vertex_t* __restrict edge_ids;

		CompactGraph(const CompactGraph&) = delete;
		CompactGraph& operator=(CompactGraph&) = delete;

	public:
		template<typename type = W, bool dummy = true>
		class iterator_fwd{
			friend class CompactGraph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			vertex_t* __restrict base_edges;
			cost_t* __restrict base_weights;
			vertex_t* __restrict base_ids;

			iterator_fwd(V* e, W* w, V* ids) noexcept : base_edges(e), base_weights(w), base_ids(ids) { }

			V* base() const noexcept { return base_edges; }
		public:
			// access the current element
		    edge_t operator*() const noexcept { return edge_t{*base_edges, *base_weights, *base_ids}; }

			// move forward
		    void operator++() noexcept { ++base_edges; ++base_weights; ++base_ids; }

			// whatever the shortcut impl. is using
			bool operator== (const fwd_t& rhs) const noexcept { return base() == rhs.base(); }
			bool operator!= (const fwd_t& rhs) const noexcept { return base() != rhs.base(); }
		};

		template<bool dummy>
		class iterator_fwd<void, dummy>{
			friend class CompactGraph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			vertex_t* __restrict base_edges;
			vertex_t* __restrict base_ids;

			iterator_fwd(V* e, V* ids) noexcept : base_edges(e), base_ids(ids) { }

			V* base() const noexcept { return base_edges; }
		public:
			// access the current element
		    edge_t operator*() const noexcept { return edge_t{*base_edges, *base_ids}; }

			// move forward
		    void operator++() noexcept { ++base_edges; ++base_ids; }

			// whatever the shortcut impl. is using
			bool operator== (const fwd_t& rhs) const noexcept { return base() == rhs.base(); }
			bool operator!= (const fwd_t& rhs) const noexcept { return base() != rhs.base(); }
		};

		template<typename type = W, bool dummy = true>
		class iterator_make{
		public:
			friend class CompactGraph;
			using fwd_t = iterator_fwd<W, dummy>;
		private:
			V* base_edges;
			W* base_weights;
			V* base_ids;
			V* end_edges;

			iterator_make(V* e, W* w, V* ids, V* end_edges) noexcept : base_edges(e), base_weights(w), base_ids(ids), end_edges(end_edges){ }

		public:
			fwd_t begin() noexcept { return fwd_t(base_edges, base_weights, base_ids); }
			fwd_t end() const noexcept { return fwd_t(end_edges, nullptr, nullptr); }
		};

		template<bool dummy>
		class iterator_make<void, dummy>{
		private:
			friend class CompactGraph;
			using fwd_t = iterator_fwd<W, dummy>;

			V* base_edges;
			V* base_ids;
			V* end_edges;

			iterator_make(V* e, void*, V* ids, V* end_edges) noexcept : base_edges(e), base_ids(ids), end_edges(end_edges){ }

		public:
			fwd_t begin() noexcept { return fwd_t(base_edges, base_ids); }
			const fwd_t end() const noexcept { return fwd_t(end_edges, base_ids); }
		};



		CompactGraph(std::size_t size, vertex_t* vertices, vertex_t* edges, cost_t* weights, vertex_t* ids) noexcept :
			vertex_count(size), vertices(vertices), edges(edges), weights(weights), edge_ids(ids){
		}

		~CompactGraph() noexcept { /* nop */ }


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
			return iterator_make<W>(edges + offset, weights + offset, edge_ids + offset, edges + vertices[vertex_id]);
		}

	};

} /*namespace gr8 */

#endif /* COMPACT_GRAPH_HPP_ */
