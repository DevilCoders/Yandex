#pragma once
#include "reqattrlist.h"
#include <util/system/defaults.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <library/cpp/charset/doccodes.h>
#include <util/charset/wide.h>
#include <util/string/join.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/formtype.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <library/cpp/tokenizer/tokenizer.h>

enum TOperType {
    // binary operators:
    oAnd,        //!< & (sentence) and && (document) operators
    oAndNot,     //!< ~ (sentence) and ~~ (document) operators
    oOr,         //!< | operator
    oRefine,     //!< <- operator
    oRestrDoc,   //!< << operator
    oWeakOr,     //!< ^ operator
    // unary operators:
    oZone,
    oRestrictByPos,
    // Tag of subtree
    oUnused = 12
};

enum TPhraseType {
    PHRASE_NONE         = 0,  // not a phrase
    PHRASE_MARK         = 1,  // obsolete, UnMarkRichTree stuff
    PHRASE_MULTIWORD    = 2,
    PHRASE_FLOAT        = 3,  // obsolete, UnMarkRichTree stuff
    PHRASE_PHRASE       = 5,  // последовательность слов, разделенных пробелами
    PHRASE_USEROP       = 6,  // последовательность слов, разделенных расстояниями, указанными пользователем - &&, &, /(1,1), ~~  и т.п.
    PHRASE_REGION       = 12, // a region word node
    PHRASE_NUMBERSEQ    = 13,
    PHRASE_MARKSEQ      = 16,
    PHRASE_MULTISEQ     = 17,
    PHRASE_CLASSIFIED   = 18
};

enum TCompareOper {
   cmpLT,
   cmpLE,
   cmpEQ,
   cmpGE,
   cmpGT,
};

enum TNodeNecessity {
   nMUSTNOT = -1,  // -word
   nDEFAULT = 0,
   nMUST = 1,      // +word
   nSHOULD = 2,    // "user defined" refine
};

const TString& ToString(TPhraseType);
const TString& ToString(TOperType);
const TString& ToString(TCompareOper);
const TString& ToString(TNodeNecessity);

bool FromString(const TString& name, TPhraseType& ret);
bool FromString(const TString& name, TOperType& ret);
bool FromString(const TString& name, TCompareOper& ret);
bool FromString(const TString& name, TNodeNecessity& ret);

//! describes parent nodes in the request tree
struct TOpInfo {
   i32 Lo;                  //!< represents distance to the left between words and phrases: /-l, /(l r)
   i32 Hi;                  //!< represents distance to the right between words and phrases: /+r, /(l r)
   TOperType Op;            //!< represents type of this parent node: OR, AND, ZONE, ATTR, etc.
   ui16 Level;              //!< represents level of an operation: sentence - 1 ('|', '~'), document - 2 ('&&', '~~', '<->')
   TCompareOper CmpOper;    //!< represents a comparison operators: ':', ':>', ':>=', ':<', ':<='
   bool Arrange;            //!< @c true if '$$$' is specified for requested zone

   bool operator == (const TOpInfo &op) const {
       return Lo == op.Lo && Hi == op.Hi && Op == op.Op && Level == op.Level && CmpOper == op.CmpOper && Arrange == op.Arrange;
   }
};
                                    //Lo Hi Op    Level  cmpOper Arrange
const TOpInfo DefaultPhraseOpInfo = { 0, 0, oAnd,     2, cmpEQ,  false};
const TOpInfo DefaultQuoteOpInfo  = { 1, 1, oAnd,     0, cmpEQ,  false};
const TOpInfo DocumentPhraseOpInfo= { 0, 0, oAnd,     2, cmpEQ,  false};
const TOpInfo SentencePhraseOpInfo= { 0, 0, oAnd,     1, cmpEQ,  false};
const TOpInfo ZeroDistanceOpInfo  = { 0, 0, oAnd,     0, cmpEQ,  false};

const TOpInfo DefaultAndNotOpInfo        = { 0, 0, oAndNot,        2, cmpEQ,  false};
const TOpInfo DefaultZoneOpInfo          = { 0, 0, oZone,          0, cmpEQ,  false};
const TOpInfo DefaultRestrDocOpInfo      = { 0, 0, oRestrDoc,      0, cmpEQ,  false};
const TOpInfo DefaultRefineOpInfo        = { 0, 0, oRefine,        2, cmpEQ,  false};
const TOpInfo DefaultRestrictByPosOpInfo = { 0, 0, oRestrictByPos, 0, cmpEQ,  false};
const TOpInfo DisjunctionOpInfo          = { 1, 1, oOr,            0, cmpEQ,  false};

enum EWildCardFlags {
    WILDCARD_NONE,
    WILDCARD_PREFIX,
    WILDCARD_SUFFIX,
    WILDCARD_INFIX // INFIX == PREFIX | SUFFIX
};


