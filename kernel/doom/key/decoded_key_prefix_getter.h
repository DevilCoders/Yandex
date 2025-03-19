#pragma once

#include <util/generic/fwd.h>

namespace NDoom {


struct TDecodedKeyPrefixGetter {
    TStringBuf operator()(const TStringBuf& key) const {
        return key.SubStr(0, key.find('\x01'));
    }
};


} // namespace NDoom
