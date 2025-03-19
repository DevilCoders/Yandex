#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>

namespace NSnippets
{
    class TConfig;

    ELanguage GetDocLanguage(const TDocInfos& docInfos, const TConfig& cfg);
}
