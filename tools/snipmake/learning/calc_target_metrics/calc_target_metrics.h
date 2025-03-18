#pragma once

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

class TMetricsCalculator {
private:
    static constexpr double PBREAK = 0.85;
    bool Pairwise;
    TVector<double> FormulaValue;
    TVector< std::pair<size_t, double> > Marks;
    typedef TVector< std::pair< std::pair<size_t, size_t>, double> > TPairsWeight;
    TPairsWeight PairsWeight;
    typedef TMap<size_t, TVector<std::pair<double, double> > > TQueriesToMarks;
    TQueriesToMarks QueriesToMarks;

private:
    void LoadFormulaValue(const TString& path);
    void LoadPairsWeight(const TString& path);
    void LoadMarks(const TString& path);

private:
    double CalcPfound(double pbreak) const;
    double CalcQuadraticError() const;
    double CalcRMSE() const;
    double CalcCorrectlyOrderedPairs() const;

public:
    TMetricsCalculator(const TString& path, bool pairwise)
        : Pairwise(pairwise)
    {
        LoadFormulaValue(path + ".matrixnet");

        if (Pairwise) {
            LoadPairsWeight(path + ".pairs");
        } else {
            LoadMarks(path);
        }
    }

    void PrintMetrics() const {
        if (Pairwise) {
            Cout << "Correctly ordered pairs: " << CalcCorrectlyOrderedPairs() << Endl;
            return;
        }
        Cout << "Clicks: " << CalcPfound(0) << Endl;
        Cout << "Pfound: " << CalcPfound(PBREAK) << Endl;
        Cout << "Quadratic error: " << CalcQuadraticError() << Endl;
        Cout << "RMSE: " << CalcRMSE() << Endl;
    }
};
