#pragma once

#include <util/generic/string.h>
#include <util/generic/map.h>

struct TParsedDocStorageConfig {
    TString RecognizeLibraryFile;
    TString ParserConfig;
    bool UseHTML5Parser = false;
    TMap<TString, TString> ParserConfigs;

    TParsedDocStorageConfig() {
    }
};
