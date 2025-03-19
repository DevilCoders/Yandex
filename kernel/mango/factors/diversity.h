#pragma once

#include <library/cpp/shingles/readable.h>
#include <library/cpp/prob_counter/prob_counter.h>
#include <util/generic/hash_set.h>


namespace NMango
{
    template <int SIZE, int WND1 = 1, int WND2 = 0>
    class TDiversityProcessorBase
    {
    protected:
        typedef TShingler<TUTF8ReadableCarver<SIZE, THashFirstUTF8Word<TFNVHash<ui64> > >, TMinModInWindow<TReadable<ui64>, WND1, WND2> > TMangoShingler;
        TMangoShingler Shingler;

        void AddTrash(TString &text)
        {
            const static TString trash(" aaa ");
            for (int i = 0; i < SIZE - 2; ++i) {
                text = trash + text + trash;
            }
        }

        void GetShingles(const TString &text, TVector<TReadableUTF8Shingler::TShingle> &shingles)
        {
            Shingler.Reset();
            Shingler.ShingleText(shingles, text.data(), text.size());
        }

        void GetShingles(const TString &text, THashSet<long unsigned int> &shinglesValues)
        {
            TVector<TReadableUTF8Shingler::TShingle> shingles;
            GetShingles(text, shingles);
            shinglesValues.clear();
            for (TVector<TReadableUTF8Shingler::TShingle>::const_iterator i = shingles.begin(), iend = shingles.end();
                    i != iend; ++i) {
                shinglesValues.insert(i->Value);
            }
        }

    public:
        TDiversityProcessorBase(size_t minWordLength = 2, bool caseSensitive = false)
            : Shingler(minWordLength, caseSensitive)
        {
        }
    };

    /// Текстовое разнообразие для случая, когда под рукой вся коллекция документов
    template <int SIZE, int WND1 = 1, int WND2 = 0>
    class TDiversityProcessor : public TDiversityProcessorBase<SIZE, WND1, WND2>
    {
        ui32 ShinglesCount;
        float ShinglesWeightSum;
        THashSet<long unsigned int> ShinglesUnique;
    public:
        TDiversityProcessor(size_t minWordLength = 2, bool caseSensitive = false)
            : TDiversityProcessorBase<SIZE, WND1, WND2>(minWordLength, caseSensitive)
            , ShinglesCount(0)
            , ShinglesWeightSum(0.0f)
        {
        }

        void Reset()
        {
            ShinglesCount = 0;
            ShinglesWeightSum = 0.0f;
            ShinglesUnique.clear();
        }

        float CalcUnlikeness(TString text)
        {
            TDiversityProcessorBase<SIZE, WND1, WND2>::AddTrash(text);
            THashSet<long unsigned int> shinglesValues;
            TDiversityProcessorBase<SIZE, WND1, WND2>::GetShingles(text, shinglesValues);
            int nBad = 0;
            for (THashSet<long unsigned int>::iterator i = shinglesValues.begin(), iend = shinglesValues.end(); i != iend; ++i) {
                if (ShinglesUnique.find(*i) == ShinglesUnique.end()) {
                    ++nBad;
                }
            }
            return nBad / (float)(shinglesValues.size());
        }

        void OnText(TString text, ui32 count = 1)
        {
            TDiversityProcessorBase<SIZE, WND1, WND2>::AddTrash(text);
            TVector<TReadableUTF8Shingler::TShingle> shingles;
            TDiversityProcessorBase<SIZE, WND1, WND2>::GetShingles(text, shingles);
            THashSet<long unsigned int> shinglesCurrent;
            size_t shinglesAdded(0);
            for (TVector<TReadableUTF8Shingler::TShingle>::const_iterator i = shingles.begin(), iend = shingles.end();
                    i != iend; ++i) {
                long unsigned int value = i->Value;
                if (ShinglesUnique.insert(value).second) {
                    ++shinglesAdded;
                }
                shinglesCurrent.insert(value);
            }
            if (!shinglesCurrent.empty()) {
                ShinglesWeightSum += shinglesAdded / static_cast<float>(shinglesCurrent.size());
            }
            ShinglesCount += count;
        }

        float CalcDiversity()
        {
            if (ShinglesCount) {
                return ShinglesWeightSum / static_cast<float>(ShinglesCount);
            } else {
                return 0.0;
            }
        }
    };

    /// Текстовое разнообразие для случая, когда под рукой всей коллекции документов нет, и надо
    /// обрабатывать их распределенно, а потом сливать
    template <int SIZE, int BUCKETCNT = 32, int WND1 = 1, int WND2 = 0>
    class TMergeableDiversityProcessor : public TDiversityProcessorBase<SIZE, WND1, WND2>
    {
        ui32 ShinglesCount;
        TFMCounter<ui32, BUCKETCNT> ShinglesUnique;

    public:
        TMergeableDiversityProcessor(size_t minWordLength = 2, bool caseSensitive = false)
            : TDiversityProcessorBase<SIZE, WND1, WND2>(minWordLength, caseSensitive)
            , ShinglesCount(0)
            , ShinglesUnique()
        {
        }

        void Reset()
        {
            ShinglesCount = 0;
            ShinglesUnique = TFMCounter<ui32, BUCKETCNT>();
        }

        void OnText(TString text, ui32 count = 1)
        {
            TDiversityProcessorBase<SIZE, WND1, WND2>::AddTrash(text);
            TVector<TReadableUTF8Shingler::TShingle> shingles;
            TDiversityProcessorBase<SIZE, WND1, WND2>::GetShingles(text, shingles);
            THashSet<long unsigned int> shinglesCurrent;
            for (TVector<TReadableUTF8Shingler::TShingle>::const_iterator i = shingles.begin(), iend = shingles.end();
                    i != iend; ++i) {
                TString text(WideToUTF8(i->Text));
                ShinglesUnique.Add(text.data());
                shinglesCurrent.insert(i->Value);
            }
            ShinglesCount += shinglesCurrent.size() * count;
        }

        float CalcDiversity()
        {
            if (ShinglesCount) {
                return Min(1.000001f, ShinglesUnique.Count() / static_cast<float>(ShinglesCount));
            } else {
                return 1.0;
            }
        }

        void Save(IOutputStream* out) const
        {
            ::Save(out, ShinglesCount);
            ::Save(out, ShinglesUnique);
        }

        void Load(IInputStream* inp)
        {
            ::Load(inp, ShinglesCount);
            ::Load(inp, ShinglesUnique);
        }

        void Save(TString& out) const
        {
            TStringOutput fout(out);
            Save(&fout);
        }

        void Load(const TString& inp)
        {
            TStringInput finp(inp);
            Load(&finp);
        }

        void Merge(const TMergeableDiversityProcessor& other)
        {
            ShinglesCount += other.ShinglesCount;
            ShinglesUnique.Merge(other.ShinglesUnique);
        }
    };

    typedef TDiversityProcessor<5, 10, 8> TResourceDiversityProcessor;
}
