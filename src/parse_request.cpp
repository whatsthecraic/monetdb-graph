/*
 * parse_request.cpp
 *
 *  Created on: 5 Feb 2017
 *      Author: Dean De Leo
 */
#include "parse_request.hpp"

#include <cstdlib> // getenv
#include <iostream> // cout
#include <string>

#include "third-party/tinyxml2.hpp"

using namespace std;
using namespace tinyxml2;

namespace gr8 {

#define ERROR(message) RAISE_EXCEPTION(ParseError, message)
#define CHECK(condition, message) if(!(condition)) ERROR(message)


namespace {
	struct ParseExpectedColumn {
		const char* name; int pos; bool set; bool required;
		ParseExpectedColumn(const char* name, bool required = true) : name(name), pos(-1), set(false), required(required) {}
	};
}

static void parse_column(XMLElement* e, string& out_name, int& out_pos){
	bool name_set = false;
	bool pos_set = false;

	auto attribute = e->FirstAttribute();
	while(e != nullptr){
		string key = attribute->Name();
		if(key == "name"){
			out_name = attribute->Value();
			name_set = true;
		} else if (key == "pos") {
			out_pos = attribute->IntValue();
			pos_set = true;
		}

		if(name_set && pos_set) break;
		attribute = attribute->Next();
	}

	CHECK(name_set && pos_set, "Column: missing attribute 'name' or 'pos'");
}

static void parse_columns(XMLElement* parent, ParseExpectedColumn* columns) {
	CHECK(parent != nullptr, "INTERNAL ERROR: Argument 'parent' is null");
	CHECK(columns != nullptr, "INTERNAL ERROR: Empty argument 'columns'");

	XMLNode* base_node = parent->FirstChild();
	CHECK(base_node != nullptr, "Empty node, element: " << parent->Name());
	do {
		XMLElement* e = base_node->ToElement();
		if(e == nullptr) continue;
		string tag = e->Name();
		CHECK(tag == "column", "Unexpected element inside <" << parent->Name() << ">: " << tag);

		string name; int pos;
		parse_column(e, name, pos);

		ParseExpectedColumn* column_descr = columns;
		bool found = false;
		while(!found && column_descr->name){
			if(name == column_descr->name){
				found = true;
			} else {
				column_descr++;
			}
		}

		CHECK(found, "Invalid column inside <" << parent->Name() << ">: " << name);
		CHECK(!column_descr->set, "Duplicate entry for " << parent->Name() << "/" << name);
		column_descr->pos = pos;
		column_descr->set = true;
	} while ( (base_node = base_node->NextSibling() ) != nullptr );


	{ // Check all columns have been set
		ParseExpectedColumn* column_descr = columns;
		do {
			CHECK(!column_descr->required || column_descr->set, "Missing entry for " << parent->Name() << "/" << column_descr->name);
			column_descr++;
		} while (column_descr->name);
	}
}


static void parse_shortest_path(Query& query, MalStkPtr stackPtr, InstrPtr instrPtr, XMLElement* xml_shortest_path){
	CHECK(xml_shortest_path != nullptr, "XML ERROR: the node is null");
	string tag = xml_shortest_path->Name();
	CHECK(tag == "shortest_path", "subexpr/ Invalid node: " << tag);

	auto get_arg = [stackPtr, instrPtr](int index) {
		CHECK(index >= 0 && index < instrPtr->argc, "Invalid argument position: " << index);
		return (bat*) getArgReference(stackPtr, instrPtr, index);
	};

	int pos_in_weights = -1, pos_out_cost = -1, pos_out_path = -1;

	XMLNode* base_node = xml_shortest_path->FirstChild();
	do {
		XMLElement* e = base_node->ToElement();
		if(!e) continue;
		string tag = e->Name();
		if(tag == "column"){
			string name; int pos;
			parse_column(e, name, pos);
			if(name == "in_weights"){
				pos_in_weights = pos;
			} else if (name == "out_cost"){
				pos_out_cost = pos;
			} else if (name == "out_path"){
				pos_out_path = pos;
			} else {
				ERROR("Invalid column: " << name);
			}
		} else {
			ERROR("Invalid tag: " << tag);
		}
	} while ( (base_node = base_node->NextSibling()) != nullptr );

	CHECK(pos_out_cost != -1, "Missing required column 'output'");

	BatHandle weights;
	if(pos_in_weights != -1){
		weights = get_arg(pos_in_weights);
	}

	query.request_shortest_path(move(weights), pos_out_cost, pos_out_path);
}


void parse_request(Query& query, MalStkPtr stackPtr, InstrPtr instrPtr) {
	const char* request = *((const char**) getArgReference(stackPtr, instrPtr, instrPtr->retc));
	auto get_arg = [stackPtr, instrPtr](int index) {
		CHECK(index >= 0 && index < instrPtr->argc, "Invalid argument position: " << index);
//		bat* bb = (bat*) getArgReference(stackPtr, instrPtr, index);
//		cout << "[get_arg] index: " << index << ", value: " << ((bb == nullptr) ? -2 : *bb) << endl;
		return (bat*) getArgReference(stackPtr, instrPtr, index);
	};

	{ // print to stdout the request ?
		char* env_debug = getenv("GRAPH_DUMP_PARSER");
		if(env_debug != nullptr && (strcmp(env_debug, "1") == 0 || strcmp(env_debug, "true") == 0)){
			cout << request << endl;;
		}
	}

	// default attributes
	GraphDescriptorType graph_type = e_graph_columns;

	// parse the request
	XMLDocument xml_document(true, Whitespace::COLLAPSE_WHITESPACE);
	XMLError xml_rc = xml_document.Parse(request);
	CHECK(xml_rc == XML_SUCCESS, "Cannot parse the given text:\n" << request << "\n>>XML Error: " << XMLDocument::ErrorIDToName(xml_rc) << " (" << xml_rc << ")");
	auto xml_root = xml_document.RootElement();
	CHECK(xml_root != nullptr, "Empty XML document");
	auto base_node = xml_root->FirstChild();
	CHECK(base_node != nullptr, "XML: There are no nodes after the root");
	do {
		XMLElement* e = base_node->ToElement();
		if(!e) continue;
		string tag = e->Name();

		// operation kind
		if(tag == "operation"){
			string operation = e->GetText();
			if(operation == "join"){
				query.operation = op_join;
			} else if (operation == "filter") {
				query.operation = op_filter;
			} else {
				ERROR("Invalid operation: " << operation);
			}
		}

		// input columns
		else if (tag == "input"){
			enum { cl = 0, cr, src, dst };
			ParseExpectedColumn columns[] = { {"candidates_left"} , {"candidates_right", /*required=*/false}, {"src"}, {"dst"}, {nullptr} };
			parse_columns(e, columns);

			query.candidates_left = get_arg(columns[cl].pos);
			if(columns[cr].set)
				query.candidates_right = get_arg(columns[cr].pos);
			query.query_src = get_arg(columns[src].pos);
			query.query_dst = get_arg(columns[dst].pos);
		}

		// graph
		else if (tag == "graph"){
			// parse the attributes

			// graph type
//			auto attr_graph_type = e->FindAttribute("type");
//			if(attr_graph_type){
//				string graph_type_str = attr_graph_type->Value();
//				if(graph_type == "columns"){
//					graph_type = e_graph_columns;
//				} else if (graph_type == "compact") {
//					graph_type = e_graph_compact;
//				}
//			}

			// parse the children
			// e_graph_columns
			switch(graph_type){
			case e_graph_columns: {
				enum { i_src = 0, i_dst };
				ParseExpectedColumn columns[] = { {"src"} , {"dst"}, {nullptr} };
				parse_columns(e, columns);

				BatHandle edges_src{get_arg(columns[i_src].pos)};
				BatHandle edges_dst{get_arg(columns[i_dst].pos)};
				query.graph.reset( new GraphDescriptorColumns(move(edges_src), move(edges_dst)) );
			} break;
			case e_graph_compact: {
				enum { i_src = 0, i_dst, i_perm, i_count };
				ParseExpectedColumn columns[] = { {"src"} , {"dst"}, {"id"}, {"count"}, {nullptr} };
				parse_columns(e, columns);

				BatHandle edges_src{get_arg(columns[i_src].pos)};
				BatHandle edges_dst{get_arg(columns[i_dst].pos)};
				BatHandle edges_id{get_arg(columns[i_perm].pos)};
				lng* count = (lng*) getArgReference(stackPtr, instrPtr, columns[i_count].pos);
				CHECK(count != nullptr, "Argument 'count' is null");
				query.graph.reset( new GraphDescriptorCompact{move(edges_src), move(edges_dst), move(edges_id), (size_t) *count});
			} break;
			default:
				ERROR("Invalid graph type: " << graph_type);
			}
		}

		// shortest paths
		else if (tag == "subexpr"){
			XMLElement* parent = e;
			XMLNode* base_node = parent->FirstChild();
			if(!base_node) continue; // empty
			do {
				XMLElement* e = base_node->ToElement();
				if(!e) continue;
				parse_shortest_path(query, stackPtr, instrPtr, e);
			} while ( (base_node = base_node->NextSibling()) != nullptr );
		}


		else if(tag == "output"){
			enum { i_left = 0, i_right };
			ParseExpectedColumn columns[] = { {"candidates_left"} , {"candidates_right", false}, {nullptr} };
			parse_columns(e, columns);

			query.set_pos_output_left(columns[i_left].pos);
			query.set_pos_output_right(columns[i_right].pos);
		}

		else {
			ERROR("Invalid element: " << tag);
		}
	} while ( (base_node = base_node->NextSibling()) != nullptr );
}

} /*namespace gr8*/


