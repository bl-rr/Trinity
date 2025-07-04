#ifndef disk_MdTrieBench_H
#define disk_MdTrieBench_H

#include "common.hpp"
#include "disk_trie.h"
#include <climits>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

template <dimension_t DIMENSION>
class disk_MdTrieBench
{
public:
  disk_MdTrieBench(disk_md_trie<DIMENSION> *disk_mdtrie, disk_bitmap::disk_CompactPtrVector *disk_p_key_to_treeblock_compact, uint64_t base)
  {
    disk_mdtrie_ = disk_mdtrie;
    base_ = base;
    disk_p_key_to_treeblock_compact_ = disk_p_key_to_treeblock_compact;
  };

  void disk_lookup(std::string outfile_name)
  {

    TimeStamp cumulative = 0, start = 0;

    for (point_t i = 0; i < points_to_lookup; i++)
    {
      start = GetTimestamp();
      disk_mdtrie_->disk_lookup_trie(i, disk_p_key_to_treeblock_compact_, base_);
      TimeStamp temp_diff = GetTimestamp() - start;
      cumulative += temp_diff;
      lookup_latency_vect_.push_back(temp_diff + SERVER_TO_SERVER_IN_NS);
    }
    flush_vector_to_file(lookup_latency_vect_,
                         results_folder_addr +
                             outfile_name);
    std::cout << "Done! "
              << "Lookup Latency per point: "
              << (float)cumulative / points_to_lookup << std::endl;

    std::ofstream outFile(outfile_name);
    outFile << ("Lookup Latency per point: " +
                std::to_string((float)cumulative / points_to_lookup) + "\n");
  }

  void disk_lookup_cold(std::string outfile_name, const char *filename)
  {

    disk_md_trie<DIMENSION> *disk_mdtrie;
    disk_bitmap::disk_CompactPtrVector *disk_primary_key_to_treeblock_mapping; // pointer to array
    uint64_t base;                                                             // needed for operation
    void *file_start;                                                          // needed for madvise
    size_t file_size;

    TimeStamp cumulative = 0, start = 0;

    for (point_t i = 0; i < points_to_lookup; i++)
    {
      int fd = deserialize_from_file_cold(filename, &disk_mdtrie,
                                          &disk_primary_key_to_treeblock_mapping, &base, &file_start, &file_size);
      start = GetTimestamp();
      disk_mdtrie->disk_lookup_trie(i, disk_primary_key_to_treeblock_mapping, base);
      TimeStamp temp_diff = GetTimestamp() - start;
      cumulative += temp_diff;
      lookup_latency_vect_.push_back(temp_diff + SERVER_TO_SERVER_IN_NS);

      munmap(file_start, file_size);
      close(fd);

      system("/home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/libmdtrie/build/drop_cache");
    }
    flush_vector_to_file(lookup_latency_vect_,
                         results_folder_addr +
                             outfile_name);
    std::cout << "Done! "
              << "Lookup Latency per point: "
              << (float)cumulative / points_to_lookup << std::endl;

    std::ofstream outFile(outfile_name);
    outFile << ("Lookup Latency per point: " +
                std::to_string((float)cumulative / points_to_lookup) + "\n");
  }

  void disk_lookup_npc(std::string outfile_name, void *file_start, size_t file_size)
  {

    TimeStamp cumulative = 0, start = 0;

    TimeStamp temp_diff;

    for (point_t i = 0; i < points_to_lookup; i++)
    {
      madvise(file_start, file_size, MADV_DONTNEED);
      start = GetTimestamp();
      disk_mdtrie_->disk_lookup_trie(i, disk_p_key_to_treeblock_compact_, base_);
      temp_diff = GetTimestamp() - start;
      madvise(file_start, file_size, MADV_DONTNEED);
      cumulative += temp_diff;
      lookup_latency_vect_.push_back(temp_diff + SERVER_TO_SERVER_IN_NS);
    }
    flush_vector_to_file(lookup_latency_vect_,
                         results_folder_addr +
                             outfile_name);
    std::cout << "Done! "
              << "Lookup Latency per point: "
              << (float)cumulative / points_to_lookup << std::endl;

    std::ofstream outFile(outfile_name);
    outFile << ("Lookup Latency per point: " +
                std::to_string((float)cumulative / points_to_lookup) + "\n");
  }

  void disk_range_search(std::string query_addr,
                         std::string outfile_name,
                         void (*get_query)(std::string,
                                           data_point<DIMENSION> *,
                                           data_point<DIMENSION> *))
  {

    std::ifstream file(query_addr);
    std::ofstream outfile(results_folder_addr +
                          outfile_name);
    TimeStamp diff = 0, start = 0;
    TimeStamp cumulative = 0;

    for (int i = 0; i < QUERY_NUM; i++)
    {

      std::vector<int32_t> found_points;
      data_point<DIMENSION> start_range;
      data_point<DIMENSION> end_range;

      std::string line;
      std::getline(file, line);
      get_query(line, &start_range, &end_range);

      start = GetTimestamp();
      disk_mdtrie_->disk_range_search_trie(
          &start_range, &end_range, disk_mdtrie_->root(base_), 0, found_points, base_);
      diff = GetTimestamp() - start;
      cumulative += diff;
      outfile << "Query " << i << ": " << diff << std::endl;
      found_points.clear();
    }

    outfile << "Done! "
            << "Total end to end latency (ms): " << cumulative / 1000
            << ", average per query: " << cumulative / 1000 / QUERY_NUM
            << std::endl;
    std::cout << "Done! "
              << "Total end to end latency (ms): " << cumulative / 1000
              << ", average per query: " << cumulative / 1000 / QUERY_NUM
              << std::endl;
  }

