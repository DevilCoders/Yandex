#include <iostream>

#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/core/geo_data/map.h>
#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/reverse_geocoder/library/log.h>

#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>
#include <util/string/printf.h>

using namespace NReverseGeocoder;

static const char* GetOutput(const TGeoId regionId, const TReverseGeocoder& reverseGeocoder) {
    const char* output = nullptr;

    reverseGeocoder.EachKv(regionId, [&](const char* k, const char* v) {
        if (!strcmp(k, "name:en"))
            output = v;
    });

    if (output)
        return output;

    reverseGeocoder.EachKv(regionId, [&](const char* k, const char* v) {
        if (!strcmp(k, "name"))
            output = v;
    });

    return output;
}

static void ShowGeoData(const TReverseGeocoder& reverseGeocoder) {
    NGeoData::Show(Cout, reverseGeocoder.GeoData());
}

static void ShowMaxEdgeRefs(const TReverseGeocoder& reverseGeocoder, size_t regionsNumber) {
    THashMap<TGeoId, size_t> size;
    THashMap<TGeoId, size_t> points;

    reverseGeocoder.EachPolygon([&](const TPolygon& polygon) {
        reverseGeocoder.EachPart(polygon, [&](const TPart&, const TNumber edgeRefsNumber) {
            size[polygon.RegionId] += edgeRefsNumber * sizeof(*reverseGeocoder.GeoData().EdgeRefs());
        });
        points[polygon.RegionId] += polygon.PointsNumber;
    });

    TVector<std::pair<size_t, TGeoId>> regions;
    for (auto const& p : size)
        regions.emplace_back(p.second, p.first);

    Sort(regions.begin(), regions.end());
    std::reverse(regions.begin(), regions.end());

    for (size_t i = 0; i < std::min(regionsNumber, regions.size()); ++i) {
        TString output;
        sprintf(output, "Edge refs size: %s (%lu) = %.3f Mb (%lu points)", GetOutput(regions[i].second, reverseGeocoder),
                regions[i].second, regions[i].first / (1024.0 * 1024.0), points[regions[i].second]);
        Cout << output << Endl;
    }
}

static ui64 GetBitsNumber(ui64 x) {
    ui64 number = 0;
    while (x) {
        ++number;
        x >>= 1;
    }
    return number;
}

static ui64 ToBytes(ui64 x) {
    return x / 8 + (x % 8 == 0 ? 0 : 1);
}

static void ShowPossibleEdgeRefsCompression(const TReverseGeocoder& reverseGeocoder) {
    const IGeoData& g = reverseGeocoder.GeoData();

    ui64 size = 0;
    ui64 possibleByteSize = 0;

    reverseGeocoder.EachPolygon([&](const TPolygon& polygon) {
        reverseGeocoder.EachPart(polygon, [&](const TPart& part, const TNumber edgeRefsNumber) {
            const TNumber edgeRefsOffset = part.EdgeRefsOffset;
            size += edgeRefsNumber * sizeof(*reverseGeocoder.GeoData().EdgeRefs());

            TRef minEdgeRef = g.EdgeRefs()[edgeRefsOffset];
            for (TNumber i = edgeRefsOffset; i < edgeRefsOffset + edgeRefsNumber; ++i)
                minEdgeRef = std::min(minEdgeRef, g.EdgeRefs()[i]);

            ui64 bitsNumber = 0;
            for (TNumber i = edgeRefsOffset; i < edgeRefsOffset + edgeRefsNumber; ++i)
                bitsNumber = std::max(bitsNumber, GetBitsNumber(g.EdgeRefs()[i] - minEdgeRef));

            possibleByteSize += ToBytes(bitsNumber) * edgeRefsNumber;
        });
    });

    {
        TString output;
        sprintf(output, "Possible edge refs size: %.3f Mb", possibleByteSize / (1024.0 * 1024.0));
        Cout << output << Endl;
    }
    {
        TString output;
        sprintf(output, "Possible edge refs compression: %.2f%%", possibleByteSize * 100.0 / size);
        Cout << output << Endl;
    }
}

void ShowGeoDataStat(const char* path, const ui16 topMemoryRegions) {
    TReverseGeocoder reverseGeocoder(path);

    ShowGeoData(reverseGeocoder);
    ShowMaxEdgeRefs(reverseGeocoder, topMemoryRegions);
    ShowPossibleEdgeRefsCompression(reverseGeocoder);
}
