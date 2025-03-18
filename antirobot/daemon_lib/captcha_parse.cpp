#include "captcha_parse.h"

#include <antirobot/idl/captcha_response.pb.h>

#include <library/cpp/protobuf/json/json2proto.h>

#include <util/generic/yexception.h>

namespace NAntiRobot {
    TErrorOr<TCaptcha> TryParseApiCaptchaResponse(TStringBuf jsonResponse) {
        using namespace NProtobufJson;

        try {
            TCaptchaResponse response;
            TJson2ProtoConfig config;
            config.FieldNameMode = TJson2ProtoConfig::FieldNameLowerCase;

            Json2Proto(jsonResponse, response, config);

            Y_ENSURE(!response.GetImageUrl().empty(), "Captcha image url must be nonempty");

            return TCaptcha{response.GetToken(), response.GetImageUrl(), response.GetVoiceUrl(), response.GetVoiceIntroUrl()};
        } catch (...) {
            return TError(__LOCATION__ + yexception() << "Could not parse captcha json: " << jsonResponse << " " << CurrentExceptionMessage());
        }
    }
}
