#pragma once

#include <string>
#include <vector>

#include <library/cpp/uri/uri.h>

#include "header.hpp"

namespace NHttpServer {
    struct TRequest {
        std::string Method;
        std::string RawUri;
        NUri::TUri Uri;
        int HttpVersionMajor;
        int HttpVersionMinor;
        std::vector<THeader> Headers;
    };
}
