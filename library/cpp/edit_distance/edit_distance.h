#pragma once

// See documentation on: https://beta.wiki.yandex-team.ru/jandekspoisk/misspell/editdistance/editdistancelib

#include <library/cpp/charset/ci_string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <util/string/split.h>
#include <util/string/vector.h>
#include <util/stream/file.h>

namespace NEditDistance {
    using TCostTableMap = THashMap<wchar16, THashMap<wchar16, long double>>;
    using TConstTableIterator = TCostTableMap::const_iterator;
    using TContext = TVector<TUtf16String>;

    struct TContextHash {
        static const size_t NUMBER = 17;
        size_t operator()(const TContext& a) const;
    };

    using TContextCostTableMap = THashMap<TContext, TCostTableMap, TContextHash>;
    using TContextSize = TVector<size_t>;

    const wchar16 EPSILON = 'E';
    const wchar16 END_OPERATION = '#';
    const wchar16 BEGINNING_OF_STRING = '^';
    const wchar16 END_OF_STRING = '$';

    const TUtf16String ALPHABET_WTROKA = ::UTF8ToWide(TString(" #'0123456789Eabcdefghijklmnopqrstuvwxyzабвгдежзийклмнопрстуфхцчшщъыьэюяё"));
    const THashSet<wchar16> ALPHABET(ALPHABET_WTROKA.begin(), ALPHABET_WTROKA.end());

    const long double MINIMAL_PROBABILITY = 1e-155;
    const long double MAXIMAL_DISTANCE = 100;

    using TContextCostTableMap = THashMap<TContext, TCostTableMap, TContextHash>;
    using TContextSize = TVector<size_t>;

    class TCostTable {
    public:
        TCostTable(const TCostTableMap& table, long double defaultValue);
        TCostTable();

        long double Cost(wchar16 lhsChar, wchar16 rhsChar) const;
        long double GetDefaultValue() const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* in);
        TConstTableIterator begin() const;
        TConstTableIterator end() const;

    private:
        TCostTableMap Table_;
        long double DefaultValue_;
    };

    class TContextCostTable {
    public:
        TContextCostTable(const TContextCostTableMap& table, const TContextSize& size, long double defaultValue);
        TContextCostTable();

        void SetContextSize(size_t lhsPrefixSize, size_t lhsSuffixSize, size_t rhsPrefixSize);
        long double Cost(const TContext& context, wchar16 lhsChar, wchar16 rhsChar) const;
        void Read(IInputStream& in);
        void Save(IOutputStream* out) const;
        void Load(IInputStream* in);
        TContextSize GetContextSize() const;

    private:
        TContextCostTableMap Table_;
        TContextSize ContextSize_;
        TVector<TContextSize> SubcontextSizes_;
        long double DefaultValue_;

    private:
        TVector<TContextSize> GetSubcontextsSizes(TContextSize contextSizes);
    };

    TCostTable ProbsToDistances(const TCostTable& probTable);
    TCostTable DistancesToProbs(const TCostTable& distanceTable);

    class TContextFreeLevenshtein {
    public:
        TContextFreeLevenshtein();
        TContextFreeLevenshtein(const TCostTable& table, double removalCoef = 1.0, double insertionCoef = 1.0);
        virtual ~TContextFreeLevenshtein();

        TCostTable GetCostTable() const;
        void SetCostTable(const TCostTable& table);
        void ReadModel(const TString& filename);
        void LoadModel(const TString& filename);
        void SaveModel(const TString& filename) const;
        virtual double CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const = 0;

    protected:
        double RemovalCoef_;
        double InsertionCoef_;
        TCostTable CostTable_;
    };

    class TViterbiLevenshtein: public TContextFreeLevenshtein {
    public:
        TViterbiLevenshtein();
        TViterbiLevenshtein(const TCostTable& table, double removalCoef = 1.0, double insertionCoef = 1.0, bool isProbTable = true);
        double CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const override;
        ~TViterbiLevenshtein() override;
    };

    class TStochasticLevenshtein: public TContextFreeLevenshtein {
    public:
        TStochasticLevenshtein();
        TStochasticLevenshtein(const TCostTable& table, double removalCoef = 1.0, double insertionCoef = 1.0, bool isProbTable = true);
        double CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const override;
        ~TStochasticLevenshtein() override;
    };

    class TContextLevenshtein {
    public:
        void ReadModel(const TString& filename);
        void LoadModel(const TString& filename);
        void SaveModel(const TString& filename) const;
        void SetContextSize(size_t lhsPrefixSize, size_t lhsSuffixSize, size_t rhsPrefixSize);
        double CalcDistance(const TUtf16String& lhs, const TUtf16String& rhs) const;

    private:
        TContextCostTable CostTable_;

    private:
        TContext GetContextByIndices(const TUtf16String& lhs, const TUtf16String& rhs,
                                     size_t lhsIndex, size_t rhsIndex) const;
        TContext ExpandContext(const TContext& context) const;
        double CalcDistanceImpl(const TUtf16String& lhs, const TUtf16String& rhs) const;
    };

    TUtf16String PreprocessWtroku(const TUtf16String wtroka, const THashSet<wchar16>& alphabet);

    template <typename T>
    void MakePredictions(T& measurer, IInputStream& inputStream, IOutputStream& outputStream, bool usePreprocessing) {
        TString TCiString;
        while (inputStream.ReadTo(TCiString, '\n')) {
            TVector<TString> strs = SplitString(TCiString, "\t");
            Y_ASSERT(strs.size() >= 2);
            TUtf16String lhs = ::UTF8ToWide(strs[0]);
            TUtf16String rhs = ::UTF8ToWide(strs[1]);
            double distance;
            if (usePreprocessing) {
                TUtf16String preprocessedLhs = PreprocessWtroku(lhs, ALPHABET);
                TUtf16String preprocessedRhs = PreprocessWtroku(rhs, ALPHABET);

                distance = measurer.CalcDistance(preprocessedLhs, preprocessedRhs);
            } else {
                distance = measurer.CalcDistance(lhs, rhs);
            }

            outputStream << lhs << "\t" << rhs << "\t" << distance << "\n";
        }
    }

}
