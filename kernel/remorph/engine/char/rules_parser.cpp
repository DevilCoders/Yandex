#include "rules_parser.h"

#include "regex_lexer.h"
#include "regex_token.h"

#include <kernel/remorph/common/source.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/engine/engine.h>

#include <library/cpp/unicode/set/set.h>

#include <util/charset/wide.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>


namespace NReMorph {
namespace NPrivate {

namespace {

struct TRegexContext {
    TWtringBuf Data;
    TStringBuf Mods;
    TSourceLocation Location;
    TSourceLocation ModsLocation;
    const TParseTokenDataFormat& DataFormat;
    NRemorph::TVectorTokens Tokens;
    TLiteralTable& LiteralTable;
    const TDefinitions& Definitions;
    bool CaseInsensitive;

    bool GroupStart;

    explicit TRegexContext(const TParseToken& regex, const TParseToken* mods, TLiteralTable& literalTable, const TDefinitions& definitions)
        : Data(regex.Data)
        , Mods()
        , Location(regex.Location)
        , ModsLocation()
        , DataFormat(regex.DataFormat)
        , Tokens()
        , LiteralTable(literalTable)
        , Definitions(definitions)
        , CaseInsensitive(false)
        , GroupStart(false)
    {
        if (mods) {
            Mods = mods->RawData;
            ModsLocation = mods->Location;
        }
    }

    inline TParsingError UnexpectedTokenError(const TRegexToken& token) {
        return TParsingError(token.Location) << "unexpected regex token: " << token;
    }
};

inline TParsingError UnexpectedTokenError(const TParseToken& token) {
    return TParsingError(token.Location) << "unexpected token " << token;
}

inline void RequireToken(const TParseToken& token, EParseTokenType tokenType) {
    if (token.Type != tokenType) {
        throw UnexpectedTokenError(token);
    }
}

inline NRemorph::TTokenPtr MakeCharToken(const TRegexContext& context, wchar32 c) {
    if (context.CaseInsensitive) {
        const ::NUnicode::NPrivate::TProperty& p = ::NUnicode::NPrivate::CharProperty(c);
        if (p.Lower || p.Upper || p.Title) {
            NUnicode::TUnicodeSet set;
            set.Add(c)
                .Add(static_cast<wchar32>(c + p.Lower))
                .Add(static_cast<wchar32>(c + p.Upper))
                .Add(static_cast<wchar32>(c + p.Title))
            ;
            return NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(context.LiteralTable.Add(set)));
        }
    }
    return NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(context.LiteralTable.Add(c)));
}

inline NRemorph::TSourcePosPtr MakeParserSourcePos(const TRegexToken& token) {
    return NRemorph::TSourcePosPtr(new NRemorph::TSourcePos(
        TString(token.Location.Name),
        token.Location.Pos.Line + 1,
        token.LocationEnd.Pos.Line + 1,
        token.Location.Pos.Column + 1,
        token.LocationEnd.Pos.Column + 1
    ));
}

inline TSourceLocation RefineLocation(const TSourceLocation& location, const NRemorph::TSourcePosPtr& sourcePos) {
    if (!sourcePos) {
        return location;
    }

    return TSourceLocation(location.Name, TSourcePos(sourcePos->BegLine - 1, sourcePos->BegCol - 1));
}

inline std::pair<int, int> ParseRegexRepeater(const TRegexToken& token) {
    std::pair<int, int> result = ::std::make_pair(0, -1);
    TString repeater = ::WideToUTF8(token.Data);
    size_t comma = repeater.find_first_of(',');
    if (comma != TString::npos) {
        result.first = ::FromString(TStringBuf(repeater.data(), comma));
        if (comma < repeater.size() - 1) {
            result.second = ::FromString(TStringBuf(repeater.data() + comma + 1, repeater.size() - comma - 1));
        }
    } else {
        result.first = ::FromString(repeater);
        result.second = result.first;
    }
    return result;
}

inline NRemorph::TAstNodePtr GetDefinition(const TRegexContext& context, const TRegexToken& reference) {
    TString name = ::WideToUTF8(reference.Data);
    TDefinitions::const_iterator found = context.Definitions.find(name);
    if (found == context.Definitions.end()) {
        throw TParsingError(reference.Location) << "no such definition: " << name.Quote();
    }
    return found->second;
}

inline void ParseRegexToken(TRegexContext& context, const TRegexToken& token) {
    NRemorph::TTokenPtr parserToken;

    wchar32 symbol;
    NUnicode::TUnicodeSet unicodeSet;
    std::pair<int, int> repeater;

    if (context.GroupStart && (token.Type != RTT_GROUP)) {
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLParen(MakeParserSourcePos(token)));
        context.Tokens.push_back(parserToken);
        context.GroupStart = false;
    }

