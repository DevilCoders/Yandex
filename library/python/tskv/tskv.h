#pragma once

#include <library/cpp/string_utils/tskv_format/tskv_map.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

using TTSKVData = THashMap<TString, TString>;

inline TTSKVData TSKVStringToDict(const char* data, size_t len) {
    const auto s_buff = TStringBuf(data, len);
    TTSKVData dict;
    {
        NTskvFormat::DeserializeMap(s_buff, dict);
    }
    return dict;
}

inline TString DictToTSKVString(const TTSKVData& dict) {
    TString res;
    return NTskvFormat::SerializeMap(dict, res);
}
