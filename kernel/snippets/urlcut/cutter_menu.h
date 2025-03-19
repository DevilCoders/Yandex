#pragma once

#include "tokens.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    class THilitedString;

    class TMenuCutter {
    private:
        class TMenuCutterImpl;
        THolder<TMenuCutterImpl> Impl;
    public:
        TMenuCutter(TTokenList& tokens, i32 maxLen);
        ~TMenuCutter();
        THilitedString GetUrl();
    };
}
