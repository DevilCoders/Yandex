#pragma once

#include <string>
#include <utility>

namespace NHttpServer {
    struct THeader {
        THeader() = default;

        THeader(std::string  name, std::string  value)
            : Name(std::move(name))
            , Value(std::move(value))
        {
        }

        std::string Name;
        std::string Value;
    };
}
