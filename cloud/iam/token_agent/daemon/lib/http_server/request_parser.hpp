#pragma once

#include <tuple>

#include "request.hpp"

namespace NHttpServer {
    class TRequestParser {
    public:
        TRequestParser();

        void Reset();

        enum class EParseResult {
            Good,
            Bad,
            Indeterminate
        };

        template<typename InputIterator>
        EParseResult parse(TRequest& request, InputIterator begin, InputIterator end) {
            while (begin != end) {
                EParseResult result = Consume(request, *begin++);
                if (result != EParseResult::Indeterminate) {
                    return result;
                }
            }
            return EParseResult::Indeterminate;
        }

    private:
        EParseResult Consume(TRequest &request, char input);

        static bool IsChar(int c);
        static bool IsWhitespace(int c);
        static bool IsControl(int c);
        static bool IsSpecial(int c);
        static bool IsDigit(int c);

        enum class EState {
            MethodStart,
            Method,
            Uri,
            HttpVersionH,
            HttpVersionT1,
            HttpVersionT2,
            HttpVersionP,
            HttpVersionSlash,
            HttpVersionMajorStart,
            HttpVersionMajor,
            HttpVersionMinorStart,
            HttpVersionMinor,
            ExpectingNewline1,
            HeaderLineStart,
            HeaderLws,
            HeaderName,
            SpaceBeforeHeaderValue,
            HeaderValue,
            ExpectingNewline2,
            ExpectingNewline3
        } State;
    };
}
