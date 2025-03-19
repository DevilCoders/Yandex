#include <cstring>
#include <cmath>
#include <util/generic/ptr.h>
#include <library/cpp/charset/recyr.hh>
#include <util/string/printf.h>
#include <library/cpp/html/face/event.h>
#include <library/cpp/html/spec/tags.h>
#include <kernel/indexer/face/inserter.h>

#include "syn_numerator.h"

// we want to work with russian-only subtokens
// IsRussianLetter checks whether the lower-case character c is allowed
// if so, returns it's code in csYandex, otherwise returns 0
inline ui8 IsRussianLetter(wchar16 c)
{
    if (c >= 0x430 && c <= 0x44F)
        return (ui8)(c - 0x430 + 0xE0);
    if (c == 0x451) // YO
        return 0xB8; // todo: may be normalize to E?
    if (IsHyphen(c) || IsDash(c))
        return '-';
    return 0;
}

void TSynonymsHandler::OnMoveInput(const THtmlChunk& c, const TZoneEntry*, const TNumerStat&)
{
    int type = c.flags.type;
    if ((type == PARSED_MARKUP) && c.flags.markup != MARKUP_IMPLIED) {
        const NHtml::TTag* tag = c.Tag;
        if (!tag)
            return;
        HT_TAG tagId = tag->id();
        if (!WasBody && tagId == HT_BODY) {
            WasBody = true;
            WordsCur = 0;
            NumTotal = NumN = NumN1 = 0;
            Crit1 = 0;
            NumTotalCur = NumNcur = NumN1cur = 0;
            Crit1cur = 0;
            NumLatinLetters = 0;
            Start = true;
        }
        bool is_link = false;
        if (tagId == HT_A) {
            // scan for 'href' attribute
            for (size_t i = 0; i < c.AttrCount; ++i) {
                const NHtml::TAttribute& a = c.Attrs[i];
                if (a.Name.Leng == 4) {
                    const char* t = c.text + a.Name.Start;
                    if ((*(ui32*)t | 0x20202020) == *(ui32*)"href") {
                        is_link = true;
                        break;
                    }
                }
            }
        }
        if (tagId == HT_A && !is_link || tagId == HT_B || tagId == HT_I ||
            tagId == HT_U || tagId == HT_SUP || tagId == HT_SUB ||
            tagId == HT_P || tagId == HT_BR || tagId == HT_PRE ||
            tagId >= HT_H1 && tagId <= HT_H6 || tagId == HT_STRONG ||
            tagId == HT_FONT || tagId == HT_S || tagId == HT_ABBR || tagId == HT_SCRIPT) {
            /* good tag, ignore it */
            if (tagId == HT_P || tagId == HT_BR) {
                Start = true;
                CurWord = nullptr;
            }
        } else
            CloseBlock();
        LastIsNumber = false;
    }
}

void TSynonymsHandler::OnSpaces(TBreakType, const wchar16* data, unsigned length, const TNumerStat&)
{
    const wchar16* end;
    if (!length)
        return;
    // numbers followed by '.' or ')' are block boundaries
    if (LastIsNumber && (data[0] == '.' || data[0] == ')'))
        CloseBlock();
    LastIsNumber = false;
    for (end = data + length; data < end; data++) {
        if (!IsSpace(*data)) {
            Start = true;
            return;
        }
    }
}

void TSynonymsHandler::OnTokenStart(const TWideToken& wtok, const TNumerStat&)
{
    LastIsNumber = false;
    TAutoPtr<char, TDeleteArray> recodedWord;
    char* word;
    char* p;
    if (wtok.Leng < sizeof(TempString))
        p = TempString;
    else
        recodedWord.Reset(p = new char[wtok.Leng]);
    const wchar16* ptr = wtok.Token;
    const wchar16* end = ptr + wtok.Leng;
    // get russian subtokens
    word = p;
    for (; ptr < end; ptr++) {
        wchar16 cur = ToLower(*ptr);
        if (cur >= 'a' && cur <= 'z')
            NumLatinLetters++;
        if (LastIsNumber && (cur == '.' || cur == ')'))
            CloseBlock();
        LastIsNumber = IsDigit(cur);
        ui8 c = IsRussianLetter(cur);
        if (c) {
            if (p != word || c != '-') // ignore hyphens at the beginning
                *p++ = c;
        } else {
            if (p != word) {
                ProcessOneWord(word, p - word);
                p = word;
            }
            Start = true;
        }
    }
    if (p != word)
        ProcessOneWord(word, p - word);
}

