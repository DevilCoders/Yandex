#pragma once

#include <library/cpp/config/config.h>

//yconf format support. safe, but non-natural mapping

namespace NConfig {
    extern const TStringBuf JSON_ATTRIBUTES;
    extern const TStringBuf JSON_DIRECTIVES;
    extern const TStringBuf JSON_SECTIONS;

    TConfig ParseRawYConf(IInputStream& in);
    void Json2RawYConf(IOutputStream& out, TStringBuf json);
    void DumpYandexCfg(const TConfig& cfg, IOutputStream& out);
    TConfig FromYandexCfg(IInputStream& in, const TGlobals& g = TGlobals());
    TConfig ReadYandexCfg(TStringBuf in, const TGlobals& g = TGlobals());
}
