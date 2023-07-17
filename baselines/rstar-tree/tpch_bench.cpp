#include <cinttypes>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <string.h>

#include <stdio.h>
#include "RStarTree.h"
#include <vector>
#include "./micro_common.hpp"

#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>

#define TPCH_SIZE 250000000 // 250M
#define SERVER_TO_SERVER_IN_NS 92
#define DIMENSION 9 + 1

int points_to_insert = 30000;
int points_to_lookup = 100;
bool dummy = true;
typedef RStarTree<bool, DIMENSION, 32, 64> RTree;

typedef RTree::BoundingBox BoundingBox;

typedef uint64_t dimension_t;

typedef unsigned long long int TimeStamp;

TimeStamp GetTimestamp() {
  struct timeval now;
  gettimeofday(&now, nullptr);

  return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
}

std::vector<int32_t>
parse_line_tpch(std::string line)
{

  std::vector<int32_t> point(DIMENSION - 1, 0);
  int index = -1;
  bool primary_key = true;
  std::string delim = ",";
  auto start = 0U;
  auto end = line.find(delim);

  // [id, QUANTITY, EXTENDEDPRICE, DISCOUNT, TAX, SHIPDATE, COMMITDATE,
  // RECEIPTDATE, TOTALPRICE, ORDERDATE]
  while (end != std::string::npos) {
    std::string substr = line.substr(start, end - start);
    start = end + 1;
    end = line.find(delim, start);

    if (primary_key) {
      primary_key = false;
      continue;
    }
    index++;
    point[index] = static_cast<int32_t>(std::stoul(substr));
  }
  index++;
  std::string substr = line.substr(start, end - start);
  point[index] = static_cast<int32_t>(std::stoul(substr));

  for (int i = 0; i < DIMENSION - 1; i++) {
    if (i >= 4 && i != 7) {
      point[i] -= 19000000;
    }
  }

  return point;
}

void flush_vector_to_file(std::vector<TimeStamp> vect, std::string filename){
    std::ofstream outFile(filename);
    for (const auto &e : vect) outFile << std::to_string(e) << "\n";
}

BoundingBox bounds(std::vector<int> coordinates)
{
	BoundingBox bb;
	
	for (uint8_t i = 0; i < DIMENSION; i++){
		bb.edges[i].first = coordinates[i];
		bb.edges[i].second = coordinates[i];
	}
	return bb;
}

BoundingBox bounds_range(std::vector<int> start, std::vector<int> end)
{
	BoundingBox bb;
	
	for (uint8_t i = 0; i < DIMENSION; i++){
		bb.edges[i].first = start[i];
		bb.edges[i].second = end[i];
	}
	return bb;
}

std::vector<RTree::Leaf *> leaf_list;
struct Visitor {
	int count;
	bool ContinueVisiting;
	
	Visitor() : count(0), ContinueVisiting(true) {};
	
	void operator()(RTree::Leaf * leaf) 
	{
		count++;
        leaf_list.push_back(leaf);
	}
};

