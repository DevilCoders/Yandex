#include "uri.h"

#include <util/stream/str.h>
#include <library/cpp/uri/encode.h>

TString NAntiRobot::EncodeUriComponent(const TString& value) {
    TStringStream out;
    NUri::TEncoder::Encode(out, value);
    return out.Str();
}
