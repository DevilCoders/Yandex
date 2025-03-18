#include "lookup.h"

#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>

#include <util/generic/ptr.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

using namespace NReverseGeocoder;

namespace {
    TLocation LocationFromString(const TString& ll_data) {
        TStringInput sstream(ll_data);
        TLocation loc;
        sstream >> loc.Lat >> loc.Lon;
        return loc;
    }
}

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

static void Show(IOutputStream& output, const TReverseGeocoder& reverseGeocoder, const TVector<TGeoId>& debug) {
    for (int i = (int)debug.size() - 1; i >= 0; --i) {
        for (int j = (int)debug.size() - 1; j > i; --j)
            output << " ";
        output << "→ [" << debug[i] << "] " << GetOutput(debug[i], reverseGeocoder) << Endl;
    }
}

static void ShowExact(IOutputStream& output, const TReverseGeocoder& reverseGeocoder, const TGeoId& regionId) {
    output << regionId << Endl;
    reverseGeocoder.EachKv(regionId, [&](const char* k, const char* v) {
        output << "  K=\"" << k << "\" V=\"" << v << "\"" << Endl;
    });
}

int TLookupTool::Run(const TString& llPointsFileName) {
    TReverseGeocoder reverseGeocoder(Config_.Path.c_str());

    THolder<TReverseGeocoder::TDebug> debug;
    if (Config_.Trace)
        debug = MakeHolder<TReverseGeocoder::TDebug>();

    IInputStream* pInput = &Cin;
    TAutoPtr<IInputStream> filePtr;

    if ("-" != llPointsFileName) {
        filePtr.Reset(new TFileInput(llPointsFileName));
        pInput = filePtr.Get();
    }

    try {
        TString inputLine;
        while (pInput->ReadLine(inputLine)) {
            const TGeoId regionId = reverseGeocoder.Lookup(LocationFromString(inputLine), debug.Get());
            if (regionId == UNKNOWN_GEO_ID) {
                Cout << "-1\n";
                continue;
            }

            if (Config_.Trace) {
                Show(Cout, reverseGeocoder, *debug);
            } else {
                ShowExact(Cout, reverseGeocoder, regionId);
            }
        }

    } catch (const std::exception& e) {
        Cout << e.what() << Endl;
    }

    return 0;
}
