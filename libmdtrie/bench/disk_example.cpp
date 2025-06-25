#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "trie.h"
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
    dimension_t num_dimensions = 9;
    max_tree_node = 512;
    int total_count = 10000;
    trie_depth = 6;
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
        start = GetTimestamp();
        mdtrie.insert_trie(&point, primary_key, &primary_key_to_treeblock_mapping);
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Insertion Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    disk_md_trie<9> *disk_mdtrie = (disk_md_trie<9> *)(&mdtrie);

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

        // std::cout << "size of disk_CompactPtrVector " << sizeof(disk_bitmap::disk_CompactPtrVector) << std::endl;
        // std::cout << "size of CompactPtrVector " << sizeof(bitmap::CompactPtrVector) << std::endl;
        // std::cout << std::endl;
        // // compare bitvector and disk_bitvector
        // std::cout << "size of BitVector " << sizeof(bitmap::BitVector) << std::endl;
        // std::cout << "size of disk_BitVector " << sizeof(disk_bitmap::disk_BitVector) << std::endl;
        // std::cout << std::endl;
        // // compare size of unsizedbitmaparry and disk_unzisedbitmaparray
        // std::cout << "size of UnsizedBitmapArray " << sizeof(bitmap::UnsizedBitmapArray<size_t>) << std::endl;
        // std::cout << "size of disk_UnsizedBitmapArray " << sizeof(disk_bitmap::disk_UnsizedBitmapArray<size_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of bitmaparry and disk_bitmaparray
        // std::cout << "size of BitmapArray " << sizeof(bitmap::BitmapArray<size_t>) << std::endl;
        // std::cout << "size of disk_BitmapArray " << sizeof(disk_bitmap::disk_BitmapArray<size_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of unsignedbitmaparray and disk_unsignedbitmaparray
        // std::cout << "size of UnsignedBitmapArray " << sizeof(bitmap::UnsignedBitmapArray<size_t>) << std::endl;
        // std::cout << "size of disk_UnsignedBitmapArray " << sizeof(disk_bitmap::disk_UnsignedBitmapArray<size_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of signedbitmaparray and disk_signedbitmaparray
        // std::cout << "size of SignedBitmapArray " << sizeof(bitmap::SignedBitmapArray<int64_t>) << std::endl;
        // std::cout << "size of disk_SignedBitmapArray " << sizeof(disk_bitmap::disk_SignedBitmapArray<int64_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of bitmap and disk_bitmap
        // std::cout << "size of Bitmap " << sizeof(bitmap::Bitmap) << std::endl;
        // std::cout << "size of disk_Bitmap " << sizeof(disk_bitmap::disk_Bitmap) << std::endl;
        // std::cout << std::endl;
        // // compare size of compact_ptr and disk_compact_ptr
        // std::cout << "size of compact_ptr " << sizeof(bits::compact_ptr) << std::endl;
        // std::cout << "size of disk_compact_ptr " << sizeof(disk_bits::disk_compact_ptr) << std::endl;
        // std::cout << std::endl;
        // // compare size of compactvector and disk_compactvector
        // std::cout << "size of CompactVector " << sizeof(bitmap::CompactVector<uint64_t, 46>) << std::endl;
        // std::cout << "size of disk_CompactVector " << sizeof(disk_bitmap::disk_CompactVector<uint64_t, 46>) << std::endl;
        // std::cout << std::endl;
        // // compare size of compactptrvector and disk_compactptrvector
        // std::cout << "size of CompactPtrVector " << sizeof(bitmap::CompactPtrVector) << std::endl;
        // std::cout << "size of disk_CompactPtrVector " << sizeof(disk_bitmap::disk_CompactPtrVector) << std::endl;
        // std::cout << std::endl;
        // // compare size of compressedbitmap and disk_compressedbitmap
        // std::cout << "size of CompressedBitmap " << sizeof(compressed_bitmap::compressed_bitmap) << std::endl;
        // std::cout << "size of disk_CompressedBitmap " << sizeof(disk_compressed_bitmap::disk_compressed_bitmap) << std::endl;
        // std::cout << std::endl;
        // // compare size of deltaencodedarray and disk_deltaencodedarray
        // std::cout << "size of DeltaEncodedArray " << sizeof(bitmap::DeltaEncodedArray<size_t>) << std::endl;
        // std::cout << "size of disk_DeltaEncodedArray " << sizeof(disk_bitmap::disk_DeltaEncodedArray<size_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of eliasgammadeltaencodedarray and disk_eliasgammadeltaencodedarray
        // std::cout << "size of EliasGammaDeltaEncodedArray " << sizeof(bitmap::EliasGammaDeltaEncodedArray<size_t>) << std::endl;
        // std::cout << "size of disk_EliasGammaDeltaEncodedArray " << sizeof(disk_bitmap::disk_EliasGammaDeltaEncodedArray<size_t>) << std::endl;
        // std::cout << std::endl;
        // // compare size of treeblock and disk_treeblock
        // std::cout << "size of tree_block " << sizeof(tree_block<9>) << std::endl;
        // std::cout << "size of disk_tree_block " << sizeof(disk_tree_block<9>) << std::endl;
        // std::cout << std::endl;
        // // compare size of trie_node and disk_trie_node
        // std::cout << "size of trie_node " << sizeof(trie_node<9>) << std::endl;
        // std::cout << "size of disk_trie_node " << sizeof(disk_trie_node<9>) << std::endl;
        // std::cout << std::endl;
        // // compare size of md_trie and disk_md_trie
        // std::cout << "size of md_trie " << sizeof(md_trie<9>) << std::endl;
        // std::cout << "size of disk_md_trie " << sizeof(disk_md_trie<9>) << std::endl;

        // // compare size of std::vector and disk_std_vector
        // std::cout << "size of std::vector " << sizeof(std::vector<int>) << std::endl;
        // std::cout << "size of disk_std_vector " << sizeof(disk_std_vector<int>) << std::endl;

        data_point<9> *pt = disk_mdtrie->disk_lookup_trie(primary_key, (disk_bitmap::disk_CompactPtrVector *)(&primary_key_to_treeblock_mapping), 0);
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

        start = GetTimestamp();
        disk_mdtrie->disk_range_search_trie(&start_range, &end_range, disk_mdtrie->root(0), 0, found_points, 0);
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