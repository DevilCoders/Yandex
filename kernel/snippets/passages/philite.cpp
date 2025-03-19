#include "passages.h"

#include <library/cpp/charset/wide.h>

#include <util/string/util.h>


constexpr unsigned TITLE_FIRST_PIECE_LEN = 40;

constexpr wchar16 INSERT_3DOTS[] = { ' ', '.', '.', '.', ' ', 0 };
constexpr size_t INSERT_3DOTS_LEN = Y_ARRAY_SIZE(INSERT_3DOTS) - 1;
const TUtf16String ELLIPSIS(INSERT_3DOTS, INSERT_3DOTS_LEN);

struct WordDescr
{//description of word in raw-painted test
    unsigned Beg;  //first symbol of the word, if painted, it = 0x1b
    unsigned End;  //last symbol
    char     Type; //relevance (0 - not painted)
};

struct PaintedPiece
{//description of part of the text which have given length
    unsigned FirstWord;   //first word of "piece"
    unsigned LastWord;    //last one (included)
    char     NumPainted;  //how many words painted
};


constexpr unsigned MAX_WORDS_NUM = 80; //Oh yes, I know that it cant be more than 64 words in the
                                   //sentence IF WE COUNT THEM BY Numerator + TNlpTokenizer. But
                                   //if we count words by spaces, it may happen that there are
                                   //more than 64 words. Such things happen. Why 80? 80 = 64 + smth.


int WhatSymbol(wchar16 Symbol)
{
    if (Symbol == HILIGHT_MARK)
        return 2;
    return IsAlnum(Symbol) ? 1 : 0;
}

//scans input Line, count words and fills WordsDescs
void ScanLine(TUtf16String& Line, WordDescr* WordsDescs, unsigned& NumWords, unsigned& NumPaintedWords) {
    bool InWord = false;
    NumWords = 0;
    NumPaintedWords = 0;
    const char *P;
    unsigned i = 0;
    for (; i < MAX_WORDS_NUM; i++)
        WordsDescs[i].Type = -1;
    for (i = 0; i < Line.size(); i++)
    {
        switch (WhatSymbol(Line[i]))
        {
            case 0:
                if (InWord && !WordsDescs[NumWords].Type)
                {
                    WordsDescs[NumWords++].End = i;
                    if (NumWords == MAX_WORDS_NUM)
                        return;
                    InWord = false;
                }
                break;
            case 1:
                if (!InWord)
                {
                    WordsDescs[NumWords].Beg = i;
                    WordsDescs[NumWords].Type = 0;
                }
                InWord = true;
                break;
            case 2:
                //painting mark
                if (i + 1 == Line.size()) {
                    // SEARCH-3549
                    return;
                }
                if (!InWord || !WordsDescs[NumWords].Type)
                {
                    P = strchr(HILIGHTS, WideCharToYandex.Tr(Line[i + 1]));
                    if (!P) {
                        Cerr << "[ERROR]: Internal error in hilighter at pos=" << i << " (" << Line << ")" << Endl;
                        // this means we hit unknown character that we cannot handle. This can occur when we try to
                        // highlight binary data (see SEARCH-2643), so bailing out is the best we can do here.
                        return;
                    }
                    NumPaintedWords++;
                    WordsDescs[NumWords].Type = (char)(P -  HILIGHTS) + 1;
                    WordsDescs[NumWords].Beg = i;
                    InWord = true;
                } else {
                    WordsDescs[NumWords++].End = i + 2;
                    if (NumWords == MAX_WORDS_NUM)
                        return;
                    InWord = false;
                }
                i++;
                break;
        }
    }
    if (InWord)
        WordsDescs[NumWords++].End = Line.size();
}

void DeterminePaintedPieces(WordDescr *WordsDescs, unsigned NumWords, unsigned CutTo,
                                PaintedPiece *PaintedPieces, unsigned &NumPaintedPieces, unsigned StartWord = 0)
