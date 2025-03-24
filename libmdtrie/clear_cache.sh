sudo sync
sudo bash -c "echo 3 > /proc/sys/vm/drop_caches"
sudo swapoff -a && sudo swapon -a