void tpch()
{
	RTree tree;
	Visitor x;

    std::ifstream infile(TPCH_DATA_ADDR);

    TimeStamp start, diff;

    diff = 0;
    uint64_t n_points = 0;
    std::string line;
    std::vector<TimeStamp> insertion_latency_vect;

    /**
     * Insertion
     */

    while (std::getline(infile, line))
    {
		std::vector<int> coordinates = parse_line_tpch(line);
        coordinates.insert(coordinates.begin(), n_points);

        start = GetTimestamp();
        tree.Insert(dummy, bounds(coordinates));
        TimeStamp latency = GetTimestamp() - start;
        diff += latency;

		if (n_points % (TPCH_SIZE / 50) == 0)
			std::cout << "finished: " << n_points << std::endl;

        if (n_points > TPCH_SIZE - points_to_insert)
            insertion_latency_vect.push_back(latency + SERVER_TO_SERVER_IN_NS);

        if (n_points == TPCH_SIZE)
            break;

        if (n_points == 0 || n_points == points_to_insert / 2) {
            for (int j = 0; j < DIMENSION; j++) {
                std::cout << coordinates[j] << " ";
            }
            std::cout << std::endl;
        }

        n_points += 1;

    }
    std::cout << "Done! " << "Insertion Latency per point: " << (float) diff / n_points << std::endl;
    flush_vector_to_file(insertion_latency_vect, results_folder_addr + "/rstar-tree/tpch_insert");

    /**
     * Point Lookup
     */

    std::vector<int> max_values = {50, 10494950, 10, 8, 19981201, 19981031, 19981231, 58063825, 19980802};
    std::vector<int> min_values = {1, 90000, 0, 0, 19920102, 19920131, 19920103, 81300, 19920101};
 
    std::vector<TimeStamp> lookup_latency_vect;
    diff = 0; 
    for (int i = 0; i < points_to_lookup; i ++) 
    {
        std::vector<int> start_range(DIMENSION, 0);
        std::vector<int> end_range(DIMENSION, 0);

        start_range[0] = i;
        end_range[0] = i;

        for (int j = 0; j < DIMENSION - 1; j++) {
            start_range[j + 1] = (uint32_t) min_values[j];
            end_range[j + 1] = (uint32_t) max_values[j];
        }

        for (dimension_t j = 0; j < DIMENSION - 1; j++){
            if (j >= 4 && j != 7) {
                start_range[j + 1] -= 19000000;
                end_range[j + 1] -= 19000000;
            }
        }

        if (i == 0) {
            for (int j = 0; j < DIMENSION; j++) {
                std::cout << start_range[j] << " ";
            }
            std::cout << std::endl;
            for (int j = 0; j < DIMENSION; j++) {
                std::cout << end_range[j] << " ";
            }
            std::cout << std::endl;
        }


        BoundingBox bound = bounds_range(start_range, end_range);
        start = GetTimestamp();
        x = tree.Query(RTree::AcceptEnclosing(bound), Visitor());
        TimeStamp temp_diff =  GetTimestamp() - start; 
        diff += temp_diff;
        
        if (leaf_list.size() > 1) {
            std::cerr << i << "wrong points!" << leaf_list.size() << std::endl;
            exit(-1);
        }
        leaf_list.clear();
        std::cout << "i: " << i << " finished, leaf_list.size(): " << leaf_list.size() << std::endl;

        if (i <= points_to_lookup)
            lookup_latency_vect.push_back(temp_diff + SERVER_TO_SERVER_IN_NS);
    }
    std::cout << "Done! " << "Lookup Latency per point: " << (float) diff / points_to_lookup << std::endl;
    flush_vector_to_file(lookup_latency_vect, results_folder_addr + "/rstar-tree/tpch_lookup");

    /**
     * Range Search
     */

    std::ifstream file(TPCH_QUERY_ADDR);
    std::ofstream outfile(results_folder_addr + "/rstar-tree/tpch_query");

    for (int i = 0; i < 1000; i ++) 
    {
        std::vector<int> start_range(DIMENSION - 1, 0);
        std::vector<int> end_range(DIMENSION - 1, 0);

        for (int j = 0; j < DIMENSION - 1; j++) {
            start_range[j] = (uint32_t) min_values[j];
            end_range[j] = (uint32_t) max_values[j];
        }

        std::string line;
        std::getline(file, line);

        std::stringstream ss(line);
        // cout << line << endl;
        // Example: 0,-1,24,2,5,7,4,19943347,19950101
        while (ss.good()) {

            std::string index_str;
            std::getline(ss, index_str, ',');

            std::string start_range_str;
            std::getline(ss, start_range_str, ',');
            std::string end_range_str;
            std::getline(ss, end_range_str, ',');

            // cout << start_range_str << " " << end_range_str << endl;
            if (start_range_str != "-1") {
                start_range[static_cast<int32_t>(std::stoul(index_str))] = static_cast<int32_t>(std::stoul(start_range_str));
            }
            if (end_range_str != "-1") {
                end_range[static_cast<int32_t>(std::stoul(index_str))] = static_cast<int32_t>(std::stoul(end_range_str));
            }
        }

        for (dimension_t j = 0; j < DIMENSION - 1; j++){
            if (j >= 4 && j != 7) {
                start_range[j] -= 19000000;
                end_range[j] -= 19000000;
            }
        }

        start_range.insert(start_range.begin(), 0);
        end_range.insert(end_range.begin(), TPCH_SIZE);

        if (i == 0) {
            for (int j = 0; j < DIMENSION; j++) {
                std::cout << start_range[j] << " ";
            }
            std::cout << std::endl;
            for (int j = 0; j < DIMENSION; j++) {
                std::cout << end_range[j] << " ";
            }
            std::cout << std::endl;
        }


        BoundingBox bound = bounds_range(start_range, end_range);
        start = GetTimestamp();
        x = tree.Query(RTree::AcceptEnclosing(bound), Visitor());
        TimeStamp temp_diff =  GetTimestamp() - start; 

        outfile << "Query " << i << " end to end latency (ms): " << temp_diff / 1000 << ", found points count: " << leaf_list.size() << std::endl;
        leaf_list.clear();
    }

    return;
}

int main(){
    std::cout << "tpch......" << std::endl;
    tpch();
}