//Fills PaintedPieces
{
    NumPaintedPieces = 0;
    unsigned LastWord = 0;
    unsigned NumPaintedInside = 0;
    PaintedPieces[0].FirstWord = StartWord;
    if (WordsDescs[0].Type)
        NumPaintedInside = 1;

    for (LastWord = StartWord + 1; LastWord < NumWords; LastWord++)
    {//to compare with "real" length were need to substract NumPaintedInside * 2 * MARK_LEN
        if (WordsDescs[LastWord].Type)
            NumPaintedInside++;
        if (WordsDescs[LastWord].End - NumPaintedInside * 2 * MARK_LEN > CutTo)
        {
            if (WordsDescs[LastWord].Type)
                NumPaintedInside--;
            break;
        }
    }

    PaintedPieces[0].LastWord = --LastWord;
    PaintedPieces[0].NumPainted = (char)NumPaintedInside;

    unsigned FirstWord = StartWord;
    for (NumPaintedPieces = 1; NumPaintedPieces < NumWords; NumPaintedPieces++)
    {
        if (LastWord == NumWords - 1)
            return;
        LastWord++; //move 1 word right
        if (WordsDescs[LastWord].Type)
            NumPaintedInside++;
        unsigned End = WordsDescs[LastWord].End;
        for (; FirstWord < LastWord; FirstWord++)
        {   //cast away some left words
            if (End - WordsDescs[FirstWord].Beg - (NumPaintedInside * 2 * MARK_LEN) <= CutTo)
                break;
            if (WordsDescs[FirstWord].Type)
                NumPaintedInside--;
        }
        PaintedPieces[NumPaintedPieces].FirstWord = FirstWord;
        unsigned Beg = WordsDescs[FirstWord].Beg;
        for (LastWord++; LastWord < NumWords; LastWord++)
        {   //may be add some more right words
            if (WordsDescs[LastWord].Type)
                NumPaintedInside++;
            if (WordsDescs[LastWord].End - Beg - NumPaintedInside * 2 * MARK_LEN > CutTo)
            {
                if (WordsDescs[LastWord].Type)
                    NumPaintedInside--;
                break;
            }
        }
        PaintedPieces[NumPaintedPieces].LastWord = --LastWord;
        PaintedPieces[NumPaintedPieces].NumPainted = (char)NumPaintedInside;
    }
}

constexpr ui32 TypeWeights[] = {3, 100, 1000, 10000};
constexpr size_t TypeWeightsCount = Y_ARRAY_SIZE(TypeWeights);
constexpr ui32 FleshWeight = 30;

ui32 PaintedPieceWeight(PaintedPiece &PntdPiece, WordDescr *WordsDescs)
//weigths PaintedPieces
{
    ui32 Weight = 0;
    unsigned i = PntdPiece.FirstWord;
    int FirstPainted = -1;
    int LastPainted = 0;
    unsigned Type;
    for (; i <= PntdPiece.LastWord; i++)
    {
        Type = WordsDescs[i].Type;
        if (Type)
        {
            if (FirstPainted == -1)
                FirstPainted = i - PntdPiece.FirstWord;
            LastPainted = i;
        }
        if (Type < TypeWeightsCount) {
            Weight += TypeWeights[Type];
        }
    }
    if (FirstPainted != -1)
    {   //take in account so-called "myasistost'"
         LastPainted = PntdPiece.LastWord - LastPainted;
         Weight += (LastPainted * FirstPainted * FleshWeight);
    }
    //when almost equal, longest is the best
    Weight += (WordsDescs[PntdPiece.LastWord].End - WordsDescs[PntdPiece.FirstWord].Beg - PntdPiece.NumPainted * 2 * MARK_LEN);
    return Weight;
}