namespace NRichTreeProtocol {
    class TRequestNodeBase;
}

namespace NSearchRequest {
    struct TOperatorsFound {
        enum EGeneralFlags {
            No     = 0,
            Unary  = 1 << 0,
            Binary = 1 << 1,
            Url    = 1 << 2,  //url:, inurl:, host:, etc, not filled while parsing
        };
        /// Operator value starting shift for a variable holding EOperatorsFound
        static constexpr unsigned OpShift = 3;
        static_assert(OpShift + oUnused < sizeof(unsigned) * 8, "Inconsistent shift base value");

        operator bool() const {
            return Value != No;
        }

        static constexpr unsigned ToFlag(TOperType op) {
            return 1 << (op + OpShift);
        }

        static constexpr unsigned ToFlag(EGeneralFlags flag) {
            return flag;
        }

        static constexpr unsigned ToFlag(unsigned flag) {
            return flag;
        }

        static constexpr unsigned ToFlag(TOperatorsFound flags) {
            return flags.Value;
        }

        template <typename T>
        TOperatorsFound& operator |=(T flag) {
            Value |= ToFlag(flag);
            return *this;
        }

        template <typename... T>
        static constexpr unsigned CombineFlags(T... flags) {
            return (ToFlag(flags) | ...);
        }

        template <typename T>
        bool Has(T flag) const {
            return Value & ToFlag(flag);
        }

        template <typename... TMore>
        bool HasMoreThan(TMore... flags) const {
            auto inFlags = CombineFlags(flags...);
            return (Value & ~inFlags) != 0;
        }

        TString DebugString() const {
            if (Value == 0)
                return "No";
            TVector<TString> ret;
            if (Has(Unary))
                ret.push_back("Unary");
            if (Has(Binary))
                ret.push_back("Binary");
            if (Has(Url))
                ret.push_back("Url");
            for (int op = 0; op < oUnused; ++op) {
                TOperType opEnum = static_cast<TOperType>(op);
                if (Has(opEnum))
                    ret.push_back(ToString(opEnum));
            }
            return JoinSeq(" ", ret);
        }

        unsigned GetValue() const noexcept {
            return Value;
        }

    private:
        unsigned Value = No;
    };
}

struct TRequestNodeBase {
    friend void SerializeBase(NRichTreeProtocol::TRequestNodeBase& message, bool humanReadable, const TRequestNodeBase& node);
    friend void DeserializeBase(const NRichTreeProtocol::TRequestNodeBase& message, TRequestNodeBase& node);

private:
    TUtf16String TextName;
    TUtf16String Text;
    TUtf16String AttrValueHi;
    TPhraseType PhraseType;

protected:
    NTokenClassification::TTokenTypes TokenTypes;

    TTokenStructure SubTokens;

    TVector<ui64> KeyPrefix;
    ui8 NGrammBase = 0;
    ui8 NGrammPercent = 100;

    bool Quoted;
    EWildCardFlags WildCard;

    TUtf16String TextAfter;
    TUtf16String TextBefore;
    TUtf16String FactorName;
    float FactorValue;

public:
    TOpInfo OpInfo;
    TNodeNecessity Necessity;
    i32 ReverseFreq;

    bool Parens; //!< @todo make this member private

protected:
    TRequestNodeBase()
        : PhraseType(PHRASE_NONE)
        , TokenTypes(0)
        , Quoted(false)
        , WildCard(WILDCARD_NONE)
        , FactorValue(.0f)
        , OpInfo(DefaultPhraseOpInfo)
        , Necessity(nDEFAULT)
        , ReverseFreq(-1)
        , Parens(false)
    {}

    void Swap(TRequestNodeBase& node);

    void RemovePrefix(TCharSpan& span);

public:
    bool Compare(const TRequestNodeBase& node) const;

    TOperType Op() const {
        return OpInfo.Op;
    }

    ui16 GetLevel() const {
        if (OpInfo.Op == oAnd || OpInfo.Op == oAndNot) {
            //задание расстояния (в том числе и нулевого!) понижает level на 1
            return (OpInfo.Lo || OpInfo.Hi || !OpInfo.Level ? OpInfo.Level + 1 : OpInfo.Level);
        }
        return OpInfo.Level;
    }

    TPhraseType GetPhraseType() const {
        return PhraseType;
    }

    void SetPhraseType(TPhraseType type) {
        PhraseType = type;
    }

    NTokenClassification::TTokenTypes GetTokenTypes() const {
        return TokenTypes;
    }

    ui8 GetNGrammBase() const {
        return NGrammBase;
    }

    void SetNGrammBase(ui8 value, ui8 ngrZoneSoftness) {
        NGrammBase = value;
        NGrammPercent = ngrZoneSoftness;
    }

