#ifndef GRAPH_HPP_
#define GRAPH_HPP_

#include <cassert>
#include <cstddef> // std::size_t
#include <iostream>

namespace monetdb {

    // Graph with weights associated to edges

	template<typename V, typename W = void>
	class Edge {
	public:
	    using vertex_t = V;
	    using weight_t = W;
	private:
		const vertex_t v;
		const weight_t  w;
	public:
		Edge(V dst, W cost) : v(dst), w(cost) { };

		vertex_t dest() const { return v; }
		weight_t dest() const { return w; }
	};

	template<typename V, typename W = void>
	class Graph {
	public:
	    using edge_t = typename Edge<V, W>;
	    using vertex_t = edge_t::vertex_t;
	    using weight_t = edge_t::weight_t;
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

		iterator_make operator[] (V vertex_id) const noexcept {
			assert(vertex_id < size());
			std::size_t offset = vertex_id == 0 ? 0 : vertices[vertex_id -1];
			return iterator_make(edges + offset, weights + offset, edges + vertices[vertex_id]);
		}

	};

	// Graph without weights associated to edges

    template<typename V>
    class Edge<V, void> {
    public:
        using vertex_t = V;
        using weight_t = std::size_t;

    private:
        const vertex_t v;

    public:
        Edge(V dst) : v(dst) { };

        vertex_t dest() const { return v; }
        weight_t cost() const { return 1; }
    };

    template<typename V>
    class Graph<V, void> {
    public:
        using edge_t = typename Edge<V>;
        using vertex_t = edge_t::vertex_t;
        using weight_t = edge_t::weight_t;
        class iterator_fwd;
        class iterator_make;

    private:
        std::size_t vertex_count;
        V* __restrict vertices;
        V* __restrict edges;

        Graph(const Graph&) = delete;
        Graph& operator=(Graph&) = delete;

    public:
        class iterator_fwd{
            friend class Graph;
        private:
            V* __restrict base_edges;

            iterator_fwd(V* e) noexcept : base_edges(e){ }

            V* base() const noexcept { return base_edges; }
        public:
            // access the current element
            edge_t operator*() const noexcept { return edge_t{*base_edges}; }

            // move forward
            void operator++() noexcept { ++base_edges; }

            // whatever the shortcut impl. is using
            bool operator== (const iterator_fwd& rhs) const noexcept { return base() == rhs.base(); }
            bool operator!= (const iterator_fwd& rhs) const noexcept { return base() != rhs.base(); }
        };

        class iterator_make{
        private:
            friend class Graph;
            V* base_edges;
            V* end_edges;

            iterator_make(V* e, V* end_edges) noexcept : base_edges(e), end_edges(end_edges){ }

        public:
            iterator_fwd begin() noexcept { return iterator_fwd(base_edges); }
            const iterator_fwd end() const noexcept { return iterator_fwd(end_edges); }
        };


        Graph(std::size_t size, V* vertices, V* edges, void*) noexcept :
            vertex_count(size), vertices(vertices), edges(edges){

        }

        ~Graph() noexcept { /* nop */ }


        std::size_t num_vertices() const noexcept {
            return vertex_count;
        }
        std::size_t size() const noexcept { return num_vertices(); } // alias

        std::size_t num_edges() const noexcept {
            return vertex_count== 0 ? 0 : vertices[vertex_count -1];
        }

        iterator_make operator[] (V vertex_id) const noexcept {
            assert(vertex_id < size());
            std::size_t offset = vertex_id == 0 ? 0 : vertices[vertex_id -1];
            return iterator_make(edges + offset, edges + vertices[vertex_id]);
        }

    };
}



#endif /* GRAPH_HPP_ */
