#pragma once

#include <library/cpp/html/face/parsface.h>
#include <kernel/indexer/face/docinfo.h>
#include <library/cpp/numerator/numerate.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <yweb/antispam/antisyn/syn_factors.h>

class IDocumentDataInserter;

class TSynonymsHandler : public INumeratorHandler {
public:
    TSynonymsHandler()
        : NumTotal(0)
        , NumN(0)
        , NumN1(0)
        , NumTotalCur(0)
        , NumNcur(0)
        , NumN1cur(0)
        , Crit1(0)
        , Crit1cur(0)
        , WordsCur(0)
        , WasBody(false)
        , Start(false)
        , Was1(false)
        , LastIsNumber(false)
        , CurWord(nullptr)
        , NumLatinLetters(0)
        {}
    void OnTextEnd(const IParsedDocProperties*, const TNumerStat& ) override;
    void OnSpaces(TBreakType, const wchar16* data, unsigned length, const TNumerStat& stat) override;
    void OnTokenStart(const TWideToken& wtok, const TNumerStat&) override;
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat&) override;

    double GetSyn7b() const;
    double GetSyn8b() const;
    double GetSyn9a() const;
    double GetPercentBadWordPairs() const
    { return Crit1 / (NumN1 + 1.0); }
    double GetNumBadWordPairs() const
    { return Crit1 / (Crit1 + 10.0); }
    double GetNumLatinLetters() const
    { return NumLatinLetters / (NumLatinLetters + 100.0); }
    void SetTester(const TTestSyn* d) {
        Tester = d;
    }

    void InsertFactors(IDocumentDataInserter& inserter) const;

private:
    const TTestSyn* Tester;
    void ProcessOneWord(const char* word, size_t len);
    void CloseBlock();
    // три пространственные координаты - сумма по предыдущим найденным блокам
    int NumTotal, NumN, NumN1;
    // три пространственные координаты - значения для текущего блока
    int NumTotalCur, NumNcur, NumN1cur;
    // две статистические координаты - сумма по предыдущим найденным блокам
    // две статистические координаты - значения для текущего блока
    int Crit1, Crit1cur;
    int WordsCur;
    // был ли уже тег <body>? а предыдущее слово вообще было?
    bool WasBody, Start;
    // встретилось ли предыдущее слово в словаре? а текущее?
    bool Was1;
    // был ли последний токен числом?
    bool LastIsNumber;
    // парадигмы для предыдущего слова
    const TTestSyn::TSetU* CurWord;
    double CorStat, CorStat2;
    TVector<double> CorForPairs;
    TVector<double> CorForPairsCur;
    int NumLatinLetters;
    char TempString[1024];
};