    switch (token.Type) {
    case RTT_EOS:
        return;
    case RTT_SYMBOL:
        parserToken = MakeCharToken(context, token.Symbol);
        break;
    case RTT_QUOTED_PAIR:
        unicodeSet.Clear();
        switch (ResolveUnicodeQuotedPair(token.Symbol, symbol, unicodeSet)) {
        case NUnicode::UQPT_SYMBOL:
            parserToken = MakeCharToken(context, symbol);
            break;
        case NUnicode::UQPT_SET:
            if (context.CaseInsensitive) {
                unicodeSet.MakeCaseInsensitive();
            }
            parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(context.LiteralTable.Add(unicodeSet)));
            break;
        }
        break;
    case RTT_SET:
        try {
            unicodeSet.Parse(token.Data);
        } catch (const yexception& error) {
            throw TParsingError(token.Location) << "incorrect unicode set " << token.Data << ": " << error.what();
        }
        if (context.CaseInsensitive) {
            unicodeSet.MakeCaseInsensitive();
        }
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(context.LiteralTable.Add(unicodeSet)));
        break;
    case RTT_ANY:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Any)));
        break;
    case RTT_GROUP:
        if (!context.GroupStart) {
            throw context.UnexpectedTokenError(token);
        }
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLParen(MakeParserSourcePos(token), ::WideToUTF8(token.Data)));
        context.GroupStart = false;
        break;
    case RTT_LPAREN:
        context.GroupStart = true;
        return;
    case RTT_RPAREN:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenRParen(MakeParserSourcePos(token)));
        break;
    case RTT_ALTERNATIVE:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenPipe);
        break;
    case RTT_ASTERISK:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenAsterisk);
        break;
    case RTT_PLUS:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenPlus);
        break;
    case RTT_QUESTION:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenQuestion);
        break;
    case RTT_BEGIN:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Bos)));
        break;
    case RTT_END:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenLiteral(NRemorph::TLiteral(0, NRemorph::TLiteral::Eos)));
        break;
    case RTT_REPEAT:
        repeater = ParseRegexRepeater(token);
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenRepeat(repeater.first, repeater.second));
        break;
    case RTT_REFERENCE:
        parserToken = NRemorph::TTokenPtr(new NRemorph::TTokenRE(GetDefinition(context, token).Get()));
        break;
    default:
        throw context.UnexpectedTokenError(token);
    }

    Y_ASSERT(parserToken);

    if (!parserToken->SourcePos) {
        parserToken->SourcePos = MakeParserSourcePos(token);
    }

    context.Tokens.push_back(parserToken);
}

inline void ParseRegexMod(TRegexContext& context, char mod) {
    switch (mod) {
    case 'i':
        context.CaseInsensitive = true;
        break;
    default:
        throw TParsingError(context.ModsLocation) << "unsupported regex mod '" << mod << "'";
    }
}