bool GoodForBegin(PaintedPiece &PntdPiece, WordDescr *WordsDescs, unsigned  NumWords, unsigned NumPaintedWords) {
    if (!PntdPiece.NumPainted)
        // don't like if there no painted words at all
        return false;
    if ((unsigned)PntdPiece.NumPainted * 2 >= NumPaintedWords)
        //enough painted words
        return true;
    int BestRelev = 0;
    for (unsigned i = 0; i < NumWords; i++)
        //checking if we have at least one bestrelev word
        if (WordsDescs[i].Type > BestRelev) {
            if (i > PntdPiece.LastWord)
                //best found outside
                return false;
            BestRelev = WordsDescs[i].Type;
        }
    return true;
}

bool CheckTitle(TUtf16String& passage) {
    if (passage.size() < size_t(MARK_LEN))
        return false;

    const size_t pos = passage.size() - MARK_LEN;
    if (passage[pos] != HILIGHT_MARK ||
        passage[pos + 1] != TITLE_MARK)
    {
        return false;
    }
    passage.resize(pos);
    return true;
}

bool CountPars(const char *line, unsigned &LineLen, const char *Pars) {
    int NumPars = 0;
    unsigned LastBalanced = 0;
    if (!LineLen)
        return false;
    const char *firstpar = strchr(line, (int)Pars[0]);
    if (firstpar == nullptr ||
            firstpar - line >= (int)LineLen)
        return false;
    unsigned  i;
    int Balance = 1;
    NumPars = 1;
    for (i = firstpar - line + 1; i < LineLen; i++) {
        if (line[i] == HILIGHT_MARK) {
            i++;
            continue;
        }
        if (line[i] == Pars[1]) {
            if (Balance > 0) {
                NumPars++;
                Balance--;
                if (Balance == 0)
                    LastBalanced = i;
                continue;
            }
        }
        if (line[i] == Pars[0]) {
            Balance++;
            NumPars++;
        }
    }
    if (NumPars > 1)
        LineLen = std::min(LineLen, LastBalanced);
    return (NumPars != 1);
}

static str_spn sspn("-:=/*|");

bool CheckTitleBegin(TUtf16String& WPassage, WordDescr *WordsDescs, int NumWords, int CutTo,
                    int &NewCut, TUtf16String& TitleBegin, int &NumBeginWords) {

    const TString Passage = WideToChar(WPassage, CODES_YANDEX);

    if (CutTo <= (int)TITLE_FIRST_PIECE_LEN)
        return false;
    unsigned lastdel = WordsDescs[0].End, goodlastdel = 0;
    while (lastdel < TITLE_FIRST_PIECE_LEN) {
        lastdel += (sspn.cspn(Passage.data() + lastdel) + 1);
        if (lastdel >= TITLE_FIRST_PIECE_LEN)
            break;
        if (Passage[lastdel] == ' ')
            goodlastdel = lastdel + 1;
    }
    if (!goodlastdel)
        return false;
    if (CountPars(Passage.data(), goodlastdel, "()"))
        return false;
    if (CountPars(Passage.data(), goodlastdel, "[]"))
        return false;
    if (CountPars(Passage.data(), goodlastdel, "{}"))
        return false;
    if (CountPars(Passage.data(), goodlastdel, "\"\""))
        return false;
    TitleBegin = TUtf16String(WPassage.data(), WPassage.data() + goodlastdel);
    NewCut = CutTo - (int)goodlastdel;
    for (NumBeginWords = 0; NumBeginWords < NumWords; NumBeginWords++)
        if (WordsDescs[NumBeginWords].Beg >= goodlastdel)
            break;
    return true;
}

void CutPassage(TString &Passage, unsigned CutTo, bool InsLeadingTrailingDots) {
    TUtf16String wpass(CharToWide(Passage, csYandex));
    CutPassage(wpass, CutTo, InsLeadingTrailingDots);
    Passage = WideToChar(wpass, CODES_YANDEX);
}

