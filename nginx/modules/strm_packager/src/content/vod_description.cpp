#include <nginx/modules/strm_packager/src/content/vod_description.h>
#include <nginx/modules/strm_packager/src/content/vod_description_details.h>

#include <strm/plgo/pkg/proto/vod/description/v1/description.pb.h>
#include <library/cpp/json/json_reader.h>
#include <util/string/builder.h>

namespace NStrm::NPackager {
    TBuffer ParseVodDescription(TRequestWorker& request, const TBuffer& buffer) {
        const auto jsonString = TString(buffer.Data(), buffer.Size());
        NJson::TJsonValue parsed;
        NJson::TParserCallbacks callback(parsed, true);
        auto in = TStringInput(jsonString);
        NJson::ReadJson(&in, &callback);
        const NJson::TJsonValue::TMapType& mapItems = parsed.GetMap();

        int version = 0;
        for (auto it = mapItems.begin(); it != mapItems.end(); ++it) {
            const TString& key = it->first;
            if (key != "version" && key != "Version") {
                continue;
            }

            Y_ENSURE(it->second.IsInteger());
            version = it->second.GetInteger();
            break;
        }

        flatbuffers::FlatBufferBuilder builder;
        switch (version) {
            case 1:
                NVodDescriptionDetails::ParseV1VodDescription(jsonString, builder);
                break;
            case 2:
                NVodDescriptionDetails::ParseV2VodDescription(jsonString, builder);
                break;
            default:
                Y_ENSURE(false, "vod description version " << version << " not supported");
        }

        const auto data = request.GetPoolUtil<TBuffer>().New(
            (const char*)builder.GetBufferPointer(),
            builder.GetSize());
        return *data;
    }
}
