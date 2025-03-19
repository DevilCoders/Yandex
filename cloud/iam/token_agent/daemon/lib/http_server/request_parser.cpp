#include "request_parser.hpp"

namespace NHttpServer {
    TRequestParser::TRequestParser()
        : State{EState::MethodStart} 
    {
    }

    void TRequestParser::Reset() {
        State = EState::MethodStart;
    }

    TRequestParser::EParseResult TRequestParser::Consume(TRequest& request, char input) {
        switch (State) {
            case EState::MethodStart:
                if (!IsChar(input) || IsControl(input) || IsSpecial(input)) {
                    return EParseResult::Bad;
                } else {
                    State = EState::Method;
                    request.Method.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::Method:
                if (input == ' ') {
                    State = EState::Uri;
                    return EParseResult::Indeterminate;
                } else if (!IsChar(input) || IsControl(input) || IsSpecial(input)) {
                    return EParseResult::Bad;
                } else {
                    request.Method.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::Uri:
                if (input == ' ') {
                    if (request.Uri.Parse(request.RawUri) != NUri::TState::ParsedOK) {
                        return EParseResult::Bad;
                    }

                    State = EState::HttpVersionH;
                    return EParseResult::Indeterminate;
                } else if (IsControl(input)) {
                    return EParseResult::Bad;
                } else {
                    request.RawUri.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::HttpVersionH:
                if (input == 'H') {
                    State = EState::HttpVersionT1;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionT1:
                if (input == 'T') {
                    State = EState::HttpVersionT2;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionT2:
                if (input == 'T') {
                    State = EState::HttpVersionP;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionP:
                if (input == 'P') {
                    State = EState::HttpVersionSlash;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionSlash:
                if (input == '/') {
                    State = EState::HttpVersionMajorStart;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionMajorStart:
                if (IsDigit(input)) {
                    request.HttpVersionMajor = input - '0';
                    State = EState::HttpVersionMajor;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionMajor:
                if (input == '.') {
                    State = EState::HttpVersionMinorStart;
                } else if (IsDigit(input)) {
                    request.HttpVersionMajor = request.HttpVersionMajor * 10 + input - '0';
                } else {
                    return EParseResult::Bad;
                }
                return EParseResult::Indeterminate;

            case EState::HttpVersionMinorStart:
                if (IsDigit(input)) {
                    request.HttpVersionMinor = input - '0';
                    State = EState::HttpVersionMinor;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HttpVersionMinor:
                if (input == '\r') {
                    State = EState::ExpectingNewline1;
                } else if (IsDigit(input)) {
                    request.HttpVersionMinor = request.HttpVersionMinor * 10 + input - '0';
                } else {
                    return EParseResult::Bad;
                }
                return EParseResult::Indeterminate;

            case EState::ExpectingNewline1:
                if (input == '\n') {
                    State = EState::HeaderLineStart;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HeaderLineStart:
                if (input == '\r') {
                    State = EState::ExpectingNewline3;
                    return EParseResult::Indeterminate;
                } else if (!request.Headers.empty() && IsWhitespace(input)) {
                    State = EState::HeaderLws;
                    return EParseResult::Indeterminate;
                } else if (!IsChar(input) || IsControl(input) || IsSpecial(input)) {
                    return EParseResult::Bad;
                } else {
                    request.Headers.emplace_back();
                    request.Headers.back().Name.push_back(input);
                    State = EState::HeaderName;
                    return EParseResult::Indeterminate;
                }

            case EState::HeaderLws:
                if (input == '\r') {
                    State = EState::ExpectingNewline2;
                    return EParseResult::Indeterminate;
                } else if (IsWhitespace(input)) {
                    return EParseResult::Indeterminate;
                } else if (IsControl(input)) {
                    return EParseResult::Bad;
                } else {
                    State = EState::HeaderValue;
                    request.Headers.back().Value.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::HeaderName:
                if (input == ':') {
                    State = EState::SpaceBeforeHeaderValue;
                    return EParseResult::Indeterminate;
                } else if (!IsChar(input) || IsControl(input) || IsSpecial(input)) {
                    return EParseResult::Bad;
                } else {
                    request.Headers.back().Name.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::SpaceBeforeHeaderValue:
                if (input == ' ') {
                    State = EState::HeaderValue;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::HeaderValue:
                if (input == '\r') {
                    State = EState::ExpectingNewline2;
                    return EParseResult::Indeterminate;
                } else if (IsControl(input)) {
                    return EParseResult::Bad;
                } else {
                    request.Headers.back().Value.push_back(input);
                    return EParseResult::Indeterminate;
                }

            case EState::ExpectingNewline2:
                if (input == '\n') {
                    State = EState::HeaderLineStart;
                    return EParseResult::Indeterminate;
                }
                return EParseResult::Bad;

            case EState::ExpectingNewline3:
                return (input == '\n') ? EParseResult::Good : EParseResult::Bad;

            default:
                return EParseResult::Bad;
        }
    }

    bool TRequestParser::IsChar(int c) {
        return c >= 0 && c <= 127;
    }

    bool TRequestParser::IsControl(int c) {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    bool TRequestParser::IsWhitespace(int c) {
        return c == ' ' || c == '\t';
    }

    bool TRequestParser::IsSpecial(int c) {
        switch (c) {
            case '(':
            case ')':
            case '<':
            case '>':
            case '@':
            case ',':
            case ';':
            case ':':
            case '\\':
            case '"':
            case '/':
            case '[':
            case ']':
            case '?':
            case '=':
            case '{':
            case '}':
            case ' ':
            case '\t':
                return true;

            default:
                return false;
        }
    }

    bool TRequestParser::IsDigit(int c) {
        return c >= '0' && c <= '9';
    }
}
