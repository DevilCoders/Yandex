#pragma once
#include <library/cpp/langs/langs.h>
#include <util/generic/vector.h>

struct TZonedString;

namespace NSnippets {

class TConfig;
class TExtraSnipAttrs;
struct ISnippetDebugOutputHandler;

struct TEnhanceSnippetConfig {
    const TConfig& Config;
    const bool IsByLink;
    const ELanguage DocLangId;
    ISnippetDebugOutputHandler* OutputHandler;
    const TString& Url;
};

void EnhanceSnippet(const TEnhanceSnippetConfig& cfg, TVector<TZonedString>& snipVec, TExtraSnipAttrs& extraSnipAttrs);

}
