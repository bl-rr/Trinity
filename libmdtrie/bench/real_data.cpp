#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "clocking_utils.hpp"

#define NUM_DIMENSIONS 8

std::vector<int32_t> parse_line_real_data(const std::string &line) {
    std::vector<int32_t> result;
    std::stringstream ss(line);
    std::string value;

    while (std::getline(ss, value, ',')) {
        try {
            result.push_back(std::stoul(value)); // Convert to int32_t
        } catch (const std::exception &e) {
            std::cerr << "Error parsing integer: " << value << " - " << e.what()
                      << std::endl;
        }
    }

    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <total_count> <query_file_id>"
                  << std::endl;
        return 1;
    }

    // Total count from command line
    int total_count = std::stoi(argv[1]);
    if (total_count <= 0) {
        std::cerr << "Error: n must be a positive integer." << std::endl;
        return 1;
    }

    // Number of queries from command line
    string file_name = "/home/leo/gpu-trie/data/embedding_table/"
                       "emb_l_19_weight_pca_8d_int32.csv";
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << file_name << std::endl;
        return 1;
    }

    string start_ranges_file =
        "/home/leo/gpu-trie/data/embedding_table/range_queries/range_start_" +
        string(argv[2]) + ".csv";
    std::ifstream start_ranges_file_stream(start_ranges_file);
    if (!start_ranges_file_stream.is_open()) {
        std::cerr << "Error: Unable to open " << start_ranges_file << std::endl;
        return 1;
    }

    string end_ranges_file =
        "/home/leo/gpu-trie/data/embedding_table/range_queries/range_end_" +
        string(argv[2]) + ".csv";
    std::ifstream end_ranges_file_stream(end_ranges_file);
    if (!end_ranges_file_stream.is_open()) {
        std::cerr << "Error: Unable to open " << end_ranges_file << std::endl;
        return 1;
    }

    string og_selected_file =
        "/home/leo/gpu-trie/data/embedding_table/selected_points/selected_" +
        string(argv[2]) + ".csv";
    std::ifstream og_selected_file_stream(og_selected_file);
    if (!og_selected_file_stream.is_open()) {
        std::cerr << "Error: Unable to open " << og_selected_file << std::endl;
        return 1;
    }

    // read in data
    // Allocate a buffer of size n * 16
    std::vector<int32_t> buffer;
    buffer.reserve(total_count * NUM_DIMENSIONS);

    std::string line;
    int row = 0;
    // Read each line until n rows have been processed.
    while (row++ < total_count && std::getline(file, line)) {
        std::vector<int32_t> point = parse_line_real_data(line);
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            buffer.push_back(point[i]);
            std::cout << point[i] << " ";
        }
        std::cout << std::endl;
    }

    // dimension_t num_dimensions = 9;
    // int n = 1000;
    trie_depth = 6;
    max_depth = 32;
    no_dynamic_sizing = true;

    bitmap::CompactPtrVector primary_key_to_treeblock_mapping(total_count);

    /* ---------- Initialization ------------ */
    // std::vector<level_t> bit_widths = {
    //     32, 32, 32, 16, 16, 16, 16, 8, 8, 8, 8, 8, 8, 8, 8, 8};
    // std::vector<level_t> start_bits = {
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    std::vector<level_t> bit_widths = {32, 32, 32, 32, 32, 32, 32, 32};
    std::vector<level_t> start_bits = {0, 0, 0, 0, 0, 0, 0, 0};

    create_level_to_num_children(bit_widths, start_bits, max_depth);

    std::cout << "configs: max_depth: " << (int)max_depth
              << ", trie_depth: " << (int)trie_depth
              << ", max_tree_node: " << max_tree_node << std::endl;
    // printout statically configured arrays in create_level_to_num_children
    std::cout << "bit_widths: ";
    for (auto &e : dimension_to_num_bits)
        std::cout << (int)e << ", ";
    std::cout << std::endl;
    std::cout << "start_bits: ";
    for (auto &e : start_dimension_bits)
        std::cout << (int)e << ", ";
    std::cout << std::endl;
    std::cout << "level_to_num_children: ";
    for (auto &e : level_to_num_children)
        std::cout << (int)e << ", ";
    std::cout << std::endl;

    md_trie<NUM_DIMENSIONS> mdtrie(max_depth, trie_depth, max_tree_node);

    init_cpu_frequency();

    /* ----------- INSERT ----------- */
    unsigned long long start, end, cumulative = 0;
    int idx = 0;

    vector<data_point<NUM_DIMENSIONS>> points;

    for (int primary_key = 0; primary_key < total_count; primary_key++) {
        data_point<NUM_DIMENSIONS> point;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; ++i) {
            point.set_coordinate(i, buffer[idx++]);
        }
        start = get_cpu_cycles_start();

        // printout point
        std::cout << "PRINTING OUT POINTS" << std::endl;
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            std::cout << point.get_coordinate(i) << " ";
        }
        std::cout << std::endl;

        mdtrie.insert_trie(&point, primary_key,
                           &primary_key_to_treeblock_mapping);
        end = get_cpu_cycles_end();
        cumulative += end - start;

        points.push_back(std::move(point));
    }

    std::cout << "Insertion Latency: "
              << cpu_cycles_to_ns(cumulative) / total_count << " ns"
              << std::endl;

    /* ---------- LOOKUP ------------ */
    // cumulative = 0;
    // for (int primary_key = 0; primary_key < n; primary_key++) {
    //     // int primary_key = buffer[primary_key_idx * NUM_DIMENSIONS];
    //     start = get_cpu_cycles_start();
    //     data_point<NUM_DIMENSIONS> *pt =
    //         mdtrie.lookup_trie(primary_key,
    //         &primary_key_to_treeblock_mapping);
    //     end = get_cpu_cycles_end();
    //     if ((int)pt->get_coordinate(0) != primary_key) {
    //         // print the point
    //         for (int i = 0; i < NUM_DIMENSIONS; i++) {
    //             std::cerr << pt->get_coordinate(i) << " ";
    //         }
    //         std::cerr << std::endl;

    //         std::cerr << "Wrong point retrieved!" << std::endl;
    //     }
    //     cumulative += end - start;
    // }
    // std::cout << "Lookup Latency: " << cpu_cycles_to_ns(cumulative) / n
    // << " ns"
    //           << std::endl;

    /* ---------- WARM UP ------------ */

    for (int c = 0; c < 1000; c++) {
        data_point<NUM_DIMENSIONS> start_range;
        data_point<NUM_DIMENSIONS> end_range;
        std::vector<int32_t> found_points;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; i++) {
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, (uint64_t)1 << bit_widths[i]);
        }
        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
    }

    /* ---------- REAL RANGE QUERY ------------ */

    std::string start_ranges_line;
    std::string end_ranges_line;
    int query_count = stoi(argv[2]);
    std::vector<int32_t> start_ranges_buffer;
    std::vector<int32_t> end_ranges_buffer;

    // Read each line until n rows have been processed.
    while (std::getline(start_ranges_file_stream, start_ranges_line)) {
        std::getline(end_ranges_file_stream, end_ranges_line);

        std::vector<int32_t> start_range_points = parse_line_real_data(line);
        std::vector<int32_t> end_range_points = parse_line_real_data(line);

        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            start_ranges_buffer.push_back(start_range_points[i]);
            end_ranges_buffer.push_back(end_range_points[i]);
        }
    }

    cumulative = 0;
    int ranges_idx = 0;
    // int num_queries = 10;
    for (int c = 0; c < query_count; c++) {
        data_point<NUM_DIMENSIONS> start_range;
        data_point<NUM_DIMENSIONS> end_range;
        std::vector<int32_t> found_points;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; i++) {
            // start_range.set_coordinate(i, start_ranges_buffer[ranges_idx]);
            // end_range.set_coordinate(i, end_ranges_buffer[ranges_idx++]);
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, (uint64_t)1 << 32 - 1);
        }

        start = get_cpu_cycles_start();
        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
        end = get_cpu_cycles_end();
        // Coordinates are flattened into one vector.
        std::cout << "Found points: " << found_points.size() << std::endl;
        if (found_points.size() == 0 ||
            found_points.size() % NUM_DIMENSIONS != 0) {
            std::cerr << "Wrong number of points found!" << std::endl;
            std::cerr << "Found: " << found_points.size() / NUM_DIMENSIONS
                      << std::endl;
            std::cerr << "Expected: " << " AT LEAST ONE!" << std::endl;
        }
        cumulative += end - start;

        // print out all points, by num_dimension
        if (c == query_count - 1) {
            for (int i = 0; i < found_points.size(); i++) {
                std::cout << found_points[i] << " ";
                if ((i + 1) % NUM_DIMENSIONS == 0) {
                    std::cout << std::endl;
                }
            }
        }
    }
    std::cout << "Total Range Query Latency: " << cpu_cycles_to_ns(cumulative)
              << " ns" << std::endl;

    std::string og_selected_line;
    for (int c = 0; c < query_count; c++) {

        std::getline(og_selected_file_stream, og_selected_line);

        std::vector<int32_t> point_buffer =
            parse_line_real_data(og_selected_line);

        data_point<NUM_DIMENSIONS> start_range;
        data_point<NUM_DIMENSIONS> end_range;
        std::vector<int32_t> found_points;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; i++) {
            start_range.set_coordinate(i, point_buffer[i]);
            end_range.set_coordinate(i, point_buffer[i]);
        }

        // printout start end range
        std::cout << "PRINTING OUT START RANGE" << std::endl;
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            std::cout << start_range.get_coordinate(i) << " ";
        }
        std::cout << std::endl;
        // end
        std::cout << "PRINTING OUT END RANGE" << std::endl;
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            std::cout << end_range.get_coordinate(i) << " ";
        }
        std::cout << std::endl;

        start = get_cpu_cycles_start();
        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
        end = get_cpu_cycles_end();
        // Coordinates are flattened into one vector.
        std::cout << "Found points: " << found_points.size() << std::endl;
        if (found_points.size() != NUM_DIMENSIONS) {
            std::cerr << "NOT FOUND... :(" << std::endl;
        }
        cumulative += end - start;

        // print out all points, by num_dimension
        for (int i = 0; i < found_points.size(); i++) {
            std::cout << found_points[i] << " ";
            if ((i + 1) % NUM_DIMENSIONS == 0) {
                std::cout << std::endl;
            }
        }
    }

    // Range Query Correctness
    for (int i = 0; i < total_count; i++) {
        std::vector<int> found_points;

        data_point<NUM_DIMENSIONS> start_range = points[i];
        data_point<NUM_DIMENSIONS> end_range = points[i];

        // printout point
        std::cout << "PRINTING OUT FOUND POINT" << std::endl;
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            std::cout << start_range.get_coordinate(i) << " ";
        }
        std::cout << std::endl;

        mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0,
                                 found_points);
        // Coordinates are flattened into one vector.
        if ((int)(found_points.size() / NUM_DIMENSIONS) != 1) {
            std::cerr << "Wrong number of points found!" << std::endl;
            std::cerr << "found: "
                      << (int)(found_points.size() / NUM_DIMENSIONS)
                      << std::endl;
            std::cerr << "total count: " << 1 << std::endl;
        } else {
            std::cout << "Query - " << i << ", found: "
                      << (int)(found_points.size() / NUM_DIMENSIONS)
                      << ", expected: " << 1 << std::endl;
        }
    }
    return 0;
}