void TSynonymsHandler::ProcessOneWord(const char* word, size_t len)
{
    ui64 id;
    bool was2;
    const TTestSyn::TSetU* newword;
    if (Tester->Paradigms.Find(word, len, &id)) {
        was2 = true;
        newword = id ? &Tester->ParadigmsVariants[id - 1] : nullptr;
    } else {
        was2 = false;
        newword = nullptr;
    }
    WordsCur++;
    if (!Start) {
        NumTotalCur++;
        if (Was1 && was2)
            NumNcur++;
        if (CurWord && newword) {
            NumN1cur++;
            float cor, prod;
            Tester->CalcCorrelation(CurWord, newword, cor, prod);
            CorForPairsCur.push_back(cor);
            if (cor < 0.0001f)
                Crit1cur++;
        }
    }
    CurWord = newword;
    Was1 = was2;
    Start = false;
}

void TSynonymsHandler::CloseBlock(void)
{
    if (!WordsCur)
        return;
    // increase variables for Syn7b factor
    NumTotal += NumTotalCur;
    NumN += NumNcur;
    NumN1 += NumN1cur;
    Crit1 += Crit1cur;
    if (WordsCur >= 200) {
        // add pairs for Syn8b and Syn9a factors
        CorForPairs.insert(CorForPairs.end(), CorForPairsCur.begin(), CorForPairsCur.end());
    }
    WordsCur = 0;
    NumTotalCur = 0;
    NumNcur = 0;
    NumN1cur = 0;
    Crit1cur = 0;
    Start = true;
    Was1 = false;
    CurWord = nullptr;
    CorForPairsCur.clear();
}

void TSynonymsHandler::OnTextEnd(const IParsedDocProperties*, const TNumerStat& )
{
    CloseBlock();
    TCoordsWithDist texts[11];
    int nbest = Tester->GetNearestTexts(NumTotal, NumN, NumN1, 10, texts);
    int i;
    double sum = 0, sumsq = 0;
    for (i = 0; i < nbest; i++) {
        sum += texts[i].b1;
        sumsq += texts[i].b1 * texts[i].b1;
    }
    double er = sum / nbest;
    double dr = (sumsq - sum * sum / nbest) / (nbest - 1);
    double crit1patch = Crit1 * Crit1 / (Crit1 + 4.0);
    CorStat = (dr >= 1e-10) ? (crit1patch - er) / sqrt(dr) :
        ((crit1patch <= er) ? 0.0 : 100000.0);
    int l = (int)ceil(sum / 10) + 2;
    double* worstpairs = new double[l + 1];
    TAutoPtr<double, TDeleteArray> worstpairs_owner(worstpairs);
    int numworst = 0;
    for (TVector<double>::const_iterator it = CorForPairs.begin(); it != CorForPairs.end(); ++it) {
        worstpairs[numworst] = *it;
        PushHeap(worstpairs, worstpairs + numworst + 1);
        if (numworst < l)
            numworst++;
        else
            PopHeap(worstpairs, worstpairs + numworst + 1);
    }
    CorForPairs.clear();
    sum = 0;
    for (i = 0; i < numworst; i++)
        sum += worstpairs[i];
    CorStat2 = numworst ? sum / numworst : -1.0;
}

double TSynonymsHandler::GetSyn7b() const
{
    if (!NumTotal)
        return 1;
    if (CorStat < 0)
        return 0;
    else
        return CorStat / (CorStat + 9);
}

double TSynonymsHandler::GetSyn8b() const
{
    if (CorStat2 < 0)
        return 1;
    double val = CorStat2;
    if (val < 1e-6)
        val = 1e-6;
    if (val > 1e-3)
        val = 1e-3;
    return -1 - log(val) / log(1e3);
}

double TSynonymsHandler::GetSyn9a() const
{
    if (CorStat2 < 0)
        return 1;
    double val = CorStat2 * 1e4;
    return (val < 1.0) ? val : 1.0;
}

void TSynonymsHandler::InsertFactors(IDocumentDataInserter& inserter) const {
    inserter.StoreErfDocAttr("Syn7bV", Sprintf("%f", GetSyn7b()));
    inserter.StoreErfDocAttr("Syn8bV", Sprintf("%f", GetSyn8b()));
    inserter.StoreErfDocAttr("Syn9aV", Sprintf("%f", GetSyn9a()));
    inserter.StoreErfDocAttr("SynPercentBadWordPairs", Sprintf("%f", GetPercentBadWordPairs()));
    inserter.StoreErfDocAttr("SynNumBadWordPairs", Sprintf("%f", GetNumBadWordPairs()));
    inserter.StoreErfDocAttr("NumLatinLetters", Sprintf("%f", GetNumLatinLetters()));
}
