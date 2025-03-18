#include "calc_target_metrics.h"

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/split.h>

void TMetricsCalculator::LoadFormulaValue(const TString& path) {
    TFileInput matrixnetFile(path);
    TString line;
    while (matrixnetFile.ReadLine(line)) {
        TVector<TString> lineData;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&lineData);
        FormulaValue.push_back(FromString<double>(lineData[4]));
    }
}

void TMetricsCalculator::LoadPairsWeight(const TString& path) {
    TFileInput pairsFile(path);
    TString line;
    while (pairsFile.ReadLine(line)) {
        TVector<TString> lineData;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&lineData);
        size_t left = FromString<size_t>(lineData[0]);
        size_t right = FromString<size_t>(lineData[1]);
        double weight = 1.0;
        if (lineData.size() == 3)
            weight = FromString<double>(lineData[2]);
        PairsWeight.push_back(std::make_pair(std::make_pair(left, right), weight));
    }
}

void TMetricsCalculator::LoadMarks(const TString& path) {
    TFileInput featuresFile(path);
    TString line;
    size_t lineNum = 0;
    while (featuresFile.ReadLine(line)) {
        TVector<TString> lineData;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&lineData);
        size_t queryID = FromString<size_t>(lineData[0]);
        double mark = FromString<double>(lineData[1]);
        Marks.push_back(std::make_pair(queryID, mark));
        QueriesToMarks[Marks[lineNum].first].push_back(std::make_pair(FormulaValue[lineNum], Marks[lineNum].second));
        ++lineNum;
    }
    for (TQueriesToMarks::iterator it = QueriesToMarks.begin(); it != QueriesToMarks.end(); ++it) {
        Sort(it->second.begin(), it->second.end());
        Reverse(it->second.begin(), it->second.end());
    }
}

double TMetricsCalculator::CalcPfound(double pbreak) const {
    double result = 0;
    for (TQueriesToMarks::const_iterator it = QueriesToMarks.begin(); it != QueriesToMarks.end(); ++it) {
        double pfound = 0;
        double plook = 1.0;
        for (size_t resultNum = 0; resultNum < it->second.size(); ++resultNum) {
             pfound += it->second[resultNum].second * plook;
             plook = plook * (1.0 - it->second[resultNum].second) * pbreak;
        }
        result += pfound;
    }
    return result / QueriesToMarks.size();
}

double sqr(double x) {
    return x * x;
}

double TMetricsCalculator::CalcQuadraticError() const {
    double result = 0;
    for (size_t lineNum = 0; lineNum < FormulaValue.size(); ++lineNum)
         result += sqr(Marks[lineNum].second - FormulaValue[lineNum]);
    return sqrt(result / FormulaValue.size());
}

double TMetricsCalculator::CalcRMSE() const {
    double result = 0;
    for (TQueriesToMarks::const_iterator it = QueriesToMarks.begin(); it != QueriesToMarks.end(); ++it) {
        double value = 0;
        for (size_t resultNum = 0; resultNum < it->second.size(); ++resultNum)
            value += FormulaValue[resultNum] - Marks[resultNum].second;
        value /= it->second.size();
        for (size_t resultNum = 0; resultNum < it->second.size(); ++resultNum)
            result += sqr(Marks[resultNum].second - FormulaValue[resultNum] - value);
    }
    return sqrt(result / FormulaValue.size());
}

double TMetricsCalculator::CalcCorrectlyOrderedPairs() const {
    double result = 0;
    double sum = 0;
    for (size_t lineNum = 0; lineNum < PairsWeight.size(); ++lineNum) {
        sum += PairsWeight[lineNum].second;
        if (FormulaValue[PairsWeight[lineNum].first.first] >=  FormulaValue[PairsWeight[lineNum].first.second])
            result += PairsWeight[lineNum].second;
    }
    return result / sum;
}
