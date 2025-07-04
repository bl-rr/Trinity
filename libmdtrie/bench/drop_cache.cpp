// cache_reset.cpp
// g++ -O2 -std=c++17 cache_reset.cpp -o cache_reset
// sudo ./cache_reset
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

using std::size_t;

static size_t parse_size(const std::string &s)
{
    // "32K", "256K", "32M"  ➜ bytes
    char unit = s.back();
    long long num = std::stoll(s.substr(0, s.size() - 1));
    switch (unit)
    {
    case 'K':
    case 'k':
        return num * 1024ULL;
    case 'M':
    case 'm':
        return num * 1024ULL * 1024ULL;
    case 'G':
    case 'g':
        return num * 1024ULL * 1024ULL * 1024ULL;
    default:
        return static_cast<size_t>(num); // already bytes
    }
}

static size_t detect_llc_size()
{
    namespace fs = std::filesystem;
    const std::string base = "/sys/devices/system/cpu/cpu0/cache";
    size_t max_size = 0;

    for (const auto &dir : fs::directory_iterator(base))
    {
        const auto level_file = dir.path() / "level";
        const auto type_file = dir.path() / "type";
        const auto size_file = dir.path() / "size";

        try
        {
            std::ifstream tf(type_file);
            std::string type;
            tf >> type;
            if (type != "Unified")
                continue;

            std::ifstream lf(level_file);
            int level = 0;
            lf >> level;
            if (level < 3)
                continue; // we want the last-level cache

            std::ifstream sf(size_file);
            std::string size_str;
            sf >> size_str;

            size_t size_bytes = parse_size(size_str);
            if (size_bytes > max_size)
                max_size = size_bytes;
        }
        catch (...)
        { /* ignore parsing errors */
        }
    }
    return max_size;
}

static void thrash_cpu_cache(size_t bytes)
{
    const size_t stride = 64; // typical cache-line size
    std::vector<char> buf(bytes, 0);

    for (size_t i = 0; i < bytes; i += stride)
        buf[i] = static_cast<char>(i); // one touch per line
}

static void drop_page_cache()
{
    // Flush dirty buffers first
    sync();

    std::ofstream dc("/proc/sys/vm/drop_caches");
    if (!dc)
    {
        std::cerr << "Cannot open /proc/sys/vm/drop_caches (need root?)\n";
        return;
    }
    dc << "3\n"; // 1 = page-cache, 2 = dentries & inodes, 3 = both
    dc.flush();
}

int main()
{
    if (geteuid() != 0)
    {
        std::cerr << "⚠️  Please run as root (sudo) so I can drop page cache.\n";
    }

    size_t llc = detect_llc_size();
    if (llc == 0)
    {
        std::cerr << "Could not detect LLC size; defaulting to 512 MiB.\n";
        llc = 512ULL * 1024 * 1024;
    }

    // 4× LLC, but never less than 256 MiB
    size_t buf_size = std::max(llc * 4, static_cast<size_t>(256ULL * 1024 * 1024));
    // std::cout << "Detected LLC ≈ " << (llc / (1024 * 1024)) << " MiB — "
    //           << "thrashing " << (buf_size / (1024 * 1024)) << " MiB buffer …\n";

    thrash_cpu_cache(buf_size);
    // std::cout << "✅ CPU cache polluted.\n";

    drop_page_cache();
    // std::cout << "✅ Linux page/dentry/inode cache dropped.\n";
    return 0;
}