inline NRemorph::TAstNodePtr ParseRegex(TRegexContext& context) {
    TRegexLexer lexer(context.Data, context.Location, context.DataFormat);
    for (const char* p = context.Mods.data(); p != context.Mods.data() + context.Mods.size(); ++p) {
        ParseRegexMod(context, *p);
    }

    TRegexToken token;
    do {
        token = lexer.GetToken();
        ParseRegexToken(context, token);
    } while (token.Type != RTT_EOS);

    NRemorph::TAstNodePtr ast;

    try {
        ast = NRemorph::Parse(context.LiteralTable, context.Tokens);
    } catch (const NRemorph::TUnexpectedToken& error) {
        throw TParsingError(RefineLocation(context.Location, error.GetSourcePos())) << "unexpected regex token " << error.GetExpandedToken().Quote();
    } catch (const yexception& error) {
        throw TParsingError(context.Location) << "incorrect regex: " << error.what();
    }

    Y_ASSERT(ast);

    return ast;
}

inline NRemorph::TNFAPtr CompileRegex(TRegexContext& context) {
    NRemorph::TAstNodePtr ast = ParseRegex(context);

    NRemorph::TNFAPtr nfa;

    try {
        nfa = NRemorph::CompileNFA(context.LiteralTable, ast.Get());
    } catch (const yexception& error) {
        throw TParsingError(context.Location) << "failed to compile regex: " << error.what();
    }

    Y_ASSERT(nfa);

    return nfa;
}

}

TRulesParser::TRulesParser(const TFileParseOptions& fileParseOptions)
    : PreParser(fileParseOptions)
    , Result()
    , Definitions()
{

}

TRulesParser::TRulesParser(const TStreamParseOptions& streamParseOptions)
    : PreParser(streamParseOptions)
    , Result()
    , Definitions()
{
}

TRulesParser::TResult TRulesParser::Parse() {
    TPreParser::TStatementHandler statementHandler = std::bind(&TRulesParser::HandleStatement, this, std::placeholders::_1);
    PreParser.Parse(statementHandler);

    return Result;
}

void TRulesParser::HandleStatement(const TParseStatement& statement) {
    if (statement.Head == "rule") {
        ParseRule(statement.Body);
    } else if (statement.Head == "def") {
        ParseDef(statement.Body);
    } else {
        throw TParsingError(statement.Location) << "unexpected statement " << statement;
    }
}

void TRulesParser::ParseRule(const TParseTokens& tokens) {
    TParseTokens::const_iterator iToken = tokens.begin();

    double weight = 1.0;
    const TParseToken* regexMods = nullptr;

    RequireToken(*iToken, PTT_ID);
    TStringBuf name = (iToken++)->RawData;
    if (iToken->Type == PTT_LPAREN) {
        ++iToken;
        RequireToken(*iToken, PTT_NUMBER);
        weight = (iToken++)->Number;
        RequireToken(*(iToken++), PTT_RPAREN);
    }
    RequireToken(*(iToken++), PTT_EQUALS);
    RequireToken(*iToken, PTT_REGEX);
    const TParseToken& regex = *(iToken++);
    if (iToken->Type == PTT_REGEX_MOD) {
        regexMods = &*(iToken++);
    }
    RequireToken(*iToken, PTT_SEMICOLON);

    TRegexContext regexContext(regex, regexMods, *Result.LiteralTable, Definitions);
    Result.Rules.push_back(std::make_pair(TString(name), weight));
    Result.NFAs.push_back(CompileRegex(regexContext));
}

void TRulesParser::ParseDef(const TParseTokens& tokens) {
    TParseTokens::const_iterator iToken = tokens.begin();

    const TParseToken* regexMods = nullptr;

    RequireToken(*iToken, PTT_ID);
    const TParseToken& nameToken = *(iToken++);
    TStringBuf name = nameToken.RawData;
    RequireToken(*(iToken++), PTT_EQUALS);
    RequireToken(*iToken, PTT_REGEX);
    const TParseToken& regex = *(iToken++);
    if (iToken->Type == PTT_REGEX_MOD) {
        regexMods = &*(iToken++);
    }
    RequireToken(*iToken, PTT_SEMICOLON);

    if (Definitions.find(name) != Definitions.end()) {
        throw TParsingError(nameToken.Location) << "duplicated definition " << TString(name).Quote();
    }

    TRegexContext regexContext(regex, regexMods, *Result.LiteralTable, Definitions);
    Definitions.emplace(TString(name), ParseRegex(regexContext));
}

} // NPrivate
} // NReMorph
