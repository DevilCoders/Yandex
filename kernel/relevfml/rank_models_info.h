#pragma once

#include <util/generic/string.h>
#include <util/stream/output.h>

struct TRankingModelInfo {
    static const TString DEFAULT_VALUE;

    TString ModelName = DEFAULT_VALUE;
    TString MatrixnetName = DEFAULT_VALUE;
    TString MatrixnetID = DEFAULT_VALUE;
    TString MatrixnetMemorySource = DEFAULT_VALUE;
    TString MatrixnetMD5 = DEFAULT_VALUE;
    TString GetModelMatrixnet = DEFAULT_VALUE;
    TString PolynomialName = DEFAULT_VALUE;
    TString GetModelPolynomial = DEFAULT_VALUE;

    TRankingModelInfo() :
        ModelName()
    {}
};

struct IRankingModelInfoFormatter {
    virtual void FormatModelInfo(const TRankingModelInfo& modelInfo) = 0;
};

struct TStreamRankingModelInfoFormatter: public IRankingModelInfoFormatter {
    explicit TStreamRankingModelInfoFormatter(IOutputStream& out) : Out(out) {}

    void FormatModelInfo(const TRankingModelInfo& modelInfo) override {
        Out << modelInfo.ModelName
            << "\t" << modelInfo.MatrixnetName
            << "\t" << modelInfo.MatrixnetID
            << "\t" << modelInfo.MatrixnetMemorySource
            << "\t" << modelInfo.MatrixnetMD5
            << "\t" << modelInfo.GetModelMatrixnet
            << "\t" << modelInfo.PolynomialName
            << "\t" << modelInfo.GetModelPolynomial
            << Endl;
    }

    IOutputStream& Out;
};

class TRankModelsMapFactory;

void FormatRankingModelsInfo(const TRankModelsMapFactory& rankModels, IRankingModelInfoFormatter& formatter, bool calcMD5 = false);
