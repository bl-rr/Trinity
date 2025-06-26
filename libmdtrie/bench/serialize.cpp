#include "benchmark.hpp"
// #include "common.hpp"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>

// Function to generate a random integer in the range [min, max]
int random_int(int min, int max)
{
    // static std::random_device rd;
    static std::mt19937 gen(1);
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

int main()
{
    dimension_t num_dimensions = 9;
    max_tree_node = 1024;
    int total_count = 1000000;
    trie_depth = 1;
    max_depth = 32;
    no_dynamic_sizing = true;
    bitmap::CompactPtrVector primary_key_to_treeblock_mapping(total_count);

    /* ---------- Initialization ------------ */
    std::vector<level_t> bit_widths = {
        32, 32, 32, 32, 32, 32, 32, 32, 32};
    std::vector<level_t> start_bits = {
        0, 0, 0, 0, 0, 0, 0, 0, 0};
    create_level_to_num_children(bit_widths, start_bits, max_depth);

    md_trie<9> mdtrie(max_depth, trie_depth, max_tree_node);

    /* ----------- INSERT ----------- */
    TimeStamp start = 0, cumulative = 0;
    int num_inserted = 0;

    for (int primary_key = 0; primary_key < total_count; primary_key++)
    {
        num_inserted++;
        if (num_inserted % (total_count / 10) == 0)
        {
            std::cout << "Inserting: " << num_inserted << " out of " << total_count << std::endl;
        }
        data_point<9> point;
        // For lookup correctness checking.
        point.set_coordinate(0, primary_key);
        for (dimension_t i = 1; i < num_dimensions; ++i)
        {
            point.set_coordinate(i, random_int(1, (int)1 << 16));
        }
        // print out the point
        // std::cout << "Inserting Point: ";
        // for (dimension_t i = 0; i < num_dimensions; i++)
        // {
        //     std::cout << point.get_coordinate(i) << " ";
        // }
        // std::cout << std::endl;

        start = GetTimestamp();
        mdtrie.insert_trie(&point, primary_key, &primary_key_to_treeblock_mapping);
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Insertion Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    // create a file for serialization
    FILE *s_file = fopen("trie.bin", "wb");

    // check if file is created successfully
    if (!s_file)
    {
        std::cerr << "Error: Unable to create file for serialization."
                  << std::endl;
        return 1;
    }

    // save space to store offset of the compactptrvector, use a dummy value for now
    size_t offset = 0;
    fwrite(&offset, sizeof(size_t), 1, s_file);
    current_offset += sizeof(size_t);

    mdtrie.serialize(s_file);

    // at this point, we know the location of the compactptrvector in the file
    // offset = ftell(s_file);
    assert(current_offset == ftell(s_file));
    // go back to the beginning of the file and write the offset
    fseek(s_file, 0, SEEK_SET);
    fwrite(&current_offset, sizeof(size_t), 1, s_file);
    // go back to the end of the file
    fseek(s_file, 0, SEEK_END);

    uint64_t ptr_addr = (uint64_t)(primary_key_to_treeblock_mapping.At(0));
    std::cout << (pointers_to_offsets_map.at(ptr_addr)) << std::endl;

    primary_key_to_treeblock_mapping.serialize(s_file);

    fclose(s_file);
    return 0;
}