#ifndef MD_CLOCKING_UTILS_BENCH_H
#define MD_CLOCKING_UTILS_BENCH_H

static int cpu_freq_KHz; // Global variable for CPU frequency in kHz
static __attribute__((always_inline)) unsigned long get_cpu_cycles_start() {
    unsigned cycles_high0, cycles_low0;
    asm volatile("cpuid\n\t"
                 "rdtsc\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high0), "=r"(cycles_low0)::"%rax", "%rbx",
                   "%rcx", "%rdx");
    return (unsigned long)cycles_high0 << 32 | cycles_low0;
}
static __attribute__((always_inline)) unsigned long get_cpu_cycles_end() {
    unsigned cycles_high1, cycles_low1;
    asm volatile("RDTSCP\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 "CPUID\n\t"
                 : "=r"(cycles_high1), "=r"(cycles_low1)::"%rax", "%rbx",
                   "%rcx", "%rdx");
    return (unsigned long)cycles_high1 << 32 | cycles_low1;
}
void init_cpu_frequency() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    if (cpuinfo.is_open()) {
        while (std::getline(cpuinfo, line)) {
            if (line.substr(0, 7) == "cpu MHz") {
                float cpuMhz = std::stof(line.substr(line.find(':') + 2));
                cpu_freq_KHz = cpuMhz * 1000.0; // Convert MHz to kHz
                break;
            }
        }
        cpuinfo.close();
    } else {
        std::cerr << "Failed to open /proc/cpuinfo" << std::endl;
        cpu_freq_KHz = 0; // Default to 0 if unable to read
        exit(1);
    }
}

__inline__ double cpu_cycles_to_ns(unsigned long long cycles) {
    // Since cpu_freq_KHz represents kHz (cycles per millisecond),
    // dividing cycles by cpu_freq_KHz gives milliseconds.
    // Multiply the result by 1,000,000 to convert milliseconds to nanoseconds.
    return (double)cycles / cpu_freq_KHz * 1000000.0;
}

#endif // MD_TRIE_MD_TRIE_H