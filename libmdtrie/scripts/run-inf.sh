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
# sudo ./build/drop_cache
# ./cgroup_launch.sh --mem=infinity ./build/microbench -b github-disk-lookup-wwarmup
# sudo ./build/drop_cache
# ./cgroup_launch.sh --mem=infinity ./build/microbench -b nyc-disk-lookup-wwarmup
sudo ./build/drop_cache
./cgroup_launch.sh --mem=4725106816 ./build/microbench -b tpch-disk-lookup-wwarmup
# save to _10 file
# mv /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk /home/wuyue/Desktop/lbh/gpu-mdtrie/disk-trinity/results/microbenchmark-disk_inf