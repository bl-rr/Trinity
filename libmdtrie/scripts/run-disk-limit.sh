#!/usr/bin/env bash

# while true; do; sudo -v; sleep 60; done &

# stats
# github_serialize/trie.bin: 4080188860
# // 10%: 408018886
# // 20%: 816037772
# // 30%: 1224056658
# // 40%: 1632075544
# // 50%: 2040094430
# // 60%: 2448113316
# // 70%: 2856132202
# // 80%: 3264151088
# // 90%: 3672169974
# // 100%: 4080188860

# nyc_serialize/trie.bin: 4065889294
# // 10%: 406588929
# // 20%: 813177858
# // 30%: 1219766787
# // 40%: 1626355716
# // 50%: 2032944645
# // 60%: 2439533574
# // 70%: 2846122503
# // 80%: 3252711432
# // 90%: 3659300361
# // 100%: 4065889294

# tpch_serialize/trie.bin: 11812767040
# // 10%: 1181276704
# // 20%: 2362553408
# // 30%: 3543830112
# // 40%: 4725106816
# // 50%: 5906383520
# // 60%: 7087660224
# // 70%: 8268936928
# // 80%: 9450213632
# // 90%: 10631490336
# // 100%: 11812767040


# 10%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=408018886 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=406588929 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1181276704 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=408018886 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=406588929 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1181276704 ./build/microbench -b tpch-disk-query
# save to _10 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_10

# 20%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=816037772 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=813177858 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2362553408 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=816037772 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=813177858 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2362553408 ./build/microbench -b tpch-disk-query
# save to _20 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_20

# 30%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1224056658 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1219766787 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3543830112 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1224056658 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1219766787 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3543830112 ./build/microbench -b tpch-disk-query
# save to _30 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_30

# 40%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1632075544 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1626355716 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4725106816 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1632075544 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=1626355716 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4725106816 ./build/microbench -b tpch-disk-query
# save to _40 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_40

# 50%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2040094430 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2032944645 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=5906383520 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2040094430 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2032944645 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=5906383520 ./build/microbench -b tpch-disk-query
# save to _50 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_50

# 60%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2448113316 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2439533574 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=7087660224 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2448113316 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2439533574 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=7087660224 ./build/microbench -b tpch-disk-query
# save to _60 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_60

# 70%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2856132202 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2846122503 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=8268936928 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2856132202 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=2846122503 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=8268936928 ./build/microbench -b tpch-disk-query
# save to _70 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_70

# 80%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3264151088 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3252711432 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=9450213632 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3264151088 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3252711432 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=9450213632 ./build/microbench -b tpch-disk-query
# save to _80 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_80

# 90%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3672169974 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3659300361 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=10631490336 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3672169974 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=3659300361 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=10631490336 ./build/microbench -b tpch-disk-query
# save to _90 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_90

# 100%
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk
# lookups
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4080188860 ./build/microbench -b github-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4065889294 ./build/microbench -b nyc-disk-lookup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=11812767040 ./build/microbench -b tpch-disk-lookup
# queries
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4080188860 ./build/microbench -b github-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4065889294 ./build/microbench -b nyc-disk-query
sudo ./build/drop_cache
./cgroup_launch.sh --mem=11812767040 ./build/microbench -b tpch-disk-query
# save to _100 file
mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_100