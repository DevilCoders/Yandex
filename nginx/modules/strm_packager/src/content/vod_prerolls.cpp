#include <nginx/modules/strm_packager/src/content/vod_prerolls.h>

#include <nginx/modules/strm_packager/src/base/config.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <util/string/split.h>

namespace NStrm::NPackager {
    class TClipPrefixMap {
    public:
        TClipPrefixMap() {
            const TString data = NResource::Find("clip_prefix_map.json");
            TMemoryInput dataInput(data.Data(), data.Size());

            const NJson::TJsonValue jval = NJson::ReadJsonTree(&dataInput, /*throwOnError = */ true);
            Y_ENSURE(jval.IsMap());
            for (const auto& [key, value] : jval.GetMapSafe()) {
                TString& prefix = ClipPrefixMap[key];
                prefix = value.GetStringSafe();
                if (prefix.back() != '/') {
                    prefix += '/';
                }
            }
        }

        const TMap<TString, TString>& Get() const {
            return ClipPrefixMap;
        }

    private:
        TMap<TString, TString> ClipPrefixMap;
    };

    TVector<TString> GetPrerolls(const TRequestWorker& request) {
        const TLocationConfig& config = request.Config;

        Y_ENSURE(config.Prerolls.Defined() == config.PrerollsClipId.Defined());

        if (config.Prerolls.Empty()) {
            return {};
        }

        const TStringBuf prerollsClipIdStr = request.GetComplexValue(*config.PrerollsClipId);
        const TStringBuf prerollsStr = request.GetComplexValue(*config.Prerolls);

        TVector<TStringBuf> prerolls;
        TVector<TStringBuf> ids;
        Split(prerollsStr, ";", prerolls);
        Split(prerollsClipIdStr, ";", ids);

        Y_ENSURE(prerolls.size() == ids.size());

        TVector<TString> res(prerolls.size());

        constexpr char jsonExt[] = ".json";

        for (size_t i = 0; i < res.size(); ++i) {
            TString const* const clipPrefix = Singleton<TClipPrefixMap>()->Get().FindPtr(ids[i]);
            Y_ENSURE(clipPrefix);

            res[i].reserve(clipPrefix->length() + prerolls[i].length() + sizeof(jsonExt));
            res[i] = *clipPrefix;
            for (const char c : prerolls[i]) {
                if (c == '*') {
                    res[i] += '/';
                } else {
                    res[i] += c;
                }
            }
            res[i] += jsonExt;
        }

        return res;
    }

    void CheckPrerollsConfig(const TLocationConfig& config) {
        Y_ENSURE(config.Prerolls.Defined() == config.PrerollsClipId.Defined());
        if (config.Prerolls.Defined()) {
            Singleton<TClipPrefixMap>();
        }
    }

    ui32 SelectPrerollTrackId(
        decltype(&NDescription::TDescription::VideoSets) descGetTracksSet,
        const NDescription::TDescription& prerollDescription,
        const NDescription::TDescription& mainDescription,
        const ui32 mainIndex) {
        constexpr ui32 langPartLen = 3;

        TMaybe<ui32> result;
        ui32 bestLangDistance = 1e9;

        TStringBuf mainLang;
        TStringBuf mainParams;

        for (const auto& set : *(mainDescription.*descGetTracksSet)()) {
            for (const auto& track : *set->Tracks()) {
                if (track->Id() == mainIndex) {
                    mainParams = TStringBuf(track->Params()->data(), track->Params()->size());
                    mainLang = TStringBuf(set->Language()->data(), Min(langPartLen, set->Language()->size()));
                    break;
                }
            }
            if (mainParams) {
                break;
            }
        }
        Y_ENSURE_EX(mainParams, THttpError(404, TLOG_WARNING) << "failed to find track with id " << mainIndex);

        for (const auto& set : *(prerollDescription.*descGetTracksSet)()) {
            const TStringBuf lang(set->Language()->data(), Min(langPartLen, set->Language()->size()));

            const ui32 langDistance =
                (lang == mainLang) ? 0
                : (lang == "rus")  ? 1
                : (lang == "und")  ? 2
                                   : 3;

            if (result.Defined() && langDistance >= bestLangDistance) {
                continue;
            }

            for (const auto& track : *set->Tracks()) {
                const TStringBuf params(track->Params()->data(), track->Params()->size());

                if (params == mainParams) {
                    result = track->Id();
                    bestLangDistance = langDistance;
                    break;
                }
            }
        }

        Y_ENSURE_EX(result.Defined(), THttpError(400, TLOG_WARNING) << "no matching prerolls");

        return *result;
    }

}
