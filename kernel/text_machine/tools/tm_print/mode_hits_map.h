#pragma once

enum class EInputFormat {
    ProtoPool,
    Base64Hits
};

template <bool IsRequest>
int MainHitsMap(int argc, const char* argv[]);
