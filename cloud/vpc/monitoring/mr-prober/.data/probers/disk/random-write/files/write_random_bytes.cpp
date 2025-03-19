#include <algorithm>
#include <ctime>
#include <chrono>
#include <cstdio>
#include <climits>
#include <functional>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>


using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

std::vector<unsigned char> generate_random_data(size_t size) {
    std::vector<unsigned char> data(size);

    random_bytes_engine rbe;
    std::generate(std::begin(data), std::end(data), std::ref(rbe));

    return data;
}

unsigned long long get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("USAGE: ./prober <filename> <size_in_bytes>\n");
        return 1;
    }

    std::string filename(argv[1]);
    int size = atoi(argv[2]);

    printf("[%llu ms] Starting...\n", get_current_timestamp());

    if (size <= 0 || size > 1000000000) {
        printf("[ERROR] Size can not be less than zero or more than 1 GB\n");
        return 2;
    }

    auto data = generate_random_data(size);

    printf("[%llu ms] Generated %zu bytes of random data\n", get_current_timestamp(), data.size());

    auto file = open(filename.c_str(), O_WRONLY | O_CREAT | O_DIRECT | O_SYNC, 0660);
    if (!file) {
        printf("[ERROR] Can't open file %s\n", filename.c_str());
        return 3;
    }

    printf("[%llu ms] Opened file %s\n", get_current_timestamp(), filename.c_str());

    size_t wrote = 0;
    while (wrote < data.size()) {
        size_t bytes_wrote = write(file, data.data() + wrote, data.size() - wrote);
        if (bytes_wrote >= 0) {
            wrote += bytes_wrote;
        } else {
            printf("[ERROR] Can't write data to file %s\n", filename.c_str());
            return 4;
        }
    }
    close(file);

    printf("[%llu ms] Wrote %zu bytes to %s\n", get_current_timestamp(), data.size(), filename.c_str());

    std::remove(filename.c_str());

    printf("[%llu ms] Removed %s\n", get_current_timestamp(), filename.c_str());

    return 0;
}
