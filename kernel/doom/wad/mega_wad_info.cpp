#include "mega_wad_info.h"

#include <kernel/doom/wad/mega_wad_info.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <util/string/cast.h>
#include <util/stream/output.h>
#include <util/stream/mem.h>
#include <util/memory/blob.h>


namespace NDoom {

void SaveMegaWadInfo(IOutputStream* output, const TMegaWadInfo& info) {
    TMegaWadInfoData proto;

    /* We want consistent serialization for MD5 checks, thus we sort global lumps by #. */
    TVector<std::pair<ui32, TString>> globalLumps;
    for (auto&& [lumpName, lumpIndex] : info.GlobalLumps) {
        globalLumps.emplace_back(lumpIndex, lumpName);
    }
    std::sort(globalLumps.begin(), globalLumps.end());

    for (auto&& [lumpIndex, lumpName] : globalLumps) {
        TMegaWadInfoData::TLumpInfo* inf = proto.AddGlobalLumps();

        inf->SetType(lumpName);
        inf->SetIndex(lumpIndex);
    }

    /* Cannot sort doc lumps as order matters here. */
    for (TString lump : info.DocLumps)
        proto.AddDocLumps(std::move(lump));

    proto.SetFirstDoc(info.FirstDoc);
    proto.SetDocCount(info.DocCount);

    NProtobufJson::TProto2JsonConfig config;
    config.SetFormatOutput(true);
    NProtobufJson::Proto2Json(proto, *output, config);
}

TMegaWadInfo LoadMegaWadInfo(IInputStream* input) {
    TMegaWadInfo result;
    TMegaWadInfoData proto = NProtobufJson::Json2Proto<TMegaWadInfoData>(*input);

    for (size_t i = 0; i < proto.GlobalLumpsSize(); i++) {
        const TMegaWadInfoData::TLumpInfo& info = proto.GetGlobalLumps(i);
        result.GlobalLumps[info.GetType()] = info.GetIndex();
    }

    for (size_t i = 0; i < proto.DocLumpsSize(); i++)
        result.DocLumps.push_back(proto.GetDocLumps(i));

    result.DocCount = proto.GetDocCount();
    result.FirstDoc = proto.GetFirstDoc();

    return result;
}

TMegaWadInfo LoadMegaWadInfo(const TBlob& blob) {
    TMemoryInput input(blob.AsCharPtr(), blob.Size());
    return LoadMegaWadInfo(&input);
}

} // NDoom