void CutPassage(TUtf16String& Passage, unsigned CutTo, bool InsLeadingTrailingDots) {
    TUtf16String TrailEllipsys = InsLeadingTrailingDots ? ELLIPSIS : TUtf16String();
    CutPassage(Passage, CutTo, true, ELLIPSIS, TrailEllipsys);
}

void CutPassage(TUtf16String& Passage, unsigned CutTo, bool AccountForEllipsisLen, const TUtf16String& LeadEllipsis, const TUtf16String& TailEllipsis) {
    bool IsTitle = CheckTitle(Passage);
    if (AccountForEllipsisLen) {
        if (CutTo <= LeadEllipsis.length() + TailEllipsis.length())
            return;
        CutTo -= LeadEllipsis.length() + TailEllipsis.length();
    }
    if (CutTo == 0)
        return;
    unsigned Len = Passage.size();
    if (Len <= CutTo) {
        return; //shorter than  CutTo - no need to cut
    }
    WordDescr WordsDescs[MAX_WORDS_NUM];
    unsigned NumWords;
    unsigned NumPaintedWords;
    unsigned CutEnd = CutTo;

    ScanLine(Passage, WordsDescs, NumWords, NumPaintedWords);

    if (Len - NumPaintedWords * 2 * MARK_LEN <= CutTo)
        return; //shorter than  CutTo (when delete inserts) - no need to cut

    if (!NumPaintedWords)
    {   //not painted (may happen with titles)
        unsigned i = 0;
        for (; i + 1 < NumWords; i++)
            if (WordsDescs[i + 1].End > CutTo)
                break;
        CutEnd = (i < NumWords) ? WordsDescs[i].End : 0;

        Passage.erase(Passage.begin() + CutEnd, Passage.end());
        Passage.insert(Passage.end(), TailEllipsis.begin(), TailEllipsis.end());
        return;
    }

    int NewCut = CutTo;
    int NumBeginWords = 0;

    TUtf16String TitleBegin;
    if (IsTitle)
        IsTitle = CheckTitleBegin(Passage, WordsDescs, NumWords, CutTo, NewCut, TitleBegin, NumBeginWords);
    PaintedPiece PaintedPieces[MAX_WORDS_NUM]; //no more than words
    unsigned NumPaintedPieces;

    DeterminePaintedPieces(WordsDescs, NumWords, NewCut, PaintedPieces, NumPaintedPieces, NumBeginWords);
    if (IsTitle && !NumPaintedPieces) {
        IsTitle = false;
        NewCut = CutTo;
        DeterminePaintedPieces(WordsDescs, NumWords, NewCut, PaintedPieces, NumPaintedPieces);
    }

    unsigned FirstPiece = 0;
    while (FirstPiece < NumPaintedPieces - 1 && PaintedPieces[0].FirstWord == PaintedPieces[FirstPiece + 1].FirstWord)
        ++FirstPiece;

    if (GoodForBegin(PaintedPieces[FirstPiece], WordsDescs, NumWords, NumPaintedWords)) {
        unsigned Begin = WordsDescs[PaintedPieces[FirstPiece].FirstWord].Beg;
        unsigned End = WordsDescs[PaintedPieces[FirstPiece].LastWord].End;
        if (End) {
            Passage.erase(Passage.begin() + End, Passage.end());
            Passage.insert(Passage.length(), TailEllipsis);
        }
        if (Begin) {
            Passage.erase(Passage.begin(),Passage.begin() + Begin);
            Passage.insert(0, LeadEllipsis);
        }
        if (IsTitle)
            Passage.insert(Passage.begin(), TitleBegin.begin(), TitleBegin.end());
        return;
    }

    Y_ASSERT(NumPaintedPieces);
    ui32 BestWeight = PaintedPieceWeight(PaintedPieces[0], WordsDescs);
    ui32 Weight;
    unsigned BestPiece = 0;
    for (unsigned i = 1; i < NumPaintedPieces; i++)
    {
        if (PaintedPieces[i].NumPainted == 0)
            continue;
        Weight = PaintedPieceWeight(PaintedPieces[i], WordsDescs);
        if (Weight > BestWeight)
        {
            BestWeight = Weight;
            BestPiece = i;
        }
    }
    unsigned Begin = WordsDescs[PaintedPieces[BestPiece].FirstWord].Beg;
    unsigned End = WordsDescs[PaintedPieces[BestPiece].LastWord].End;
    unsigned CutLen = End - Begin - PaintedPieces[BestPiece].NumPainted * 3 * MARK_LEN;
    if (CutLen < CutTo && End < Len && Passage[End] != ' '
            && !WhatSymbol(Passage[End]))
        End++; //add 1 more punctuation symbol at the end

    // Passage = TString(Passage, Begin, End - Begin);
    if (End) {
        Passage.erase(Passage.begin() + End, Passage.end());
        Passage.insert(Passage.length(), TailEllipsis);
    }
    if(Begin) {
        Passage.erase(Passage.begin(), Passage.begin() + Begin);
        Passage.insert(0, LeadEllipsis);
    }
    if (IsTitle)
        Passage.insert(0, TitleBegin);
}

