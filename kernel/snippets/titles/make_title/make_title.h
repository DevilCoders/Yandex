#pragma once

#include <kernel/snippets/idl/enums.h>

#include <util/generic/string.h>

class TRichRequestNode;

namespace NSnippets
{
    class TSnipTitle;
    class TConfig;
    class TQueryy;

    class TMakeTitleOptions {
    public:
        bool AllowBreakMultiToken;
        ETitleGeneratingAlgo TitleGeneratingAlgo;
        ETextCutMethod TitleCutMethod;

        // Options for TitleCutMethod == TCM_PIXEL
        float MaxTitleLengthInRows;
        int PixelsInLine;
        float TitleFontSize;

        // Options for TitleCutMethod == TCM_SYMBOL
        int MaxTitleLen;

        // Options for definitions
        int MaxDefinitionLen;
        ETitleDefinitionMode DefinitionMode;
        TString Url;
        bool HostnamesForDefinitions;

    public:
        explicit TMakeTitleOptions(const TConfig& cfg);
    };

    // Important!!! Need to refresh pixel width data every time
    // in snippets/smartcut/pixel_length.cpp when changes SERP design!
    TSnipTitle MakeTitle(const TUtf16String& source, const TConfig& cfg, const TQueryy& query,
        const TMakeTitleOptions& options);

    // Cuts title to fit in maxPixelLength pixels when rendered with
    //     font-family: Arial; font-size: fontSize px;
    // NOTE: richtreeRoot may be nullptr
    // NOTE: to prevent overflow in 100% of cases maxPixelLength should be
    //     slightly less than container width
    TUtf16String MakeTitleString(const TUtf16String& source, const TRichRequestNode* richtreeRoot,
        int maxPixelLength, float fontSize);

    // Cuts title to fit in maxTitleLen symbols
    TUtf16String MakeTitleString(const TUtf16String& source, const TRichRequestNode* richtreeRoot, size_t maxTitleLen);
}
