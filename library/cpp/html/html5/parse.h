#pragma once

#include "options.h"

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

class IParserResult;
class IInputStream;
class IZeroCopyInput;

namespace NHtml5 {
    void ParseHtml(TStringBuf html, IParserResult* result, TStringBuf url);
    void ParseHtml(TStringBuf html, IParserResult* result);
    void ParseHtml(TStringBuf html, IParserResult* result, const TParserOptions& opts);

    /**
 * @note call NormalizeUtfInput before parse html document.
 */
    void ParseHtml(TBuffer& doc, IParserResult* result, TStringBuf url);
    void ParseHtml(TBuffer& doc, IParserResult* result);

    void ParseHtml(IInputStream* input, IParserResult* parserResult);
    void ParseHtml(IZeroCopyInput* input, IParserResult* parserResult);

}