bool IsMark(size_t mark) {
    return mark < (NUM_MARKS * 2);
}

bool IsBeginMark(size_t mark) {
    return mark < (size_t)NUM_MARKS;
}

bool IsEndMark(size_t mark) {
    return mark >= (size_t)NUM_MARKS && mark <= (NUM_MARKS * 2);
}

template<class S>
bool FindEndMark(size_t beginMark, S& str, unsigned i) {
    for (; i < str.size() - 1; ++i) {
        if (str[i] != HILIGHT_MARK)
            continue;

        const char* P = strchr(HILIGHTS, str[i + 1]);
        if (!P)
            continue;
        size_t mark = (size_t)(P -  HILIGHTS);

        if (IsMark(mark)) {
            if (mark == beginMark + NUM_MARKS)
                return true;
            break;
        }
    }

    return false;
}

template <class S>
bool EnablePaint(size_t mark, S& str, unsigned i, bool* seenBeginMark) {
    if (IsBeginMark(mark)) {
        if (FindEndMark(mark, str, i + 2)) {
            *seenBeginMark = true;
            return true;
        }
        return false;
    }
    else if (IsEndMark(mark)) {
        if (*seenBeginMark) {
            *seenBeginMark = false;
            return true;
        }
        return false;
    }

    return true;
}

void ReplaceHilites(TUtf16String& ToReplace, bool WithContent, const TArrayRef<const char*> HtmlHilites)
{
    size_t Len = ToReplace.size();
    if (!Len)
        return;

    TUtf16String Result;
    size_t CopyBeg = 0;
    bool seenBeginMark = false;
    for (unsigned i = 0; i < Len - 1; ++i) {
        if (ToReplace[i] == HILIGHT_MARK)
        {
            const char* P = strchr(HILIGHTS, ToReplace[i + 1]);
            if (!P)
                continue;
            size_t Paint = (size_t)(P -  HILIGHTS);
            if (i > CopyBeg)
                Result.insert(Result.end(), ToReplace.begin() + CopyBeg, ToReplace.begin() + i);
            if (WithContent && EnablePaint(Paint, ToReplace, i, &seenBeginMark)) {
                TStringBuf paint;
                if (HtmlHilites.size() > Paint && HtmlHilites[Paint] != nullptr)
                    paint = HtmlHilites[Paint];
                else
                    paint = DEF_HTML_HILIGHTS[Paint];
                Result.append(CharToWide(paint, csYandex));
            }
            CopyBeg = i + 2;
        }
    }
    if (CopyBeg == 0)
        //nothing replaced
        return;

    if (Len > CopyBeg)
        Result.insert(Result.end(), ToReplace.begin() + CopyBeg, ToReplace.end());
    ToReplace.swap(Result);
}

