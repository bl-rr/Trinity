#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "disk_trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>

// Function to generate a random integer in the range [min, max]
int random_int(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

int main()
{

    /* ---------- Settings ------------ */
    dimension_t num_dimensions = 9;
    max_tree_node = 1024;
    int total_count = 1000000;
    trie_depth = 6;
    max_depth = 32;

    max_depth_ = 32;
    trie_depth_ = 6;
    max_tree_nodes_ = 512;

    /* ---------- Initialization ------------ */
    std::vector<level_t> bit_widths = {
        32, 32, 32, 32, 32, 32, 32, 32, 32};
    std::vector<level_t> start_bits = {
        0, 0, 0, 0, 0, 0, 0, 0, 0};
    create_level_to_num_children(bit_widths, start_bits, max_depth);

    /* ----------- INSERT via deserialization ----------- */
    TimeStamp start = 0, cumulative = 0;

    int fd;
    if ((fd = open("/home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/libmdtrie/build/trie.bin", O_RDWR)) == -1)
    {
        perror("open");
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("fstat");
        return 1;
    }
    size_t filesize = sb.st_size;

    void *file_start = mmap(
        nullptr, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    madvise(file_start, filesize, MADV_RANDOM);
    if (file_start == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    };

    uint64_t cpv_offset = *((size_t *)file_start);

    disk_md_trie<9> *disk_mdtrie = (disk_md_trie<9> *)((uint64_t)file_start + sizeof(size_t));
    disk_bitmap::disk_CompactPtrVector *primary_key_to_treeblock_mapping =
        (disk_bitmap::disk_CompactPtrVector *)((uint64_t)file_start + cpv_offset);

    uint64_t base = (uint64_t)(file_start);

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

        madvise(file_start, filesize, MADV_DONTNEED);

        start = GetTimestamp();
        data_point<9> *pt = disk_mdtrie
                                ->disk_lookup_trie(primary_key, primary_key_to_treeblock_mapping, base);
        cumulative += GetTimestamp() - start;

        if ((int)pt->get_coordinate(0) != primary_key)
        {
            std::cerr << "Wrong point retrieved!" << std::endl;
        }
        // print out the point for validation
        std::cout << "Gottent Point: ";
        for (dimension_t i = 0; i < num_dimensions; i++)
        {
            std::cout << pt->get_coordinate(i) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Lookup Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    /* ---------- RANGE QUERY ------------ */
    cumulative = 0;
    int num_queries = 3;
    std::cout << "Creating range queries that return every point. " << std::endl;
    for (int c = 0; c < num_queries; c++)
    {
        data_point<9> start_range;
        data_point<9> end_range;
        std::vector<int> found_points;
        /* We used the first coordinate to be the primary key. */
        start_range.set_coordinate(0, 0);
        end_range.set_coordinate(0, total_count);
        for (dimension_t i = 1; i < num_dimensions; i++)
        {
            start_range.set_coordinate(i, 0);
            end_range.set_coordinate(i, (int)(1 << 16));
        }

        madvise(file_start, filesize, MADV_DONTNEED);
        start = GetTimestamp();
        disk_mdtrie->disk_range_search_trie(&start_range, &end_range, disk_mdtrie->root(base), 0, found_points, base);
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