#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <fstream>
#include <random>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

// Function to generate a random integer in the range [min, max]
int random_int(int min, int max) {
    // static std::random_device rd;
    static std::mt19937 gen(1);
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

int main() {
    dimension_t num_dimensions = 9;
    max_tree_node = 512;
    int total_count = 10000;
    trie_depth = 6;
    max_depth = 32;
    no_dynamic_sizing = true;
    md_trie<9> mdtrie(max_depth, trie_depth, max_tree_node);
    bitmap::CompactPtrVector primary_key_to_treeblock_mapping(total_count);

    /* ---------- Initialization ------------ */
    std::vector<level_t> bit_widths = {32, 32, 32, 32, 32, 32, 32, 32, 32};
    std::vector<level_t> start_bits = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    create_level_to_num_children(bit_widths, start_bits, max_depth);

    /* ----------- INSERT ----------- */
    TimeStamp start = 0, cumulative = 0;
    int num_inserted = 0;
    vector<data_point<9>> points;
    points.reserve(total_count);

    for (int primary_key = 0; primary_key < total_count; primary_key++) {
        num_inserted++;
        if (num_inserted % (total_count / 10) == 0) {
            std::cout << "Inserting: " << num_inserted << " out of "
                      << total_count << std::endl;
        }
        data_point<9> point;
        // For lookup correctness checking.
        point.set_coordinate(0, primary_key);
        for (dimension_t i = 1; i < num_dimensions; ++i) {
            point.set_coordinate(i, random_int(1, (int)1 << 16));
        }
        start = GetTimestamp();
        mdtrie.insert_trie(&point, primary_key,
                           &primary_key_to_treeblock_mapping);
        cumulative += GetTimestamp() - start;
        points.push_back(std::move(point));
    }
    std::cout << "Insertion Latency per point: "
              << (float)cumulative / total_count << " us" << std::endl;

    /* ---------- LOOKUP ------------ */
    cumulative = 0;
    int num_lookup = 0;
    for (int primary_key = 0; primary_key < total_count; primary_key++) {
        num_lookup++;
        if (num_lookup % (total_count / 10) == 0) {
            std::cout << "Looking up: " << num_lookup << " out of "
                      << total_count << std::endl;
        }

        start = GetTimestamp();
        data_point<9> *pt =
            mdtrie.lookup_trie(primary_key, &primary_key_to_treeblock_mapping);
        if ((int)pt->get_coordinate(0) != primary_key) {
            std::cerr << "Wrong point retrieved!" << std::endl;
        }
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Lookup Latency per point: " << (float)cumulative / total_count
              << " us" << std::endl;

    /* ---------- RANGE QUERY ------------ */
    cumulative = 0;
    int num_queries = 3;
    std::cout << "Creating range queries that return every point. "
              << std::endl;
    for (int c = 0; c < num_queries; c++) {
        data_point<9> start_range;
        data_point<9> end_range;
        std::vector<int> found_points;
        /* We used the first coordinate to be the primary key. */
        start_range.set_coordinate(0, 0);
        end_range.set_coordinate(0, total_count);
        for (dimension_t i = 1; i < num_dimensions; i++) {
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, (int)(1 << 16));
        }

        start = GetTimestamp();
        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
        // Coordinates are flattened into one vector.
        if ((int)(found_points.size() / num_dimensions) != total_count) {
            std::cerr << "Wrong number of points found!" << std::endl;
            std::cerr << "found: "
                      << (int)(found_points.size() / num_dimensions)
                      << std::endl;
            std::cerr << "total count: " << total_count << std::endl;
        } else {
            std::cout << "Query - " << c << ", found: "
                      << (int)(found_points.size() / num_dimensions)
                      << ", expected: " << total_count << std::endl;
        }
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Per-query Latency: " << (cumulative / 1000.0) / num_queries
              << " ms" << std::endl;

    // Range Query Correctness
    for (int i = 0; i < total_count; i++) {
        std::vector<int> found_points;

        data_point<9> start_range = points[i];
        data_point<9> end_range = points[i];

        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
        // Coordinates are flattened into one vector.
        if ((int)(found_points.size() / num_dimensions) != 1) {
            std::cerr << "Wrong number of points found!" << std::endl;
            std::cerr << "found: "
                      << (int)(found_points.size() / num_dimensions)
                      << std::endl;
            std::cerr << "total count: " << 1 << std::endl;
        } else {
            std::cout << "Query - " << i << ", found: "
                      << (int)(found_points.size() / num_dimensions)
                      << ", expected: " << 1 << std::endl;
        }
    }
    return 0;
}