#include <nginx/modules/strm_packager/src/common/drm_info_util.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/uuid_util.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/base64/base64.h> // Base64StrictDecode

#include <util/stream/mem.h>

namespace NStrm::NPackager {
    TCommonDrmInfo GetCommonDrmInfo(TRequestWorker& request, const ui64 segmentIndex) {
        const auto& config = request.Config;

        TCommonDrmInfo result;

        result.Enabled = config.DrmEnable.Defined() && request.GetComplexValue<bool>(*config.DrmEnable);
        result.WholeSegmentAes128 = result.Enabled && config.DrmWholeSegmentAes128.Defined() && request.GetComplexValue<bool>(*config.DrmWholeSegmentAes128);

        result.AllowDrmContentUnencrypted = config.AllowDrmContentUnencrypted.Defined() && request.GetComplexValue<bool>(*config.AllowDrmContentUnencrypted);

        const auto emptyInfo = NThreading::MakeFuture<TMaybe<TDrmInfo>>();

        result.Info4Muxer = emptyInfo;
        result.Info4Sender = emptyInfo;

        if (!result.Enabled) {
            return result;
        }

        Y_ENSURE(config.DrmRequestUri.Defined());

        const auto info = request.CreateSubrequest(
            TSubrequestParams{
                .Uri = (TString)request.GetComplexValue(*config.DrmRequestUri),
            },
            NGX_HTTP_OK);

        NThreading::TFuture<TMaybe<TDrmInfo>> gotInfo = info.Apply([aes128 = result.WholeSegmentAes128, segmentIndex](const NThreading::TFuture<TBuffer>& future) -> TMaybe<TDrmInfo> {
            const TBuffer& buf = future.GetValue();
            TMemoryInput bufInput(buf.Data(), buf.Size());

            const NJson::TJsonValue jval = NJson::ReadJsonTree(&bufInput, /*throwOnError = */ true);
            Y_ENSURE(jval.IsArray());
            Y_ENSURE(jval.GetArraySafe().size() == 1);
            Y_ENSURE(jval.GetArraySafe()[0].IsMap());
            const NJson::TJsonValue::TMapType& map = jval.GetArraySafe()[0].GetMapSafe();

            TDrmInfo res;

            NJson::TJsonValue const* const iv = map.FindPtr("iv");
            if (iv) {
                res.IV = Base64StrictDecode(iv->GetStringSafe());
                Y_ENSURE(res.IV->size() == 16);
            }
            res.Key = Base64StrictDecode(map.at("key").GetStringSafe());
            res.KeyId = Base64StrictDecode(map.at("key_id").GetStringSafe());

            Y_ENSURE(res.Key.size() == 16);
            Y_ENSURE(res.KeyId.size() == 16);

            NJson::TJsonValue const* const psshArr = map.FindPtr("pssh");
            if (psshArr) {
                Y_ENSURE(psshArr->IsArray());
                for (const NJson::TJsonValue& pssh : psshArr->GetArraySafe()) {
                    Y_ENSURE(pssh.IsMap());
                    const NJson::TJsonValue::TMapType& psshMap = pssh.GetMapSafe();

                    TDrmInfo::TPssh newpssh;
                    newpssh.SystemId = Uuid2Bytestring(psshMap.at("uuid").GetStringSafe());
                    newpssh.Data = Base64StrictDecode(psshMap.at("data").GetStringSafe());

                    res.Pssh.push_back(newpssh);
                }
            }

            if (aes128 && !iv) {
                res.IV = TString(16, 0);
                for (size_t i = 0; i < sizeof(segmentIndex); ++i) {
                    (*res.IV)[15 - i] = ui8((segmentIndex >> (8 * i)) & 0xff);
                }
            }

            return res;
        });

        if (result.WholeSegmentAes128) {
            result.Info4Sender = gotInfo;
        } else {
            result.Info4Muxer = gotInfo;
        }

        return result;
    }

}
