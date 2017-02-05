/*
 * prepare_graph.hpp
 *
 *  Created on: 3 Feb 2017
 *      Author: Dean De Leo
 */

#ifndef PREPARE_GRAPH_HPP_
#define PREPARE_GRAPH_HPP_

#include "query.hpp"

namespace gr8 {

void prepare_graph(Query& q);

void reorder_computations(Query& q);

}



#endif /* PREPARE_GRAPH_HPP_ */
