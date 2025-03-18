#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/core/geo_data/map.h>
#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>

#include <util/generic/algorithm.h>
#include <util/string/printf.h>

#include <thread>
#include <random>

using namespace NReverseGeocoder;

static const size_t LOOKUPS_NUMBER = 1e6;
static const size_t THREADS_NUMBER = 4;

static TLocation RandomLocation(std::mt19937& rnd) {
    TLocation l;
    l.Lat = rnd() * 180.0 / RAND_MAX - 90.0;
    l.Lon = rnd() * 360.0 / RAND_MAX - 180.0;
    return l;
}

void RunBenchmark(const char* path) {
    TReverseGeocoder reverseGeocoder(path);

    TVector<std::thread> threads(THREADS_NUMBER);
    TVector<TVector<float>> seconds(THREADS_NUMBER);

    for (size_t i = 0; i < THREADS_NUMBER; ++i) {
        threads[i] = std::thread([&reverseGeocoder, &seconds, i]() {
            Cout << "[" << i + 1 << "] Run benchmark" << Endl;

            std::mt19937 random(i + 337);

            TVector<TLocation> locations(LOOKUPS_NUMBER);
            for (size_t j = 0; j < locations.size(); ++j)
                locations[j] = RandomLocation(random);

            for (size_t j = 0; j < locations.size(); ++j) {
                TStopWatch stopWatch;
                stopWatch.Run();

                const TGeoId geoId = reverseGeocoder.Lookup(locations[j]);

                float const secs = stopWatch.Get();

                if (geoId != UNKNOWN_GEO_ID)
                    seconds[i].push_back(secs);
            }
        });
    }

    for (size_t i = 0; i < THREADS_NUMBER; ++i)
        threads[i].join();

    TVector<float> total;
    for (size_t i = 0; i < THREADS_NUMBER; ++i)
        total.insert(total.end(), seconds[i].begin(), seconds[i].end());

    Sort(total.begin(), total.end());

    for (int i = 1; i <= 99; ++i) {
        TString output;
        sprintf(output, "[%2d%%] <= %.6f", i, total[std::min(total.size() - 1, i * total.size() / 100)]);
        Cout << output << Endl;
    }

    for (int i = 1; i <= 100; ++i) {
        TString output;
        sprintf(output, "[%2d.%.2d%%] <= %6f", (i == 100 ? 100 : 99), (i == 100 ? 0 : i),
                total[std::min(total.size() - 1, (99 * 100 + i) * total.size() / 10000)]);
        Cout << output << Endl;
    }
}
