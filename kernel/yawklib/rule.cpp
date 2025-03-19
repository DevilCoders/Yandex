#include "rule.h"
#include "phrase_analyzer.h"
#include "phrase_iterator.h"
#include "wtrutil.h"
#include <library/cpp/charset/wide.h>
#include <ysite/yandex/doppelgangers/normalize.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NYawk {
////////////////////////////////////////////////////////////////////////////////////////////////////
static IPhraseRecognizer *CreateRecognizerFromPattern(const TUtf16String &pattern, IRecognizerFactory *factory)
{
    THolder<IPhraseIterator> it(CreatePhraseIterator(pattern));
    return it->CreateRecognizer(factory);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TRule::TRule(IRecognizerFactory *factory)
    : RecognizerFactory(factory)
{
    TokenNames.push_back("opt");
    Tokens.emplace_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRule::AddToken(const TString &name, const TUtf16String &description)
{
    for (size_t n = 1; n < Tokens.size(); ++n) {
        if (TokenNames[n] == name) {
            Tokens[n].push_back(description);

            return;
        }
    }
    TokenNames.push_back(name);
    Tokens.emplace_back();
    Tokens.back().push_back(description);

}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRule::AddOptionalToken(const TUtf16String &description)
{
    Tokens[0].push_back(description);

}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TRule::HasToken(const TString &name) const
{
    for (size_t n = 1; n < Tokens.size(); ++n)
        if (TokenNames[n] == name)
            return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRule::GetUsedFileNames(TVector<TString> *res) const
{
    if (RecognizerFactory) {
        RecognizerFactory->GetUsedFileNames(res);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRule::Init() const
{
    if (Recognizers.size() != Tokens.size())
        Recognizers.resize(Tokens.size());
    for (size_t n = 0; n < Tokens.size(); ++n) {
        const TVector<TUtf16String> &token = Tokens[n];
        if (!token.empty() && !Recognizers[n]) {
            if (token.size() == 1)
                Recognizers[n] = CreateRecognizerFromPattern(token[0], RecognizerFactory);
            else {
                TVector<TPtr<IPhraseRecognizer> > parts(token.size());
                for (size_t k = 0; k < token.size(); ++k)
                    parts[k] = CreateRecognizerFromPattern(token[k], RecognizerFactory);
                Recognizers[n] = Union(parts);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef ui64 TToken;
struct TObliviousProgress
{
    TVector<TToken> Variants;
    TToken Common;
    TObliviousProgress()
        : Common(TToken(-1))
    {
    }
    void MakeFirstStep() {
        Variants.push_back(0);
        Common = 0;
    }
    bool Done(ui64 bit) const {
        return Common & bit;
    }
    bool AddTokens(const TObliviousProgress &other, ui64 additionalBit, size_t, size_t, size_t) {
        // oh crap, it's O(n*m); but that's ok as n, m are usually about 0-2
        for (size_t n = 0; n < other.Variants.size(); ++n) {
            ui64 variant = other.Variants[n];
            if (variant & additionalBit)
                continue;
            variant |= additionalBit;
            if (!Contains(Variants, variant)) {
                Variants.push_back(variant);
                if (Variants.size() >= 1024)
                    return false;
            }
        }
        Common &= (other.Common | additionalBit);
        return true;
    }
    void SaveResult(const TUtf16String&, size_t, size_t, size_t, const TVector<TString>&, IResultConsumer*) const {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static size_t GetMissingOne(TToken mask, TToken fullMask)
{
    if (mask == fullMask)
        return 0;
    size_t res = 1;
    for (size_t bit = 1; bit & fullMask; ++res, bit *= 2)
        if ((mask | bit) == fullMask)
            return res;
    return (size_t)-1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TStatefulProgress
{
    TVector<TToken> Variants;
    struct TMatchData {
        const TStatefulProgress *Parent;
        size_t Idx;
        size_t TokenId;
        size_t From;
        size_t To;
    };
    TVector<TMatchData> MatchData;
    void MakeFirstStep() {
        Variants.push_back(0);
        MatchData.emplace_back();
        MatchData.back().Parent = nullptr;
    }
    bool Done(ui64) const {
        return false;
    }
    bool HasOptional(size_t from, size_t to) const
    {
        for (TVector<TMatchData>::const_iterator it = MatchData.begin(); it != MatchData.end(); ++it) {
            if (it->TokenId == 0 && it->From == from && it->To == to)
                return true;
        }
        return false;
    }
    bool AddTokens(const TStatefulProgress &other, ui64 additionalBit, size_t tokenId, size_t from, size_t to) {
        for (size_t n = 0; n < other.Variants.size(); ++n) {
            ui64 variant = other.Variants[n];
            if (variant & additionalBit)
                continue;
            if (!additionalBit) {
                const TMatchData &md = other.MatchData[n];
                // two optional tokens one after one, if they can be combined - ignore this match
                if (!md.TokenId && HasOptional(md.From, to))
                    continue;
            }
            if (Variants.size() > 1024)
                return false;
            variant |= additionalBit;
            Variants.push_back(variant);
            MatchData.emplace_back();
            TMatchData &p = MatchData.back();
            p.Parent = &other;
            p.Idx = n;
            p.TokenId = tokenId;
            p.From = from;
            p.To = to;
        }
        return true;
    }
    void SaveResult(const TUtf16String &req, size_t idx, size_t from, size_t lastMatched, const TVector<TString> &tokenNames, IResultConsumer *res) const {
        size_t dstIdx = tokenNames.size() - 1;
        if (lastMatched) {
            --dstIdx;
            res->Part(dstIdx, tokenNames[lastMatched], req.substr(from));
        }
        const TStatefulProgress *parent = this;
        while (parent) {
            const TMatchData &md = parent->MatchData[idx];
            if (md.TokenId) {
                --dstIdx;
                res->Part(dstIdx, tokenNames[md.TokenId], req.substr(md.From, md.To - md.From));
            }
            parent = md.Parent;
            idx = md.Idx;
        }
        Y_ASSERT(dstIdx == 0);
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool AcceptsOrdered(const TVector<TToken> &variants, ui64 bit)
{
    return Contains(variants, bit-1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class TProgress>
bool DoRecognize(const TUtf16String &req, const TVector<size_t> &wordStarts,
    const TVector<TObj<IPhraseRecognizer> > &recognizers, bool ordered,
    const TVector<TString> &tokenNames, IResultConsumer *res)
{
    Y_ASSERT(!recognizers.empty());
    TVector<TProgress> progress(wordStarts.size());
    progress[0].MakeFirstStep();

    bool added = false;
    bool exponentialBlowupReported = false;
    size_t nTokens = recognizers.size() - 1;
    TToken fullMask = ((ui64)1 << nTokens) - 1;
    for (size_t n = 0; n < wordStarts.size(); ++n) {
        const TProgress &cur = progress[n];
        if (cur.Variants.empty())
            continue;
        // full phrase
        size_t from = wordStarts[n];
        bool wasMatchWithLastOptional = false;
        for (size_t m = 0; m < progress[n].Variants.size(); ++m) {
            TToken variant = progress[n].Variants[m];
            size_t missingOne = GetMissingOne(variant, fullMask);
            if (missingOne != (size_t)-1) {
                Y_ASSERT(missingOne < recognizers.size());
                IPhraseRecognizer *r = recognizers[missingOne];
                if (r && r->IsMatch(req, from, req.size())) {
                    if (!res)
                        return true;
                    else {
                        if (missingOne == 0)
                            wasMatchWithLastOptional = true;
                        added = true;
                        res->NewMatch(nTokens);
                        cur.SaveResult(req, m, from, missingOne, tokenNames, res);
                        res->EndMatch();
                    }
                }
            }
        }
        // partial progress
        for (size_t m = n + 1; m < wordStarts.size(); ++m) {
            size_t to = wordStarts[m] - 1;
            for (size_t k = 0; k < recognizers.size(); ++k) {
                ui64 bit;
                if (k)
                    bit = ((ui64)1) << (k - 1);
                else {
                    if (wasMatchWithLastOptional)
                        continue;
                    bit = 0;
                }
                if (ordered && bit) {
                    if (!AcceptsOrdered(cur.Variants, bit))
                        continue;
                }
                if (cur.Done(bit))
                    continue;
                IPhraseRecognizer *r = recognizers[k];
                if (r && r->IsMatch(req, from, to)) {
                    bool success = progress[m].AddTokens(cur, bit, k, from, to);
                    if (!success && !exponentialBlowupReported) {
                        exponentialBlowupReported = true;
                        Cerr << "Warning: exponential blowup for request " << req << Endl;
                    }
                }
            }
        }
    }
    return added;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TVectorConsumer: public IResultConsumer {
    TVector<TResult> *Res;

    void NewMatch(size_t numParts) override {
        Res->emplace_back(numParts);
    }

    void Part(size_t partIdx, const TString &token, const TUtf16String &value) override {
        TResultPart &dst = Res->back()[partIdx];
        dst.TokenName = token;
        dst.TokenValue = value;
    }

    void EndMatch() override {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TRule::Recognize(const TUtf16String &phrase, bool ordered, TVector<TResult> *res) const
{
    TVectorConsumer vc;
    vc.Res = res;
    return Recognize(phrase, ordered, &vc);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TRule::IsMatch(const TUtf16String &phrase, bool ordered) const
{
    return Recognize(phrase, ordered, (IResultConsumer*)nullptr);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TRule::Recognize(const TUtf16String &phraseInitial, bool ordered, IResultConsumer *res) const
{

    if (Recognizers.size() < 2) {
        ythrow yexception() << "yawk error: no obligatory tokens specified";
    }

    TDoppelgangersNormalize normalizer(false, false, false);
    TUtf16String req = normalizer.Normalize(phraseInitial, false);

    // special optimized case: only one obligatory token
    if (Recognizers.size() == 2 && !Recognizers[0]) {
        IPhraseRecognizer *r = Recognizers[1];
        bool b = r->IsMatch(req, 0, req.size());
        if (b && res) {
            res->NewMatch(1);
            res->Part(0, TokenNames[1], req);
            res->EndMatch();
        }
        return b;
    }

    // multiple tokens
    TVector<size_t> wordStarts;
    wordStarts.push_back(0);

    for (size_t n = 0; n < req.size(); ++n)
        if (req[n] == ' ')
            wordStarts.push_back(n + 1);

    if (wordStarts.size() < Tokens.size() - 1)
        return false;

    if (!res) {
        return DoRecognize<TObliviousProgress>(req, wordStarts, Recognizers, ordered, TokenNames, nullptr);
    } else {
        return DoRecognize<TStatefulProgress>(req, wordStarts, Recognizers, ordered, TokenNames, res);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool CanBeNamePart(char c) {
    switch (c) {
        case 0:
        case ':':
        case '[':
        case ']':
        case '{':
        case '}':
        case '/':
        case '\\':
        case '*':
            return false;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsUnique(const TVector<TString> &tokenNames, int id) {
    TString val = Sprintf("%d", id);
    for (size_t n = 0; n < tokenNames.size(); ++n)
        if (val == tokenNames[n])
            return false;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRule::CreateRule(const TVector<TString> &tokens, bool isUtf) {
    TVector<TString> tokenNames;
    TVector<TString> tokenVals;
    for (int param = 0; param < tokens.ysize(); ++param) {
        TString arg = tokens[param];
        size_t semicPos = 0;
        for (; CanBeNamePart(arg[semicPos]); ++semicPos)
            ;
        if (arg[semicPos] == ':') {
            tokenNames.push_back(arg.substr(0, semicPos));
            tokenVals.push_back(arg.substr(semicPos + 1));
        } else {
            tokenNames.emplace_back();
            tokenVals.push_back(arg);
        }
    }
    int uniqueId = 1;
    for (size_t n = 0; n < tokenNames.size(); ++n) {
        TString &name = tokenNames[n];
        if (name.empty()) {
            while (!IsUnique(tokenNames, uniqueId))
                ++uniqueId;
            name = Sprintf("%d", uniqueId);
        }
        TUtf16String val = isUtf? UTF8ToWide(tokenVals[n]) : CharToWide(tokenVals[n], csYandex);
        if (name == "opt" || name == "optional")
            AddOptionalToken(val);
        else
            AddToken(name, val);
    }

    Init();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NYawk;
