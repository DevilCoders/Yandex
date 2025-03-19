#include "manager.h"

namespace NCS {
    namespace NSecret {

        bool TFakeManager::DoEncode(TVector<TDataToken>& data, const bool /*writeIfNotExists*/) const {
            for (auto&& d : data) {
                d.SetSecretId(Base64Encode(d.SerializeToJson().GetStringRobust()));
            }
            return true;
        }

        bool TFakeManager::DoDecode(TVector<TDataToken>& data, const bool /*forceCheckExistance*/) const {
            for (auto&& d : data) {
                const TString sData = Base64Decode(d.GetSecretId());
                NJson::TJsonValue jsonData;
                if (!NJson::ReadJsonFastTree(sData, &jsonData)) {
                    TFLEventLog::Error("cannot parse secret as json");
                    return false;
                }
                if (!d.DeserializeFromJson(jsonData) || !d.GetData()) {
                    TFLEventLog::Error("cannot parse from json");
                    return false;
                }
            }
            return true;
        }

    }
}
