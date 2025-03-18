#include "extractor.h"

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/reverse_geocoder/library/fs.h>
#include <library/cpp/reverse_geocoder/proto_library/reader.h>

#include <util/generic/hash_set.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NReverseGeocoder;
using namespace NBorderExtractor;

TInitParams::TInitParams(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    options
        .AddLongOption('i', "input", "-- input GeoBase protobuf data path; maybe comma-separated list; maybe dirname for autoselect all files")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&InputPath);

    options
        .AddLongOption('I', "wanted_ids", "-- show borders for specified region; maybe comma-separated list; show all if not specified")
        .Optional()
        .RequiredArgument("REGION_ID")
        .StoreResult(&WantedIds);

    options
        .AddLongOption('s', "save-in", "directory to store borders in separated files '${SAVE-IN}/${reg_id}.border'; folder should exists")
        .Optional()
        .RequiredArgument("SAVE-IN")
        .StoreResult(&SavePath);

    options
        .AddLongOption('c', "check_only", "-- check for existance and print simple metadata")
        .Optional()
        .NoArgument()
        .DefaultValue("0")
        .OptionalValue("1")
        .StoreResult(&CheckOnly);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);
}

namespace {
    using TIdsList = THashSet<int>;

    TIdsList ParseIntString(const char* str) {
        TVector<TStringBuf> parts;
        Split(str, ",", parts);

        TIdsList list;
        for (const auto& value : parts) {
            list.insert(FromString<int>(value));
        }
        return list;
    }

    void DumpBordersInStream(IOutputStream& out, const NProto::TRegion& region) {
        int polyNum = 0;
        for (const NProto::TPolygon& polygon : region.GetPolygons()) {
            polyNum++;
            out << "\n# p#" << polyNum << "; " << polygon.LocationsSize() << " points\n";

            for (const NProto::TLocation& l : polygon.GetLocations()) {
                out << l.GetLat() << " " << l.GetLon() << " " << "\n";
            }
        }
    }
}

void NBorderExtractor::Extract(const TInitParams& params) {
    auto wantedIds = ParseIntString(params.WantedIds.c_str());
    const bool showAll = wantedIds.empty();
    bool doNothing = false;

    for (const auto& fname : GetDataFilesList(params.InputPath.c_str())) {
        NProto::TReader reader(fname.data());
        reader.Each([&](const NProto::TRegion& region) {
            if (doNothing) {
                return;
            }

            if (!showAll) {
                if (wantedIds.empty()) {
                    doNothing = true;
                    return;
                }

                const auto& it = wantedIds.find(region.GetRegionId());
                if (wantedIds.end() == it) {
                    return;
                }
                wantedIds.erase(it);
            }

            if (params.CheckOnly) {
                Cout << "\n# r#" << region.GetRegionId() << "; " << region.PolygonsSize() << " parts\n";
                return;
            }

            if (params.SavePath.length()) {
                const auto& fname = TFsPath(params.SavePath) / TString::Join(ToString(region.GetRegionId()), ".border");
                TFileOutput regBorder(fname);
                DumpBordersInStream(regBorder, region);
            } else {
                DumpBordersInStream(Cout, region);
            }
        });
    }
}
