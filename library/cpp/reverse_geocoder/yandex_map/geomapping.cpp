#include "geomapping.h"

#include <util/stream/file.h>
#include <util/string/split.h>

#include <library/cpp/reverse_geocoder/logger/log.h>

using namespace NReverseGeocoder;
using namespace NYandexMap;

TMappingTraits MakeTraitsFromLine(const TString& line) {
    TVector<TStringBuf> stringParts;
    StringSplitter(line).Split('\t').AddTo(&stringParts);

    static const size_t colsQty = 13;
    if (stringParts.size() < colsQty) {
        DEBUG_LOG << ">>> NB: bad mapping data: " << line << "\n";
    }

    TMappingTraits traits;
    {
        // NB: format of geomatch files:
        // reg_id {0} \t reg_type \t reg_path_name \t toponym_path \t kind \t toponim_id \t toponym_parent_id {6} \t match_traits_str \t borders_qty \t points_qty \t poly_square \t source_id \t uri {12}

        traits.RegionId = FromString<int>(stringParts[0]);
        traits.RegType = FromString<int>(stringParts[1]);
        traits.RegNamePath = stringParts[2];

        traits.ToponymId = FromString<long>(stringParts[5]);
        traits.Kind = stringParts[4];
        traits.ToponymPath = stringParts[3];

        traits.SourceId = FromString<long>(stringParts[11]);
    }
    return traits;
}

Id2ToponymMappingType NReverseGeocoder::NYandexMap::LoadGeoMapping(TStringBuf mappingFileName, TTopId2RegIdMapping& sid2regidMap, TTopId2RegIdMapping& topid2regidMap) {
    Id2ToponymMappingType mapping;
    if (mappingFileName.empty()) {
        return mapping;
    }

    TFileInput input(mappingFileName.data());

    const char COMMENT = '#';
    TString line;
    size_t rowsAmount = 0;

    while (input.ReadLine(line)) {
        ++rowsAmount;
        if (COMMENT == line[0] || line.empty())
            continue;

        const auto trait = MakeTraitsFromLine(line);

        if (trait.ToponymId < 0) {
            ythrow yexception() << "unknown top-id " << trait.ToString();
        }
        if (trait.RegionId <= 0) {
            ythrow yexception() << "unknown reg-id " << trait.ToString();
        }

        mapping[trait.RegionId] = trait;
        if (-1 != trait.SourceId) {
            sid2regidMap[trait.SourceId] = trait.RegionId;
        }
        if (0 == trait.ToponymId) {
            WARNING_LOG << ">>> NOTA BENE: top-id eq ZERO for " << trait.ToString() << "\n";
        }
        topid2regidMap[trait.ToponymId] = trait.RegionId;
    }

    DEBUG_LOG << ">>> mapping(" << mappingFileName << "): " << rowsAmount << "/" << mapping.size() << " (total/loaded)\n";
    return mapping;
}

TString TMappingTraits::ToString() const {
    TStringStream out;
    out << "GEOMATCH: " << RegionId << "/t:" << RegType << "/" << RegNamePath << "\t" << ToponymId << "/"<< Kind << "/" << ToponymPath;
    return out.Str();

}