    ui8 GetNGrammPercent() const {
        return NGrammPercent;
    }

    void SetTokenTypes(NTokenClassification::TTokenTypes tokenTypes) {
        TokenTypes = tokenTypes;
    }

    void SetQuoted(bool val = true) {
        Quoted = val;
    }

    bool IsQuoted() const {
        return Quoted;
    }

    inline const TVector<ui64>& GetKeyPrefix() const {
        return KeyPrefix;
    }

    inline void AddKeyPrefix(ui64 value) {
        KeyPrefix.push_back(value);
    }

    inline void SetKeyPrefix(const TVector<ui64>& value) {
        KeyPrefix = value;
    }

    //! returns @c true if this node is enclosed in quotes or parentheses
    inline bool IsEnclosed() const {
        return Parens || IsQuoted();
    }

    void SetZoneName(const TUtf16String& name, bool arrange) {
        TextName = name;
        SetTokSpan(TCharSpan(0, TextName.size()));
        OpInfo.CmpOper = cmpEQ;
        OpInfo.Arrange = arrange;
        Y_ASSERT(!Text && !AttrValueHi);
        Y_ASSERT(PhraseType == PHRASE_NONE);
    }

    void SetLeafWord(const TUtf16String& text, const TCharSpan& span, TPhraseType phraseType = PHRASE_NONE) {
        OpInfo = ZeroDistanceOpInfo;
        OpInfo.Op = oUnused;
        Text = text;
        TextName.remove();
        SetTokSpan(span);
        Y_ASSERT(!AttrValueHi);
        //Y_ASSERT(phraseType == PHRASE_NONE || phraseType == PHRASE_MARK || phraseType == PHRASE_FLOAT);
        SetPhraseType(phraseType);
    }

    void SetMultitoken(const TUtf16String& text, const TTokenStructure& tst, TPhraseType phraseType) {
        Y_ASSERT(phraseType == PHRASE_MULTIWORD || phraseType == PHRASE_MARKSEQ || phraseType == PHRASE_NUMBERSEQ);
        OpInfo = DefaultQuoteOpInfo;
        Text = text;
        TextName.remove();
        SubTokens = tst;
        SetPhraseType(phraseType);
        Y_ASSERT(!AttrValueHi);
    }

    void SetMultitoken(const TUtf16String& text, const TCharSpan& span, TPhraseType phraseType) {
        TTokenStructure tst;
        tst.push_back(span);
        SetMultitoken(text, tst, phraseType);
    }

    void SetPhrase(TPhraseType phraseType) {
        PhraseType = phraseType;
        TextName.remove();
        Text.remove();
        SubTokens.clear();
    }

    void SetAttrValues(const TUtf16String& name, const TUtf16String& valLo, const TUtf16String& valHi) {
        TextName = name;
        SetTokSpan(TCharSpan(0, TextName.size()));
        Text = valLo;
        AttrValueHi = valHi;
        OpInfo.Op = oUnused;
        Y_ASSERT(PHRASE_USEROP == PhraseType || PHRASE_NONE == PhraseType);
    }

    void SetRefineFactor(const TUtf16String& name, float val = 1.f) {
        Y_ASSERT(val >= 0.f && val <= 1.f);
        FactorName = name;
        FactorValue = val;
        Y_ASSERT(!TextName);
        Y_ASSERT(!Text);
        Y_ASSERT(PHRASE_USEROP == PhraseType || PHRASE_NONE == PhraseType); // UserOp - in binary tree, None - in rich tree
        Y_ASSERT(oRefine == OpInfo.Op);
    }

    const TCharSpan GetTokSpan() const { // if the node is a leaf or multitoken then it is word subspan without suffix/prefix
        if (SubTokens.empty()) {
            Y_ASSERT(!"There is no span");
            ythrow yexception() << "There is no span";
        }
        TCharSpan ret(SubTokens[0].Pos, SubTokens.back().EndPos() - SubTokens[0].Pos);
        ret.PrefixLen = SubTokens[0].PrefixLen;
        ret.SuffixLen = SubTokens.back().SuffixLen;
        return ret;
    }

    const TUtf16String& GetTextName() const {
        return TextName;
    }

    void SetTextName(const TUtf16String& textName) {
        TextName = textName;
    }

    const TUtf16String& GetText() const {
        return Text;
    }

    void SetText(const TUtf16String& text) {
        Text = text;
    }

    const TUtf16String GetPunctAfter() const;

    const TUtf16String& GetTextAfter() const {
        return TextAfter;
    }

    void SetTextAfter(const TUtf16String& textAfter) {
        TextAfter = textAfter;
    }

    const TUtf16String& GetTextBefore() const {
        return TextBefore;
    }

