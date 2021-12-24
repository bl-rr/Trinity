#include "trie.h"
#include <unistd.h>
#include <sys/time.h>
#include <climits>
#include <tqdm.h>
#include <vector>
#include <iostream>
#include <fstream>

void run_bench(level_t max_depth, level_t trie_depth, preorder_t max_tree_node){

    auto *found_points = new point_array();
    auto *all_points = new std::vector<data_point>();
    all_points_ptr = all_points;

    auto *mdtrie = new md_trie(max_depth, trie_depth, max_tree_node);
    auto *leaf_point = new data_point();

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;
    FILE *fp = fopen("../libmdtrie/bench/data/sample_shuf.txt", "r");
    std::ofstream writefile;
    writefile.open("filesystem_with_size.csv");

    // If the file cannot be open
    if (fp == nullptr)
    {
        fprintf(stderr, "file not found\n");
        exit(EXIT_FAILURE);
    }
    
    n_leaves_t n_points = 0;
    uint64_t max[DATA_DIMENSION];
    uint64_t min[DATA_DIMENSION];
    n_leaves_t n_lines = 14583357;
    total_points_count = n_lines;

    tqdm bar;
    TimeStamp start, diff;
    diff = 0;

    /**
     * Insertion
     */

    while ((read = getline(&line, &len, fp)) != -1)
    {
        bar.progress(n_points, n_lines);
        char *token = strtok(line, " ");
        char *ptr;

        for (uint8_t i = 1; i <= 2; i ++){
            token = strtok(nullptr, " ");
        }

        for (dimension_t i = 0; i < DATA_DIMENSION; i++){
            token = strtok(nullptr, " ");
            leaf_point->set_coordinate(i, strtoul(token, &ptr, 10));
        }

        for (dimension_t i = 0; i < DATA_DIMENSION; i++){
            
            if (n_points == 0){
                max[i] = leaf_point->get_coordinate(i);
                min[i] = leaf_point->get_coordinate(i);
            }
            else {
                if (leaf_point->get_coordinate(i) > max[i]){
                    max[i] = leaf_point->get_coordinate(i);
                }
                if (leaf_point->get_coordinate(i) < min[i]){
                    min[i] = leaf_point->get_coordinate(i);
                }
            }          
        }
        (*all_points).push_back((*leaf_point));
        start = GetTimestamp();
        mdtrie->insert_trie(leaf_point, n_points);
        diff += GetTimestamp() - start;
        n_points ++;
    }

    bar.finish();
    std::cout << "Insertion Latency: " << (float) diff / n_lines << std::endl;
    std::cout << "mdtrie storage: " << mdtrie->size() << std::endl;

    /**
     * check whether a data point exists given coordinates
     */

    tqdm bar2;
    TimeStamp check_diff = 0;
    for (uint64_t i = 0; i < n_lines; i++){
        bar2.progress(i, n_lines);
        auto check_point = (*all_points)[i];
        start = GetTimestamp();
        if (!mdtrie->check(&check_point)){
            raise(SIGINT);
        } 
        check_diff += GetTimestamp() - start;  
    }
    bar2.finish();
    std::cout << "Average time to check one point: " << (float) check_diff / n_lines << std::endl;

    auto *start_range = new data_point();
    auto *end_range = new data_point();

    int itr = 0;
    std::ofstream file("range_search_filesystem.csv", std::ios_base::app);
    srand(time(NULL));

    /**
     * Benchmark range search given a query selectivity
     */

    tqdm bar3;
    while (itr < 600){
        bar3.progress(itr, 600);

        for (unsigned int j = 0; j < DATA_DIMENSION; j++){
            start_range->set_coordinate(j, min[j] + (max[j] - min[j] + 1) / 10 * (rand() % 10));
            end_range->set_coordinate(j, start_range->get_coordinate(j) + (max[j] - start_range->get_coordinate(j) + 1) / 10 * (rand() % 10));
        }

        auto *found_points_temp = new point_array();
        start = GetTimestamp();
        mdtrie->range_search_trie(start_range, end_range, mdtrie->root(), 0, found_points_temp);
        diff = GetTimestamp() - start;

        if (found_points_temp->size() >= 1000){
            file << found_points_temp->size() << "," << diff << "," << std::endl;
            itr ++;
        }
    }
    bar3.finish();
    
    /**
     * Range Search with full range
     */

    start_range = new data_point();
    end_range = new data_point();
    for (dimension_t i = 0; i < DATA_DIMENSION; i++){
        start_range->set_coordinate(i, min[i]);
        end_range->set_coordinate(i, max[i]);
    }

    start = GetTimestamp();
    mdtrie->range_search_trie(start_range, end_range, mdtrie->root(), 0, found_points);
    diff = GetTimestamp() - start;

    std::cout << "found_pts size: " << found_points->size() << std::endl;
    std::cout << "Range Search Latency: " << (float) diff / found_points->size() << std::endl;

    /**
     * Point lookup given primary keys returned by range search
     */

    n_leaves_t found_points_size = found_points->size();
    TimeStamp diff_primary = 0;
    n_leaves_t checked_points_size = 0;
    tqdm bar4;

    for (n_leaves_t i = 0; i < found_points_size; i += 5){
        checked_points_size++;

        bar4.progress(i, found_points_size);
        data_point *point = found_points->at(i);
        n_leaves_t returned_primary_key = point->read_primary();

        symbol_t *node_path_from_primary = (symbol_t *)malloc((max_depth + 1) * sizeof(symbol_t));

        tree_block *t_ptr = (tree_block *) (p_key_to_treeblock_compact.At(returned_primary_key));
        
        start = GetTimestamp();
        symbol_t parent_symbol_from_primary = t_ptr->get_node_path_primary_key(returned_primary_key, node_path_from_primary);
        node_path_from_primary[max_depth - 1] = parent_symbol_from_primary;
        data_point *returned_coordinates = t_ptr->node_path_to_coordinates(node_path_from_primary, DATA_DIMENSION);
        diff_primary += GetTimestamp() - start;

        for (dimension_t j = 0; j < DATA_DIMENSION; j++){
            if (returned_coordinates->get_coordinate(j) != point->get_coordinate(j)){
                std::cerr << "points found incorrect!" << std::endl;
                raise(SIGINT);
            }
        }    
        free(node_path_from_primary);
    }
    bar4.finish();
    std::cout << "Lookup Latency: " << (float) diff_primary / checked_points_size << std::endl;
}

int main() {

    /**
     * Set hyperparameters
     * treeblock_size: maximum number of nodes a treeblock can hode
     * trie_depth: the maximum level of the top-level pointer-based trie structure
     * max_depth: the depth of whole data structure
     * dimension_bits: the bit widths of each column, with default start-level all set to 0.
     */

    level_t treeblock_size = 512;
    uint32_t trie_depth = 10;
    level_t max_depth = 32;

    std::vector<level_t> dimension_bits = {32, 32, 32, 32, 24, 24, 32};

    std::cout << "dimension: " << DATA_DIMENSION << std::endl;
    std::cout << "trie depth: " << trie_depth << std::endl;
    std::cout << "treeblock sizes: " << treeblock_size << std::endl;
    create_level_to_num_children(dimension_bits, max_depth);

    run_bench(max_depth, trie_depth, treeblock_size);
    std::cout << std::endl;
}