#include "trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>

#include <iostream>
#include <sstream>
#include <string>

#define NUM_DIMENSION 16

int main()
{

    std::string filename = "/users/blrr/data/full-climate_fever-sentences.csv";
    std::ifstream infile(filename);
    std::string line;
    int total_count = 24544931;

    dimension_t num_dimensions = NUM_DIMENSION;
    preorder_t max_tree_node = 128;
    level_t trie_depth = 1;
    level_t max_depth = 64;
    no_dynamic_sizing = true;
    bitmap::CompactPtrVector primary_key_to_treeblock_mapping(total_count);

    /* ---------- Initialization ------------ */
    std::vector<level_t> bit_widths = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};
    std::vector<level_t> start_bits = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    create_level_to_num_children(bit_widths, start_bits, max_depth);

    md_trie<NUM_DIMENSION> mdtrie(max_depth, trie_depth, max_tree_node);

    /* ----------- INSERT ----------- */
    TimeStamp start = 0, cumulative = 0;
    int num_inserted = 0;
    int primary_key = 0;

    for (int primary_key = 0; primary_key < total_count && std::getline(infile, line); primary_key++)
    {
        std::stringstream ss(line);

        num_inserted++;
        // if (num_inserted % (1000) == 0)
        // {
        std::cout << "Inserting: " << num_inserted << " out of " << total_count << std::endl;
        // }
        data_point<NUM_DIMENSION> point;
        // For lookup correctness checking.
        point.set_coordinate(0, primary_key);

        std::string cell;
        for (dimension_t i = 1; i < num_dimensions && std::getline(ss, cell, ','); ++i)
        {
            point.set_coordinate(i, std::stoull(cell));
        }

        start = GetTimestamp();
        mdtrie.insert_trie(&point, primary_key, &primary_key_to_treeblock_mapping);
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Insertion Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    /* ---------- LOOKUP ------------ */
    cumulative = 0;
    int num_lookup = 0;
    for (int primary_key = 0; primary_key < total_count; primary_key++)
    {
        num_lookup++;
        if (num_lookup % (total_count / 10) == 0)
        {
            std::cout << "Looking up: " << num_lookup << " out of " << total_count << std::endl;
        }

        start = GetTimestamp();
        data_point<NUM_DIMENSION> *pt = mdtrie.lookup_trie(primary_key, &primary_key_to_treeblock_mapping);
        if ((int)pt->get_coordinate(0) != primary_key)
        {
            std::cerr << "Wrong point retrieved!" << std::endl;
        }
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Lookup Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    /* ---------- RANGE QUERY ------------ */
    cumulative = 0;
    int num_queries = 3;
    std::cout << "Creating range queries that return every point. " << std::endl;
    for (int c = 0; c < num_queries; c++)
    {
        data_point<NUM_DIMENSION> start_range;
        data_point<NUM_DIMENSION> end_range;
        std::vector<uint64_t> found_points;
        /* We used the first coordinate to be the primary key. */
        start_range.set_coordinate(0, 0);
        end_range.set_coordinate(0, total_count);
        for (dimension_t i = 1; i < num_dimensions; i++)
        {
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, (int)(1 << 16));
        }

        start = GetTimestamp();
        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0, found_points);
        // Coordinates are flattened into one vector.
        if ((int)(found_points.size() / num_dimensions) != total_count)
        {
            std::cerr << "Wrong number of points found!" << std::endl;
            std::cerr << "found: " << (int)(found_points.size() / num_dimensions) << std::endl;
            std::cerr << "total count: " << total_count << std::endl;
        }
        else
        {
            std::cout << "Query - " << c << ", found: " << (int)(found_points.size() / num_dimensions) << ", expected: " << total_count << std::endl;
        }
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Per-query Latency: " << (cumulative / 1000.0) / num_queries << " ms" << std::endl;
    return 0;
}