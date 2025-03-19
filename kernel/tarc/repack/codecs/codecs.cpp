#include "codecs.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/codecs/static/static.h>

#include <library/cpp/codecs/codecs_registry.h>
#include <kernel/tarc/repack/codecs/offroad_codecs.h>

#include <library/cpp/codecs/static/static_codec_info.pb.h>

#include <openssl/md5.h>

namespace NRepack {
    using CodecsStorage = THashMap<TString, TCodec>;

    TString CalculateCodecsHash(const TCodec::DictsData& data) {
        MD5_CTX ctx;
        MD5_Init(&ctx);

        MD5_Update(&ctx, data.mkupModel.data(), data.mkupModel.size());
        MD5_Update(&ctx, data.zoneModel.data(), data.zoneModel.size());
        MD5_Update(&ctx, data.textModel.data(), data.textModel.size());
        MD5_Update(&ctx, data.sentModel.data(), data.sentModel.size());

        unsigned char res[MD5_DIGEST_LENGTH];
        MD5_Final(res, &ctx);

        return TString(reinterpret_cast<char*>(res), MD5_DIGEST_LENGTH);
    }

    // Later we may want to implement automatic codec creation
    // For example, we may create a YT pipeline that processes certain data and outputs trained codecs
    // Then we will need to monitor these outputs, download the codecs and use them here
    // The next two functions should be changed accordingly

    TCodec::DictsData LoadData(TStringBuf ver) {
        TCodec::DictsData data;
        data.mkupModel = NResource::Find("mkup_model");
        data.zoneModel = NResource::Find("zone_model");
        data.textModel = NResource::Find("text_model");
        data.sentModel = NResource::Find("sentinfo_model");

        Y_ENSURE(ver == CalculateCodecsHash(data));
        return data;
    }

    TCodecWithInfo TCodecWithInfo::GetLatest() {
        TCodec::DictsData data;
        data.mkupModel = NResource::Find("mkup_model");
        data.zoneModel = NResource::Find("zone_model");
        data.textModel = NResource::Find("text_model");
        data.sentModel = NResource::Find("sentinfo_model");

        return TCodecWithInfo(data);
    }

    TCodec& GetInstance(TStringBuf ver) {
        auto& storage = *Singleton<CodecsStorage>();
        const auto codecIt = storage.find(ver);
        if (codecIt == storage.end()) {
            return storage.emplace(ver, LoadData(ver)).first->second;
        }
        return codecIt->second;
    }

    TCodec& GetInstance(TStringBuf ver, const TCodec::DictsData& data) {
        return Singleton<CodecsStorage>()->try_emplace(ver, data).first->second;
    }

    TCodecWithInfo::TCodecWithInfo(TStringBuf ver)
        : info({.Version=TString(ver)})
        , codec(GetInstance(ver)) {
    }

    TCodecWithInfo::TCodecWithInfo(const TCodec::DictsData& data)
        : info({.Version=CalculateCodecsHash(data)})
        , codec(GetInstance(info.Version, data)) {
    }

    TCodec::TCodec(const DictsData& data) {
        // Register all custom codec classes here prior to loading any codec
        RegisterOffroadCodecs();

        MarkupCodec_ = NCodecs::RestoreCodecFromCodecInfo(NCodecs::LoadCodecInfoFromString(data.mkupModel));
        WeightZonesCodec_ = NCodecs::RestoreCodecFromCodecInfo(NCodecs::LoadCodecInfoFromString(data.zoneModel));
        SentencesCodec_ = NCodecs::RestoreCodecFromCodecInfo(NCodecs::LoadCodecInfoFromString(data.textModel));
        BlocksCodec_ = NCodecs::RestoreCodecFromCodecInfo(NCodecs::LoadCodecInfoFromString(data.sentModel));
    }
}
