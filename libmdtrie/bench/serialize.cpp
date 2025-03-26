#include "device_vector.h"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <cstdio>
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

    std::cout << "[Starting benchmark...]" << std::endl;

    if (std::stoi(argv[1]) > 9994222)
        argv[1] = "9994222";

    if (std::stoi(argv[2]) > 9994222)
        argv[2] = "9994222";

    std::cout << "total_count: " << argv[1] << std::endl;
    std::cout << "query_file_id: " << argv[2] << std::endl;

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

    int query_count = stoi(argv[2]);

    // buffer storing all points in a flattened manner, size = total_count *
    // NUM_DIMENSIONS
    std::vector<int32_t> buffer;
    buffer.reserve(total_count * NUM_DIMENSIONS);

    // read each line until n = total_count rows have been processed
    std::string line;
    int row = 0;
    while (row++ < total_count && std::getline(file, line)) {
        std::vector<int32_t> point = parse_line_real_data(line);
        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            buffer.push_back(point[i]);
        }
    }

    trie_depth = 6;
    max_depth = 32;
    no_dynamic_sizing = true;

    /* ---------- Initialization ------------ */

    device_vector<level_t> bit_widths(8, (level_t)32);
    device_vector<level_t> start_bits(8, (level_t)0);

    create_level_to_num_children(bit_widths, start_bits, max_depth);

    std::cout << "\t" << "configs: max_depth: " << (int)max_depth
              << ", trie_depth: " << (int)trie_depth
              << ", max_tree_node: " << max_tree_node << std::endl;
    std::cout << "\t" << "bit_widths: ";
    for (auto &e : dimension_to_num_bits)
        std::cout << (int)e << ", ";
    std::cout << "\t" << std::endl;
    std::cout << "\t" << "start_bits: ";
    for (auto &e : start_dimension_bits)
        std::cout << (int)e << ", ";
    std::cout << "\t" << std::endl;
    std::cout << "\t" << "level_to_num_children: ";
    for (auto &e : level_to_num_children)
        std::cout << (int)e << ", ";
    std::cout << "\t" << std::endl;

    md_trie<NUM_DIMENSIONS> mdtrie(max_depth, trie_depth, max_tree_node);

    init_cpu_frequency();

    /* ----------- INSERT ----------- */
    unsigned long long start, end, cumulative = 0;
    int idx = 0;

    for (int primary_key = 0; primary_key < total_count; primary_key++) {
        data_point<NUM_DIMENSIONS> point;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; ++i) {
            point.set_coordinate(i, buffer[idx++]);
        }
        start = get_cpu_cycles_start();

        mdtrie.insert_trie(&point, primary_key);
        end = get_cpu_cycles_end();
        cumulative += end - start;
    }

    std::cout << "\t" << "Insertion Latency: "
              << cpu_cycles_to_ns(cumulative) / total_count << " ns"
              << std::endl;

    // create a file for serialization
    FILE *s_file = fopen("trie.bin", "wb");

    // check if file is created successfully
    if (!s_file) {
        std::cerr << "Error: Unable to create file for serialization."
                  << std::endl;
        return 1;
    }

    mdtrie.serialize(s_file);
    fclose(s_file);

    return 0;
}