void ReplaceHilites(TString &ToReplace, bool ReplaceOrDelete, const TArrayRef<const char*> HtmlHilites)
{
    unsigned Len = ToReplace.length();
    if (Len == 0)
        return;

    TString Result;
    unsigned i = 0, CopyBeg = 0;
    bool seenBeginMark = false;
    for (; i < Len - 1; i++)
    {
        if (ToReplace[i] == HILIGHT_MARK)
        {
            const char* P = strchr(HILIGHTS, ToReplace[i + 1]);
            if (!P)
                continue;
            size_t Paint = (size_t)(P -  HILIGHTS);
            if (i > CopyBeg)
                Result.append(ToReplace, CopyBeg, i - CopyBeg);
            if (ReplaceOrDelete && EnablePaint(Paint, ToReplace, i, &seenBeginMark))
            {
                if (HtmlHilites.size() > Paint && HtmlHilites[Paint] != nullptr)
                    Result += HtmlHilites[Paint];
                else
                    Result += DEF_HTML_HILIGHTS[Paint];
             }
             CopyBeg = i + 2;
        }
    }
    if (CopyBeg == 0)
        //nothing replaced
        return;

    if (Len > CopyBeg)
        Result.append(ToReplace, CopyBeg, Len - CopyBeg);
    ToReplace = Result;
    return;
}


void GlueSentences(const TVector<TUtf16String>& sentences, TUtf16String& result, bool insert3Dots)
{
    if (sentences.empty())
    {
        result.clear();
        return;
    }

    size_t len = 0;
    for (size_t i = 0; i < sentences.size(); ++i)
        len += (sentences[i].size() + 1);
    if (insert3Dots)
        len += (sentences.size() * INSERT_3DOTS_LEN);

    TCharTemp p(len);

    len = 0;
    for (size_t i = 0; i < sentences.size(); ++i)
    {
        if (sentences[i].empty())
            continue;
        std::char_traits<wchar16>::copy(p.Data() + len, sentences[i].data(), sentences[i].size());
        len += sentences[i].size();
        if (!insert3Dots)
        {
            p.Data()[len++] = ' ';
        } else {
            std::char_traits<wchar16>::copy(p.Data() + len, ELLIPSIS.data(), INSERT_3DOTS_LEN);
            len += INSERT_3DOTS_LEN;
        }
    }
    if (len)
        len -= 1;
    p.Data()[len] = '\0'; // ?
    result.assign(p.Data(), len);
}

// TString EncodeXMLString(const char *);

TString
ConvertRawText(const char* rawtext, ui32 maxLen, int mode,
                   const char *Open1, const char *Open2, const char *Open3,
                   const char *Close1, const char *Close2, const char *Close3)
{
    const TStringBuf textBuf = rawtext ? TStringBuf(rawtext) : TStringBuf("");
    return ConvertRawTextBuf(textBuf, maxLen, mode, Open1, Open2, Open3, Close1, Close2, Close3);
}

TString
ConvertRawTextBuf(const TStringBuf rawtext, ui32 maxLen, int mode,
                   const char *Open1, const char *Open2, const char *Open3,
                   const char *Close1, const char *Close2, const char *Close3)
{
    //mode: 0 = html (highlighted), 1 = html (no highlites), 2 = plain text
    TUtf16String wstr = UTF8ToWide(rawtext.data(), rawtext.length(), csYandex);

    if (maxLen > 0)
        CutPassage(wstr, maxLen);       // 1. Cut passage first

    if (mode < 2)                       // 2. Escape now, Hilites shouldn't be escaped
        EscapeHtmlChars<false>(wstr);

    if (mode == 0 || mode == 3) {
        std::array<const char*, 6> hilites = {Open1, Open2, Open3, Close1, Close2, Close3};
        ReplaceHilites(wstr, true, hilites); //3. replace hilites if mode == 0
    } else {
        ReplaceHilites(wstr, false);          //3. delete hilites if mode != 0
    }

    return WideToUTF8(wstr);
}
