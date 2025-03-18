// See documentation on: https://beta.wiki.yandex-team.ru/jandekspoisk/misspell/editdistance/editdistancelib
#include "edit_distance.h"
#include "align_wtrokas.h"
#include <library/cpp/charset/ci_string.h>
#include <util/digest/murmur.h>
#include <util/ysaveload.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace NEditDistance {
    namespace {
        TUtf16String RemoveEpsilonsFromContext(const TUtf16String& wtroka) {
            TVector<wchar16> symbols(wtroka.begin(), wtroka.end());
            TVector<wchar16> newWtrokaSymbols;
            for (const auto& symbol : symbols) {
                if (symbol != EPSILON) {
                    newWtrokaSymbols.push_back(symbol);
                }
            }
            return TUtf16String(newWtrokaSymbols.begin(), newWtrokaSymbols.end());
        }

        TVector<double> ReadCoefficients(IInputStream& inputStream) {
            TString TCiString;
            inputStream.ReadTo(TCiString, '\n');
            TVector<TString> strs = SplitString(TCiString, "\t");
            TVector<double> coefficients(strs.size());
            for (size_t i = 0; i < strs.size(); ++i) {
                coefficients[i] = StrToD(strs[i].c_str(), nullptr);
            }

            return coefficients;
        }

        TContextCostTableMap ReadCostTable(IInputStream& inputStream, bool readContext) {
            TString TCiString;

            TContextCostTableMap contextMap;
            TContext context(3);

            while (inputStream.ReadTo(TCiString, '\n')) {
                TVector<TString> strs = SplitString(TCiString, "\t");

                if (readContext) {
                    Y_ASSERT(strs.size() == 6);
                    for (size_t i = 0; i < 3; ++i) {
                        context[i] = RemoveEpsilonsFromContext(::UTF8ToWide(strs[i]));
                    }
                } else {
                    Y_ASSERT(strs.size() == 3);
                }

                wchar16 lhs = ::UTF8ToWide(strs[strs.size() - 3])[0];
                wchar16 rhs = ::UTF8ToWide(strs[strs.size() - 2])[0];

                double value = StrToD(strs.back().c_str(), nullptr);
                contextMap[context][lhs][rhs] = value;
            }

            return contextMap;
        }

        struct SubcontextComparator {
            bool operator()(const TContextSize& lhs, const TContextSize& rhs) {
                return std::accumulate(lhs.begin(), lhs.end(), 0) > std::accumulate(rhs.begin(), rhs.end(), 0);
            }
        };

    }

    size_t TContextHash::operator()(const TContext& a) const {
        size_t hash = 0;
        for (const auto& TCiString : a) {
            hash += MurmurHash<size_t>(TCiString.data(), TCiString.size() * sizeof(TCiString[0]));
            hash *= NUMBER;
        }

        return hash;
    }

    TCostTable::TCostTable(const TCostTableMap& table, long double defaultValue)
        : Table_(table)
        , DefaultValue_(defaultValue)
    {
    }

    TCostTable::TCostTable() {
    }

    long double TCostTable::Cost(wchar16 lhsChar, wchar16 rhsChar) const {
        auto rowIterator = Table_.find(lhsChar);
        if (rowIterator != Table_.end()) {
            auto cellIterator = rowIterator->second.find(rhsChar);
            if (cellIterator != rowIterator->second.end()) {
                return cellIterator->second;
            }
        }
        return DefaultValue_;
    }

    long double TCostTable::GetDefaultValue() const {
        return DefaultValue_;
    }

    void TCostTable::Save(IOutputStream* out) const {
        ::Save(out, Table_);
        ::Save(out, DefaultValue_);
    }

    void TCostTable::Load(IInputStream* in) {
        ::Load(in, Table_);
        ::Load(in, DefaultValue_);
    }

    TConstTableIterator TCostTable::begin() const {
        return Table_.begin();
    }

    TConstTableIterator TCostTable::end() const {
        return Table_.end();
    }

    TContextCostTable::TContextCostTable() {
    }

    TContextCostTable::TContextCostTable(const TContextCostTableMap& table, const TContextSize& size, long double defaultValue)
        : Table_(table)
        , ContextSize_(size)
        , SubcontextSizes_(GetSubcontextsSizes(size))
        , DefaultValue_(defaultValue)
    {
    }

    void TContextCostTable::SetContextSize(size_t lhsPrefixSize, size_t lhsSuffixSize, size_t rhsPrefixSize) {
        ContextSize_.push_back(lhsPrefixSize);
        ContextSize_.push_back(lhsSuffixSize);
        ContextSize_.push_back(rhsPrefixSize);
    }

    long double TContextCostTable::Cost(const TContext& context, wchar16 lhsChar, wchar16 rhsChar) const {
        for (const auto& subcontextSize : SubcontextSizes_) {
            TContext subcontext(3);
            subcontext[0] = TUtf16String(context[0], context[0].size() - subcontextSize[0], subcontextSize[0]);
            subcontext[1] = TUtf16String(context[1], 0, subcontextSize[1]);
            subcontext[2] = TUtf16String(context[2], context[2].size() - subcontextSize[2], subcontextSize[2]);

            auto contextIt = Table_.find(subcontext);
            if (contextIt != Table_.end()) {
                auto rowIt = contextIt->second.find(lhsChar);
                if (rowIt != contextIt->second.end()) {
                    auto cellIt = rowIt->second.find(rhsChar);
                    if (cellIt != rowIt->second.end()) {
                        return cellIt->second;
                    }
                }
            }
        }

        return lhsChar != rhsChar ? DefaultValue_ : 0.;
    }

    void TContextCostTable::Read(IInputStream& in) {
        TVector<double> coeffs = ReadCoefficients(in);
        Y_ASSERT(coeffs.size() == 4);
        ContextSize_.clear();
        std::for_each(coeffs.begin(), coeffs.end(), [&](double x) { ContextSize_.push_back(static_cast<size_t>(x)); });
        DefaultValue_ = coeffs[3];
        Table_ = ReadCostTable(in, true);
        SubcontextSizes_ = GetSubcontextsSizes(ContextSize_);
    }

    void TContextCostTable::Save(IOutputStream* out) const {
        ::Save(out, Table_);
        ::Save(out, ContextSize_);
        ::Save(out, SubcontextSizes_);
        ::Save(out, DefaultValue_);
    }

    void TContextCostTable::Load(IInputStream* in) {
        ::Load(in, Table_);
        ::Load(in, ContextSize_);
        ::Load(in, SubcontextSizes_);
        ::Load(in, DefaultValue_);
    }

    TContextSize TContextCostTable::GetContextSize() const {
        return ContextSize_;
    }

    TVector<TContextSize> TContextCostTable::GetSubcontextsSizes(TContextSize contextSizes) {
        TVector<TContextSize> subcontextsSizes;
        for (size_t first = 0; first <= contextSizes[0]; ++first) {
            for (size_t second = 0; second <= contextSizes[1]; ++second) {
                for (size_t third = 0; third <= contextSizes[2]; ++third) {
                    TContextSize currentSizes(3);
                    currentSizes[0] = first;
                    currentSizes[1] = second;
                    currentSizes[2] = third;
                    subcontextsSizes.push_back(currentSizes);
                }
            }
        }
        std::sort(subcontextsSizes.begin(), subcontextsSizes.end(), SubcontextComparator());
        return subcontextsSizes;
    }

    TCostTable ProbsToDistances(const TCostTable& probTable) {
        TCostTableMap distanceTable;
        for (const auto& lhsMap : probTable) {
            for (const auto& rhsValue : lhsMap.second) {
                double distance = -std::log(std::max(MINIMAL_PROBABILITY, rhsValue.second));
                distanceTable[lhsMap.first][rhsValue.first] = distance;
            }
        }

        return TCostTable(distanceTable, -std::log(std::max(MINIMAL_PROBABILITY, probTable.GetDefaultValue())));
    }

    TCostTable DistancesToProbs(const TCostTable& distanceTable) {
        TCostTableMap probTable;
        for (const auto& lhsMap : distanceTable) {
            for (const auto& rhsValue : lhsMap.second) {
                double probability = std::exp(-std::min(MAXIMAL_DISTANCE, rhsValue.second));
                probTable[lhsMap.first][rhsValue.first] = probability;
            }
        }

        return TCostTable(probTable, std::exp(-std::min(MAXIMAL_DISTANCE, distanceTable.GetDefaultValue())));
    }

    TContextFreeLevenshtein::TContextFreeLevenshtein(const TCostTable& table, double removalCoef, double insertionCoef)
        : RemovalCoef_(removalCoef)
        , InsertionCoef_(insertionCoef)
        , CostTable_(table)
    {
    }

    TContextFreeLevenshtein::TContextFreeLevenshtein() {
    }

    TCostTable TContextFreeLevenshtein::GetCostTable() const {
        return CostTable_;
    }

    void TContextFreeLevenshtein::SetCostTable(const TCostTable& table) {
        CostTable_ = table;
    }

    void TContextFreeLevenshtein::ReadModel(const TString& filename) {
        TUnbufferedFileInput inputStream(filename);

        TVector<double> coeffs = ReadCoefficients(inputStream);
        Y_ASSERT(coeffs.size() == 3);
        RemovalCoef_ = coeffs[0];
        InsertionCoef_ = coeffs[1];
        double defaultValue = coeffs[2];
        CostTable_ = TCostTable(ReadCostTable(inputStream, false)[TContext(3)], defaultValue);
    }

    void TContextFreeLevenshtein::LoadModel(const TString& filename) {
        TUnbufferedFileInput inputStream(filename);
        ::Load(&inputStream, RemovalCoef_);
        ::Load(&inputStream, InsertionCoef_);
        ::Load(&inputStream, CostTable_);
    }

    void TContextFreeLevenshtein::SaveModel(const TString& filename) const {
        TUnbufferedFileOutput outputStream(filename);
        ::Save(&outputStream, RemovalCoef_);
        ::Save(&outputStream, InsertionCoef_);
        ::Save(&outputStream, CostTable_);
    }

    TContextFreeLevenshtein::~TContextFreeLevenshtein() = default;

    TViterbiLevenshtein::TViterbiLevenshtein(const TCostTable& table, double removalCoef, double insertionCoef, bool isProbTable)
        : TContextFreeLevenshtein(isProbTable ? ProbsToDistances(table) : table, removalCoef, insertionCoef)
    {
    }

    TViterbiLevenshtein::TViterbiLevenshtein() {
    }

    TViterbiLevenshtein::~TViterbiLevenshtein() = default;

    double TViterbiLevenshtein::CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const {
        std::pair<size_t, size_t> distTableSize(lhs.size() + 1, rhs.size() + 1);
        TVector<TVector<double>> distTable(distTableSize.first, TVector<double>(distTableSize.second, 0.));

        for (size_t lhsIndex = 0; lhsIndex < distTableSize.first; ++lhsIndex) {
            for (size_t rhsIndex = 0; rhsIndex < distTableSize.second; ++rhsIndex) {
                if (!lhsIndex && !rhsIndex) {
                    continue;
                }
                TVector<double> operationCosts(3, MAXIMAL_DISTANCE);
                if (lhsIndex > 0) {
                    operationCosts[0] = distTable[lhsIndex - 1][rhsIndex] +
                                        CostTable_.Cost(lhs[lhsIndex - 1], EPSILON) * RemovalCoef_;
                    if (rhsIndex == 0) {
                    }
                }

                if (rhsIndex > 0) {
                    operationCosts[1] = distTable[lhsIndex][rhsIndex - 1] +
                                        CostTable_.Cost(EPSILON, rhs[rhsIndex - 1]) * InsertionCoef_;
                    if (lhsIndex == 0) {
                    }
                }

                if (rhsIndex > 0 && lhsIndex > 0) {
                    operationCosts[2] = distTable[lhsIndex - 1][rhsIndex - 1] +
                                        CostTable_.Cost(lhs[lhsIndex - 1], rhs[rhsIndex - 1]);
                }

                distTable[lhsIndex][rhsIndex] = *std::min_element(operationCosts.begin(), operationCosts.end());
            }
        }

        return distTable[distTableSize.first - 1][distTableSize.second - 1];
    }

    TStochasticLevenshtein::TStochasticLevenshtein(const TCostTable& table, double removalCoef, double insertionCoef, bool isProbTable)
        : TContextFreeLevenshtein(isProbTable ? table : DistancesToProbs(table), removalCoef, insertionCoef)
    {
    }

    TStochasticLevenshtein::TStochasticLevenshtein() {
    }

    TStochasticLevenshtein::~TStochasticLevenshtein() = default;

    double TStochasticLevenshtein::CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const {
        std::pair<size_t, size_t> distTableSize(lhs.size() + 1, rhs.size() + 1);
        TVector<TVector<long double>> distTable(distTableSize.first, TVector<long double>(distTableSize.second, 0.));

        distTable[0][0] = 1.;

        for (size_t lhsIndex = 0; lhsIndex < distTableSize.first; ++lhsIndex) {
            for (size_t rhsIndex = 0; rhsIndex < distTableSize.second; ++rhsIndex) {
                if (!lhsIndex && !rhsIndex) {
                    continue;
                }
                TVector<long double> operationCosts(3, 0.0);
                if (lhsIndex > 0) {
                    operationCosts[0] = distTable[lhsIndex - 1][rhsIndex] *
                                        CostTable_.Cost(lhs[lhsIndex - 1], EPSILON) * RemovalCoef_;
                }

                if (rhsIndex > 0) {
                    operationCosts[1] = distTable[lhsIndex][rhsIndex - 1] *
                                        CostTable_.Cost(EPSILON, rhs[rhsIndex - 1]) * InsertionCoef_;
                }

                if (rhsIndex > 0 && lhsIndex > 0) {
                    operationCosts[2] = distTable[lhsIndex - 1][rhsIndex - 1] *
                                        CostTable_.Cost(lhs[lhsIndex - 1], rhs[rhsIndex - 1]);
                }

                distTable[lhsIndex][rhsIndex] = std::accumulate(operationCosts.begin(), operationCosts.end(), 0.L);
            }
        }
        long double distance = distTable[distTableSize.first - 1][distTableSize.second - 1];
        distance = std::max(distance, MINIMAL_PROBABILITY);
        return -std::log(distance * CostTable_.Cost(END_OPERATION, END_OPERATION));
    }

    void TContextLevenshtein::ReadModel(const TString& filename) {
        TUnbufferedFileInput inputStream(filename);
        CostTable_.Read(inputStream);
    }

    void TContextLevenshtein::LoadModel(const TString& filename) {
        TUnbufferedFileInput inputStream(filename);
        ::Load(&inputStream, CostTable_);
    }

    void TContextLevenshtein::SaveModel(const TString& filename) const {
        TUnbufferedFileOutput outputStream(filename);
        ::Save(&outputStream, CostTable_);
    }

    void TContextLevenshtein::SetContextSize(size_t lhsPrefixSize, size_t lhsSuffixSize, size_t rhsPrefixSize) {
        CostTable_.SetContextSize(lhsPrefixSize, lhsSuffixSize, rhsPrefixSize);
    }

    double TContextLevenshtein::CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const {
        if (lhs.empty() && rhs.empty()) {
            return 0.;
        }
        auto wtrokaPairs = AlignWtrokas(lhs, rhs);
        if (wtrokaPairs.empty()) {
            return CalcDistanceImpl(lhs, rhs);
        }
        double distance = 0.;
        for (const auto& pair : wtrokaPairs) {
            if (pair.first.empty() && pair.second.empty()) {
                continue;
            }
            distance += CalcDistanceImpl(pair.first, pair.second);
        }

        return distance;
    }

    TContext TContextLevenshtein::GetContextByIndices(const TUtf16String& lhs, const TUtf16String& rhs,
                                                      size_t lhsIndex, size_t rhsIndex) const {
        auto contextSize = CostTable_.GetContextSize();
        size_t lhsPrefixBegin = std::max(static_cast<int>(lhsIndex - contextSize[0]), 0);
        size_t rhsPrefixBegin = std::max(static_cast<int>(rhsIndex - contextSize[2]), 0);
        TContext context(3);
        context[0] = TUtf16String(lhs, lhsPrefixBegin, lhsIndex - lhsPrefixBegin);
        context[1] = TUtf16String(lhs, lhsIndex, contextSize[1]);
        context[2] = TUtf16String(rhs, rhsPrefixBegin, rhsIndex - rhsPrefixBegin);

        return ExpandContext(context);
    }

    TContext TContextLevenshtein::ExpandContext(const TContext& context) const {
        TContext expandedContext(3);
        auto contextSize = CostTable_.GetContextSize();
        expandedContext[0] = TUtf16String::Join(TUtf16String(contextSize[0] - context[0].size(), BEGINNING_OF_STRING), context[0]);
        expandedContext[1] = TUtf16String::Join(context[1], TUtf16String(contextSize[1] - context[1].size(), END_OF_STRING));
        expandedContext[2] = TUtf16String::Join(TUtf16String(contextSize[2] - context[2].size(), BEGINNING_OF_STRING), context[2]);

        return expandedContext;
    }

    double TContextLevenshtein::CalcDistanceImpl(const TUtf16String& lhs, const TUtf16String& rhs) const {
        std::pair<size_t, size_t> distTableSize(lhs.size() + 1, rhs.size() + 1);
        TVector<TVector<double>> distTable(distTableSize.first, TVector<double>(distTableSize.second, 0.));
        for (size_t lhsIndex = 0; lhsIndex < distTableSize.first; ++lhsIndex) {
            for (size_t rhsIndex = 0; rhsIndex < distTableSize.second; ++rhsIndex) {
                if (!lhsIndex && !rhsIndex) {
                    continue;
                }

                TVector<double> operationCosts(3, MAXIMAL_DISTANCE);
                if (lhsIndex > 0) {
                    operationCosts[0] = distTable[lhsIndex - 1][rhsIndex] +
                                        CostTable_.Cost(GetContextByIndices(lhs, rhs, lhsIndex - 1, rhsIndex), lhs[lhsIndex - 1], EPSILON);
                }

                if (rhsIndex > 0) {
                    GetContextByIndices(lhs, rhs, lhsIndex, rhsIndex - 1);

                    operationCosts[1] = distTable[lhsIndex][rhsIndex - 1] +
                                        CostTable_.Cost(GetContextByIndices(lhs, rhs, lhsIndex, rhsIndex - 1), EPSILON, rhs[rhsIndex - 1]);
                }

                if (rhsIndex > 0 && lhsIndex > 0) {
                    operationCosts[2] = distTable[lhsIndex - 1][rhsIndex - 1] +
                                        CostTable_.Cost(GetContextByIndices(lhs, rhs, lhsIndex - 1, rhsIndex - 1), lhs[lhsIndex - 1], rhs[rhsIndex - 1]);
                }

                distTable[lhsIndex][rhsIndex] = *std::min_element(operationCosts.begin(), operationCosts.end());
            }
        }

        return distTable[distTableSize.first - 1][distTableSize.second - 1] / (lhs.size() + rhs.size());
    }

    TUtf16String PreprocessWtroku(const TUtf16String wtroka, const THashSet<wchar16>& alphabet) {
        TVector<wchar16> symbols;
        const TUtf16String lower = to_lower(wtroka);
        for (const auto& symbol : lower) {
            if (alphabet.count(symbol)) {
                symbols.push_back(symbol);
            }
        }

        return TUtf16String(symbols.begin(), symbols.end());
    }

}
