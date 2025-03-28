#include "device_vector.h"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <cstdio>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include <fcntl.h>    // open
#include <sys/mman.h> // mmap, munmap
#include <sys/stat.h> // fstat
#include <unistd.h>   // close

#include "clocking_utils.hpp"

#define NUM_DIMENSIONS 8

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <*.bin> <query_file_id>"
                  << std::endl;
        return 1;
    }

    // 1. open the file
    int fd;
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }

    // 2. obtain the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        return 1;
    }
    size_t filesize = sb.st_size;

    // init
    max_depth_ = 32;
    trie_depth_ = 6;
    max_tree_nodes_ = 512;

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

    // 3. mmap the file
    md_trie<NUM_DIMENSIONS> *mdtrie = (md_trie<NUM_DIMENSIONS> *)mmap(
        nullptr, filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mdtrie == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd); // fd no longer needed after mmap

    uint64_t base_addr = (uint64_t)(mdtrie);

    // 4. deserialize
    mdtrie->deserialize(base_addr);

    // 5. query

    // set up the query
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

    int query_count = std::stoi(argv[2]);

    /* ---------- WARM UP ------------ */

    // querying the entire range for 1000 times
    for (int c = 0; c < 10; c++) {
        data_point<NUM_DIMENSIONS> start_range;
        data_point<NUM_DIMENSIONS> end_range;
        device_vector<int32_t> found_points;
        for (dimension_t i = 0; i < NUM_DIMENSIONS; i++) {
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, INT32_MAX);
        }
        mdtrie->range_search_trie(&start_range, &end_range, mdtrie->root(), 0,
                                  found_points);
        std::cout << "found_points.size(): "
                  << found_points.size() / NUM_DIMENSIONS << std::endl;
    }

    /* ---------- REAL RANGE QUERY ------------ */

    std::string start_ranges_line;
    std::string end_ranges_line;
    std::vector<int32_t> start_ranges_buffer;
    std::vector<int32_t> end_ranges_buffer;

    // read in all the queries
    while (std::getline(start_ranges_file_stream, start_ranges_line)) {
        std::getline(end_ranges_file_stream, end_ranges_line);

        std::vector<int32_t> start_range_points =
            parse_line_real_data(start_ranges_line);
        std::vector<int32_t> end_range_points =
            parse_line_real_data(end_ranges_line);

        for (int i = 0; i < NUM_DIMENSIONS; i++) {
            start_ranges_buffer.push_back(start_range_points[i]);
            end_ranges_buffer.push_back(end_range_points[i]);
        }
    }

    unsigned long long start, end, cumulative = 0;
    int query_iterations = 1;
    int ranges_idx = 0;
    int total_found_points = 0;
    for (int iter = 0; iter < query_iterations; iter++) {
        ranges_idx = 0;
        for (int c = 0; c < query_count; c++) {
            data_point<NUM_DIMENSIONS> start_range;
            data_point<NUM_DIMENSIONS> end_range;
            device_vector<int32_t> found_points;
            for (dimension_t i = 0; i < NUM_DIMENSIONS; i++) {
                start_range.set_coordinate(i, start_ranges_buffer[ranges_idx]);
                end_range.set_coordinate(i, end_ranges_buffer[ranges_idx++]);
            }

            start = get_cpu_cycles_start();
            mdtrie->range_search_trie(&start_range, &end_range, mdtrie->root(),
                                      0, found_points);
            end = get_cpu_cycles_end();

            // Coordinates are flattened into one vector.
            if (found_points.size() == 0 ||
                found_points.size() % NUM_DIMENSIONS != 0) {
                std::cerr << "Wrong number of points found!" << std::endl;
                std::cerr << "Found: " << found_points.size() / NUM_DIMENSIONS
                          << std::endl;
                std::cerr << "Expected: " << " AT LEAST ONE!" << std::endl;
            }
            total_found_points += found_points.size() / NUM_DIMENSIONS;
            cumulative += end - start;
        }
    }
    std::cout << "\t" << "Total Found Points: " << total_found_points
              << std::endl;
    std::cout << "\t" << "Average Range Query Latency: "
              << cpu_cycles_to_ns(cumulative) / query_iterations / query_count
              << " ns" << std::endl;

    std::cout << "[Finished benchmark...]" << std::endl;

    // 6. Cleanup
    munmap(mdtrie, filesize);

    return 0;
}