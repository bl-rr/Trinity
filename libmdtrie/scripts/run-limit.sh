#!/usr/bin/env bash

# while true; do; sudo -v; sleep 60; done &

mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk

# do tpch
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=128000 ./build/microbench -b tpch-disk-lookup

# do github
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=188416 ./build/microbench -b github-disk-lookup

# do nyc
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=237568 ./build/microbench -b nyc-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=45293568 ./build/microbench -b tpch-disk-query

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=41201664 ./build/microbench -b github-disk-query


sudo ./build/drop_cache
./cgroup_estimate.sh --mem=84584448 ./build/microbench -b nyc-disk-query

mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk-1


# iteration 2

mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk

# do tpch
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=128000 ./build/microbench -b tpch-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=37744640 ./build/microbench -b tpch-disk-query

# do github
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=117760 ./build/microbench -b github-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=34334720 ./build/microbench -b github-disk-query

# do nyc
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=148480 ./build/microbench -b nyc-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=70487040 ./build/microbench -b nyc-disk-query

mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk-2

# iteration 3
mkdir -p /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk

# do tpch
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=128000 ./build/microbench -b tpch-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=37744640 ./build/microbench -b tpch-disk-query

# do github
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=117760 ./build/microbench -b github-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=34334720 ./build/microbench -b github-disk-query

# do nyc
sudo ./build/drop_cache
./cgroup_estimate.sh --mem=148480 ./build/microbench -b nyc-disk-lookup

sudo ./build/drop_cache
./cgroup_estimate.sh --mem=70487040 ./build/microbench -b nyc-disk-query

mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbench-disk-3