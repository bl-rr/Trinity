#include "trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>

#include <algorithm>

#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>
#include <sys/wait.h>

#define NUM_DIMENSION 16

std::vector<data_point<NUM_DIMENSION>> load_points(const std::string &filename)
{
    std::ifstream infile(filename);
    std::string line;
    std::vector<data_point<NUM_DIMENSION>> points;

    while (points.size() < 344 && std::getline(infile, line))
    {
        std::stringstream ss(line);
        data_point<NUM_DIMENSION> point;
        point.set_coordinate(0, points.size());

        std::string cell;
        for (dimension_t d = 1; d < NUM_DIMENSION && std::getline(ss, cell, ','); ++d)
        {
            point.set_coordinate(d, std::stoull(cell));
        }
        points.push_back(point);
    }

    if (points.size() < 344)
    {
        std::cerr << "❌ Not enough points loaded from file.\n";
        exit(1);
    }

    return points;
}

// This function runs in a subprocess to test crashability
bool test_combination(std::vector<data_point<NUM_DIMENSION>> &points, std::vector<int> &indices)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        preorder_t max_tree_node = 128;
        level_t trie_depth = 1;
        level_t max_depth = 64;
        no_dynamic_sizing = true;
        bitmap::CompactPtrVector primary_key_to_treeblock_mapping(1000);

        std::vector<level_t> bit_widths(NUM_DIMENSION, 64);
        std::vector<level_t> start_bits(NUM_DIMENSION, 0);
        create_level_to_num_children(bit_widths, start_bits, max_depth);

        md_trie<NUM_DIMENSION> mdtrie(max_depth, trie_depth, max_tree_node);
        std::sort(indices.begin(), indices.end());

        for (int i : indices)
        {
            mdtrie.insert_trie(&points[i], i, &primary_key_to_treeblock_mapping);
            // std::cout << "Inserting point with primary key: " << i << std::endl;
        }
        mdtrie.insert_trie(&points[343], 343, &primary_key_to_treeblock_mapping);

        exit(0); // success
    }
    else
    {
        int status = 0;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status) == 0;
        }
        else if (WIFSIGNALED(status))
        {
            std::cerr << "❌ Crashed with signal: " << strsignal(WTERMSIG(status)) << "\n";
            return false;
        }
        else
        {
            std::cerr << "❌ Unknown child termination\n";
            return false;
        }
    }
}

// Binary shrinking of the crashing set
std::vector<int> delta_debug(std::vector<data_point<NUM_DIMENSION>> &points, std::vector<int> subset)
{
    int n = 2;
    while (subset.size() >= 2)
    {
        bool reduced = false;
        size_t chunk_size = subset.size() / n;

        for (int i = 0; i < n; ++i)
        {
            std::vector<int> trial;

            // Copy everything except chunk i
            for (int j = 0; j < (int)subset.size(); ++j)
            {
                if (j < i * chunk_size || j >= (i + 1) * chunk_size)
                {
                    trial.push_back(subset[j]);
                }
            }

            if (!test_combination(points, trial))
            {
                // Found a crashing smaller subset
                subset = trial;
                n = std::max(2, n - 1);
                reduced = true;
                break;
            }
        }

        if (!reduced)
        {
            if (n >= (int)subset.size())
                break; // can't split further
            n = std::min((int)subset.size(), n * 2);
        }
    }

    return subset;
}

bool is_1_minimal(std::vector<data_point<NUM_DIMENSION>> &points, const std::vector<int> &subset)
{
    for (size_t i = 0; i < subset.size(); ++i)
    {
        std::vector<int> trial;
        for (size_t j = 0; j < subset.size(); ++j)
        {
            if (i != j)
                trial.push_back(subset[j]);
        }
        if (!test_combination(points, trial))
        {
            std::cerr << "❌ Subset is NOT minimal: removing index " << subset[i] << " still crashes\n";
            return false;
        }
    }
    std::cout << "✅ Verified: subset is 1-minimal.\n";
    return true;
}

int main()
{
    std::string filename = "/home/wuyue/Desktop/lbh/gpu-mdtrie/data/morton_solver/sorted_output.csv";
    auto points = load_points(filename);

    std::vector<int> full_set;
    for (int i = 0; i < 343; ++i)
    {
        full_set.push_back(i);
    }

    if (!test_combination(points, full_set))
    {
        std::cout << "🚨 Confirmed: full set + 343 causes crash. Running delta-debug...\n";
        auto result = delta_debug(points, full_set);
        std::cout << "\n🎯 Minimal crashing subset:\n";

        is_1_minimal(points, result);
        for (int i : result)
        {
            std::cout << i << " ";
        }
        std::cout << "+ 343\n";
    }
    else
    {
        std::cout << "✅ Full set does NOT crash. No bug to isolate.\n";
    }

    return 0;
}