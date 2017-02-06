/*
 * sequential_dijkstra.hpp
 *
 *  Created on: 28 Sep 2016
 *      Author: Dean De Leo
 */

#ifndef ALGORITHM_SEQUENTIAL_DIJKSTRA_HPP_
#define ALGORITHM_SEQUENTIAL_DIJKSTRA_HPP_

#include "query.hpp"

namespace gr8 { namespace algorithm { namespace sequential {


class SequentialDijkstra{
public:
	SequentialDijkstra();

	void execute(Query& query, ShortestPath* sp, bool join_results);
};

} } } // gr8::algorithm::sequential



#endif /* ALGORITHM_SEQUENTIAL_DIJKSTRA_HPP_ */
