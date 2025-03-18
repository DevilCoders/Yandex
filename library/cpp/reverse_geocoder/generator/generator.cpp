#include "config.h"
#include "gen_geo_data.h"
#include "generator.h"
#include "mut_geo_data.h"
#include "raw_handler.h"
#include "regions_handler.h"
#include "slab_handler.h"

#include <library/cpp/reverse_geocoder/core/geo_data/map.h>
#include <library/cpp/reverse_geocoder/library/fs.h>
#include <library/cpp/reverse_geocoder/library/log.h>
#include <library/cpp/reverse_geocoder/logger/log.h>
#include <library/cpp/reverse_geocoder/proto_library/reader.h>

#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/split.h>

using namespace NReverseGeocoder;
using namespace NGenerator;

NReverseGeocoder::NGenerator::TGenerator::TGenerator(const TConfig& config, TGenGeoData* geoData)
    : Config(config)
    , GeoData(geoData)
    , Handlers()
{
#define HANDLER(Handler) \
    THandlerPtr(new Handler(Config, GeoData))

    Handlers = THandlerPtrs{
        HANDLER(TSlabHandler),
        HANDLER(TRawHandler),
        HANDLER(TRegionsHandler),
    };
#undef HANDLER
}

namespace {
    using TIdsList = THashSet<int>;
    using TId2IdMap = THashMap<int, int>;

    using TStringParts = TVector<TStringBuf>;
    using TLineProcessor = std::function<void(const TStringParts& parts)>;

    const char COMMENT = '#';
    const TStringBuf DEFAULT_SEPARATORS = " \t#";

    void PerLinerStreamProcessor(TStringBuf fileName, const TStringBuf lineSep, TLineProcessor processor) {
        if (fileName.empty()) {
            return;
        }

        size_t rowsAmount = 0;
        size_t rowsProcessed = 0;

        TFileInput input(fileName.data());

        TString line;
        while (input.ReadLine(line)) {
            ++rowsAmount;
            if (COMMENT == line[0] || line.empty())
                continue;

            TVector<TStringBuf> parts;
            Split(line.c_str(), lineSep.data(), parts);

            if (parts.size()) {
                ++rowsProcessed;
                processor(parts);
            }
        }

        DEBUG_LOG << ">>> input-file(" << fileName << "): " << rowsAmount << "/" << rowsProcessed << "(total/processed)\n";
    }

    TIdsList LoadExcludeList(TStringBuf fileName) {
        TIdsList list;

        PerLinerStreamProcessor(
            fileName,
            DEFAULT_SEPARATORS,
            [&](const TStringParts& parts) {
                const int excudeId = FromString<int>(parts[0]); // NB: only 1-st field in row is significant; others - helpers/comments, etc.
                list.insert(excudeId);
            });

        DEBUG_LOG << ">>> exclude-list(" << fileName << "): " << list.size() << " ids loaded\n";
        return list;
    }

    TId2IdMap LoadIdsSubstList(TStringBuf fileName) {
        TId2IdMap list;

        PerLinerStreamProcessor(
            fileName,
            DEFAULT_SEPARATORS,
            [&](const TStringParts& parts) {
                const int fromId = FromString<int>(parts[0]);
                const int toId = FromString<int>(parts[1]);
                list.insert({fromId, toId});
            });

        DEBUG_LOG << ">>> subst-ids-list(" << fileName << "): " << list.size() << " pairs loaded\n";
        return list;
    }
}

void NReverseGeocoder::NGenerator::TGenerator::Init() {
    for (THandlerPtr& h : Handlers)
        h->Init();
}

void NReverseGeocoder::NGenerator::TGenerator::Update(const NProto::TRegion& region) {
    for (THandlerPtr& h : Handlers)
        h->Update(region);
}

void NReverseGeocoder::NGenerator::TGenerator::Fini() {
    for (THandlerPtr& h : Handlers)
        h->Fini();
}

void NReverseGeocoder::NGenerator::Generate(const TConfig& config) {
    TMutGeoData geoData;
    TGenerator generator(config, &geoData);

    generator.Init();
    const auto& idsExcludeList = LoadExcludeList(config.ExcludePath);
    const auto& idsSubstList = LoadIdsSubstList(config.SubstPath);

    for (const auto& fname : GetDataFilesList(config.InputPath.data())) {
        NProto::TReader reader(fname.data());
        reader.Each([&](const NProto::TRegion& region) {
            if (idsExcludeList.end() != idsExcludeList.find(region.GetRegionId())) {
                return;
            }

            const auto& itSubst = idsSubstList.find(region.GetRegionId());
            if (idsSubstList.end() == itSubst) {
                generator.Update(region);
                return;
            }

            const int newId = itSubst->second;
            NProto::TRegion regionDup = region;
            regionDup.SetRegionId(newId);
            generator.Update(regionDup);
        });
    }

    generator.Fini();
    TGeoDataMap::SerializeToFile(config.OutputPath.data(), geoData);
}