  void disk_range_search_cold(std::string query_addr,
                              std::string outfile_name,
                              void (*get_query)(std::string,
                                                data_point<DIMENSION> *,
                                                data_point<DIMENSION> *),
                              const char *filename)
  {

    disk_md_trie<DIMENSION> *disk_mdtrie;
    disk_bitmap::disk_CompactPtrVector *disk_primary_key_to_treeblock_mapping; // pointer to array
    uint64_t base;                                                             // needed for operation
    void *file_start;                                                          // needed for madvise
    size_t file_size;

    std::ifstream file(query_addr);
    std::ofstream outfile(results_folder_addr +
                          outfile_name);
    TimeStamp diff = 0, start = 0;
    TimeStamp cumulative = 0;

    for (int i = 0; i < QUERY_NUM; i++)
    {
      // std::cout << "trying to open file: " << std::string(filename) << std::endl;
      int fd = deserialize_from_file_cold(filename, &disk_mdtrie,
                                          &disk_primary_key_to_treeblock_mapping, &base, &file_start, &file_size);

      std::vector<int32_t> found_points;
      data_point<DIMENSION> start_range;
      data_point<DIMENSION> end_range;

      std::string line;
      std::getline(file, line);
      get_query(line, &start_range, &end_range);

      start = GetTimestamp();
      disk_mdtrie->disk_range_search_trie(
          &start_range, &end_range, disk_mdtrie->root(base), 0, found_points, base);
      diff = GetTimestamp() - start;
      cumulative += diff;
      outfile << "Query " << i << ": " << diff << std::endl;
      found_points.clear();

      munmap(file_start, file_size);
      close(fd);

      system("/home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/libmdtrie/build/drop_cache");
    }

    outfile << "Done! "
            << "Total end to end latency (ms): " << cumulative / 1000
            << ", average per query: " << cumulative / 1000 / QUERY_NUM
            << std::endl;
    std::cout << "Done! "
              << "Total end to end latency (ms): " << cumulative / 1000
              << ", average per query: " << cumulative / 1000 / QUERY_NUM
              << std::endl;
  }

  void disk_range_search_npc(std::string query_addr,
                             std::string outfile_name,
                             void (*get_query)(std::string,
                                               data_point<DIMENSION> *,
                                               data_point<DIMENSION> *),
                             void *file_start, size_t file_size)
  {

    std::ifstream file(query_addr);
    std::ofstream outfile(results_folder_addr +
                          outfile_name);
    TimeStamp diff = 0, start = 0;
    TimeStamp cumulative = 0;

    for (int i = 0; i < QUERY_NUM; i++)
    {

      std::vector<int32_t> found_points;
      data_point<DIMENSION> start_range;
      data_point<DIMENSION> end_range;

      std::string line;
      std::getline(file, line);
      get_query(line, &start_range, &end_range);

      madvise(file_start, file_size, MADV_DONTNEED);
      start = GetTimestamp();
      disk_mdtrie_->disk_range_search_trie(
          &start_range, &end_range, disk_mdtrie_->root(base_), 0, found_points, base_);
      diff = GetTimestamp() - start;
      madvise(file_start, file_size, MADV_DONTNEED);
      cumulative += diff;
      outfile << "Query " << i << " end to end latency (ms): " << diff / 1000
              << ", found points count: " << found_points.size() / DIMENSION
              << std::endl;
      found_points.clear();
    }

    outfile << "Done! "
            << "Total end to end latency (ms): " << cumulative / 1000
            << ", average per query: " << cumulative / 1000 / QUERY_NUM
            << std::endl;
    std::cout << "Done! "
              << "Total end to end latency (ms): " << cumulative / 1000
              << ", average per query: " << cumulative / 1000 / QUERY_NUM
              << std::endl;
  }

  void disk_range_search_random(std::string outfile_name,
                                void (*get_query)(data_point<DIMENSION> *,
                                                  data_point<DIMENSION> *),
                                unsigned int upper_bound,
                                unsigned int lower_bound)
  {

    std::ofstream outfile(results_folder_addr +
                          outfile_name);
    TimeStamp diff = 0, start = 0;
    int i = 0;

    while (i < QUERY_NUM * 10)
    {

      std::vector<int32_t> found_points;
      data_point<DIMENSION> start_range;
      data_point<DIMENSION> end_range;

      get_query(&start_range, &end_range);

      start = GetTimestamp();
      disk_mdtrie_->disk_range_search_trie(
          &start_range, &end_range, disk_mdtrie_->root(base_), 0, found_points);
      diff = GetTimestamp() - start;

      if (found_points.size() / DIMENSION > upper_bound ||
          found_points.size() / DIMENSION < lower_bound)
        continue;

      outfile << "Query " << i << " end to end latency (ms): " << diff / 1000
              << ", found points count: " << found_points.size() / DIMENSION
              << std::endl;
      found_points.clear();
      i += 1;
    }
  }

protected:
  std::vector<TimeStamp> insertion_latency_vect_;
  std::vector<TimeStamp> lookup_latency_vect_;
  disk_md_trie<DIMENSION> *disk_mdtrie_;
  uint64_t base_;
  disk_bitmap::disk_CompactPtrVector *disk_p_key_to_treeblock_compact_;
};

#endif // disk_MdTrieBench_H
