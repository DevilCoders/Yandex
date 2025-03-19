#include "nodebase.h"

const TUtf16String TRequestNodeBase::GetPunctAfter() const {
    static const TUtf16String good_puncts(u".;:,!?%-'@*/");
    static const TUtf16String double_comma(u",,");
    static const TUtf16String double_colon(u"::");
    static const TUtf16String attr_zone(u"->");
    static const TUtf16String percent(u"%");
    static const TUtf16String dash(u"-");

    TUtf16String punct(TextAfter);
    size_t endpos = punct.find_first_not_of(good_puncts.c_str());

    if (endpos == 0 ||
         punct == percent ||
         punct.StartsWith(double_comma) ||
         punct.StartsWith(double_colon) ||
         punct.StartsWith(attr_zone))
        return TUtf16String();

    if (endpos == TUtf16String::npos) {
        // strip final minus or percent - too likely to be an operator on the next word
        size_t len = punct.length();
        if (len > 1) {
            if (punct.EndsWith(dash) || punct.EndsWith(percent))
                punct.resize(len - 1);
        }
    } else {
        punct.resize(endpos);
    }

    return punct;
}

bool TRequestNodeBase::Compare(const TRequestNodeBase& node) const {
    return ((OpInfo == node.OpInfo)
        && (WildCard == node.WildCard)
        && (TextName == node.TextName)
        && (GetSafeTokSpan() == node.GetSafeTokSpan())
        && (TextAfter == node.TextAfter)
        && (TextBefore == node.TextBefore)
        && (Text == node.Text)
        // && (TokenTypes == node.GetTokenTypes()) isn't serialized.
        && (AttrValueHi == node.AttrValueHi)
        && (Necessity == node.Necessity)
        && (KeyPrefix == node.KeyPrefix)
        && (NGrammBase == node.NGrammBase)
        && (NGrammPercent == node.NGrammPercent)
        && (PhraseType == node.PhraseType && Quoted == node.Quoted)
        && (ReverseFreq == node.ReverseFreq)
        && (Parens == node.Parens)
        && (FactorName == node.FactorName)
        && (FactorValue == node.FactorValue));
}

void TRequestNodeBase::Swap(TRequestNodeBase& node) {
    ::DoSwap(TextName, node.TextName);
    ::DoSwap(Text, node.Text);
    ::DoSwap(AttrValueHi, node.AttrValueHi);
    ::DoSwap(PhraseType, node.PhraseType);

    ::DoSwap(TokenTypes, node.TokenTypes);
    ::DoSwap(SubTokens, node.SubTokens);
    ::DoSwap(KeyPrefix, node.KeyPrefix);
    ::DoSwap(NGrammBase, node.NGrammBase);
    ::DoSwap(NGrammPercent, node.NGrammPercent);
    ::DoSwap(Quoted, node.Quoted);
    ::DoSwap(WildCard, node.WildCard);

    ::DoSwap(TextAfter, node.TextAfter);
    ::DoSwap(TextBefore, node.TextBefore);
    ::DoSwap(FactorName, node.FactorName);
    ::DoSwap(FactorValue, node.FactorValue);

    ::DoSwap(OpInfo, node.OpInfo);
    ::DoSwap(Necessity, node.Necessity);
    ::DoSwap(ReverseFreq, node.ReverseFreq);
    ::DoSwap(Parens, node.Parens);
}

void TRequestNodeBase::RemovePrefix(TCharSpan& span) {
    if (SubTokens.size() > 0 && SubTokens[0].PrefixLen) {
        const ui16 prefixLen = SubTokens[0].PrefixLen;
        Text.remove(0, prefixLen);
        SubTokens[0].PrefixLen = 0;
        for (size_t i = 0; i < SubTokens.size(); ++i)
            SubTokens[i].Pos -= prefixLen;
        span.Pos += prefixLen;
        span.Len -= prefixLen;
    }
}