    void SetTextBefore(const TUtf16String& textBefore) {
        TextBefore = textBefore;
    }

    const TUtf16String& GetAttrValueHi() const {
        return AttrValueHi;
    }

    void SetWildCard(EWildCardFlags wc) {
        WildCard = wc;
    }

    EWildCardFlags GetWildCard() const {
        return WildCard;
    }

    const TTokenStructure& GetSubTokens() const {
        return SubTokens;
    }

    TWideToken GetMultitoken() const {
        if (!!TextName)
            return TWideToken(TextName.data(), TextName.size(), SubTokens);
        else if (!!Text)
            return TWideToken(Text.data(), Text.size(), SubTokens);
        return TWideToken();
    }

    const TUtf16String& GetFactorName() const {
        return FactorName;
    }

    float GetFactorValue() const {
        return FactorValue;
    }

    // Is node special tokenizer symbol - like infinity symbol or unicode emoji?
    // See SEARCH-3781
    bool IsSpecialToken() const {
        return IsSpecialTokenizerSymbol(GetText());
    }

private:
    void SetTokSpan(const TCharSpan& span) {
        SubTokens.clear();
        SubTokens.push_back(span);
    }
    TCharSpan GetSafeTokSpan() const {
        if (!SubTokens.empty())
            return GetTokSpan();
        return TCharSpan();
    }
};
inline bool IsClassifiedToken(const TRequestNodeBase& n) {
    return n.GetTokenTypes() != NTokenClassification::ETT_NONE;
}

template <typename NTokenClassification::ETokenType Type>
inline bool IsClassifiedToken(const TRequestNodeBase& n) {
    return n.GetTokenTypes() & Type;
}

inline bool AlwaysTrue(const TRequestNodeBase&) {
    return true;
}

inline bool IsWordOrMultitoken(const TRequestNodeBase& n) {
    return !n.GetTextName() && !!n.GetText();
}

inline bool IsMergedMark(const TRequestNodeBase& n) {
    // result of "mark merging"
    return n.GetPhraseType() == PHRASE_MARKSEQ || n.GetPhraseType() == PHRASE_NUMBERSEQ;
}

inline bool IsMultitoken(const TRequestNodeBase& n) {
    return PHRASE_MULTIWORD == n.GetPhraseType() || PHRASE_MULTISEQ == n.GetPhraseType()
        || IsMergedMark(n);
}

inline bool IsWord(const TRequestNodeBase& n) {
    return n.Op() != oAnd && IsWordOrMultitoken(n) && !IsMultitoken(n);
}

inline bool IsWordAndNotEmoji(const TRequestNodeBase& n) {
    return IsWord(n) && !IsAsciiEmojiPart(n.GetText());
}

inline bool IsMergedMarkOrWord(const TRequestNodeBase& n) {
    return IsMergedMark(n) || IsWord(n);
}

inline bool IsAttributeOrZone(const TRequestNodeBase& n) {
    return !!n.GetTextName();
}

inline bool IsAttribute(const TRequestNodeBase& n)  {
    return IsAttributeOrZone(n) && !!n.GetText();
}

inline bool IsZone(const TRequestNodeBase& n) {
    return IsAttributeOrZone(n) && !n.GetText();
}

inline bool IsAndOp(const TRequestNodeBase& n) {
    return oAnd == n.OpInfo.Op;
}

inline bool IsUserPhrase(const TRequestNodeBase& n) {
    return (IsAndOp(n) && (n.GetPhraseType() == PHRASE_PHRASE));
}

inline bool IsQuote(const TRequestNodeBase& n) {
    return n.IsQuoted();
}

inline bool IsOr(const TRequestNodeBase& n) {
    return oOr == n.OpInfo.Op;
}

inline bool IsTitleZone(const TRequestNodeBase& n) {
    return IsZone(n) && n.GetTextName().compare(TITLE_STR) == 0;
}

inline bool IsAttribute(const TRequestNodeBase& n, const wchar16* value) {
    return IsAttribute(n) && n.GetTextName().compare(value) == 0;
}

inline bool IsInUrlAttribute(const TRequestNodeBase& n) {
    return IsAttribute(n, INURL_STR);
}

inline bool IsSiteAttribute(const TRequestNodeBase& n) {
    return IsAttribute(n, SITE_STR);
}

inline bool IsUrlAttribute(const TRequestNodeBase& n) {
    return IsAttribute(n, URL_STR);
}

inline bool IsLinkAttribute(const TRequestNodeBase& n) {
    return IsAttribute(n, LINK_STR);
}

inline bool IsAnchorZone(const TRequestNodeBase& n) {
    return IsZone(n) && n.GetTextName().compare(ANCHOR_STR) == 0;
}
