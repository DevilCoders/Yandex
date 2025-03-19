#pragma once

namespace NSnippets
{
    class TConfig;
    class TWordSpanLen;
    class TSnip;
    class TSingleSnip;
    namespace NMaxFit
    {
        // returns fragment prefix that fits in maxLength
        TSnip GetTSnippet(const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSingleSnip& fragment, float maxLength);
    }
}
