#pragma once

#include <util/stream/str.h>
#include <util/digest/murmur.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitops.h>
#include <util/ysaveload.h>

#include <cmath>

/*
  TFMCounter* - вероятностный счетчик числа уникальных объектов, умеющий сливаться
  TFMSummator* - вероятностный сумматор характеристики уникальных объектов (интеграл), умеющий сливаться
      (характеристики должны быть положительными)

  TFMCounter* требует для хранения (cellCount) байт памяти
  TFMSummator* требует для хранения (cellCount * bucketCount + 8) байт памяти

  считается, что счетчик хорошо работает при (element count >= 20 * cellCount)

  TFMSummator:
  maxValue = 1e6, minValue = 1e-6, bucketCount = 40 - при таком значении параметров
      в следующей корзинке лежат значения в 2 раза больше предыдущего
      (отрезок (minValue, maxValue) делится на (bucketCount) равных частей по логарифмической шкале)

  для счетчика рекомендуется cellCount = 32
  для сумматора рекомендуется cellCount = 16/32, bucketCount = log2(maxValue / minValue)
      (по умолчанию cellCount = 16 в целях экономии памяти для хранения состояния - это уже 648 байт)

  таблица зависимости ошибок от cellCount
      (независимо от того, в какую она сторону - вверх или вниз, значение 1.0 - отсутствие ошибок):

TFMCounter32:

cellCount - avg error - max error
4    1.585479094 3.656613597
8    1.435650934 3.86530784
16   1.356020367 2.642593145
32   1.099148144 1.312603831
64   1.057000133 1.353642464
128  1.305872791 1.441774011
256  1.335297704 1.418946028
512  1.212575736 1.37388134
1024 1.224877373 1.302187681
2048 1.221827511 1.272405871
4096 1.234324136 1.320669906
8192 1.236135418 1.293136376

TFMCounter64:

cellCount - avg error - max error
4    1.73658318  2.981188122
8    1.303454954 2.21916693
16   1.395477736 2.445100784
32   1.252591309 1.605483819
64   1.066046975 1.511825681
128  1.369583435 1.626104355
256  1.155239768 1.444371104
512  1.273730689 1.373177528
1024 1.206562146 1.307770875
2048 1.244177107 1.326493458
4096 1.241850294 1.283434876
8192 1.219413984 1.292261359

TFMSummator32:

cellCount - avg error - max error (bucketCount = 40)
4    1.93387561  3.87979
8    1.980729858 3.19527
16   1.326852051 1.78697
32   1.056799683 1.49596
64   1.179052856 2.22513
128  1.175697632 1.54667
256  1.186656006 1.67279
512  1.056351318 1.35407
1024 1.137850586 1.61874
2048 1.151888794 1.87434
4096 1.104470459 1.83913
8192 1.111626831 1.28914

TFMSummator64:

cellCount - avg error - max error (bucketCount = 40)
4    1.515503418 2.03681
8    1.425166992 2.67419
16   1.136989258 1.65087
32   1.26325415  1.64347
64   1.130359619 1.99673
128  1.060022461 1.58957
256  1.140120117 1.59753
512  1.059144409 1.35561
1024 1.141551025 1.6553
2048 1.104541382 1.90504
4096 1.120599976 1.85664
8192 1.121497681 1.29573
*/

template <typename T>
class THashTraits {
public:
    static inline i8 LowestOneIdx(T x) {
        return LeastSignificantBit(x);
    }

    static T KeyHashFunction(const char* data, size_t dataSize, int key) {
        return static_cast<T>(
            MurmurHash<T>(data, dataSize, key));
    }
};

class TFMCounterException: public yexception {};

/// Flajolet-Martin
template <typename T, size_t CellCount = 32, typename TTraits = THashTraits<T>>
class TFMCounter {
private:
    i8 Borders[CellCount];

public:
    TFMCounter() {
        Reset();
    }

    void Reset() {
        memset(Borders, -1, sizeof(Borders));
    }

    void Add(const char* data, size_t dataSize) {
        for (size_t i = 0; i < CellCount; ++i) {
            T hashValue = TTraits::KeyHashFunction(data, dataSize, i);
            Borders[i] = Max(Borders[i], TTraits::LowestOneIdx(hashValue));
        }
    }

    void Add(const char* data) {
        Add(data, strlen(data));
    }

    float Count() const {
        int sum(0), cnt(0);
        for (size_t i = 0; i < CellCount; ++i) {
            if (Borders[i] >= 0) {
                sum += Borders[i];
                ++cnt;
            }
        }
        if (cnt) {
            return pow(2.0f, sum / static_cast<float>(cnt) - 1.0f) / 0.77351f;
        } else {
            return 0.0;
        }
    }

    void Save(IOutputStream* out) const {
        ::SaveArray(out, Borders, CellCount);
    }

    void Load(IInputStream* inp) {
        ::LoadArray(inp, Borders, CellCount);
    }

    void Save(TString& out) const {
        TStringOutput fout(out);
        Save(&fout);
    }

    void Load(const TString& inp) {
        if (CellCount != inp.size()) {
            throw TFMCounterException() << "size of data != combinator capacity (" << inp.size() << " != " << CellCount << ")";
        }
        TStringInput finp(inp);
        Load(&finp);
    }

    void Merge(const TFMCounter<T, CellCount, TTraits>& x) {
        for (size_t i = 0; i < CellCount; ++i) {
            Borders[i] = Max(Borders[i], x.Borders[i]);
        }
    }
};

/// probabilistic sum of unique values (positive)
template <typename T, size_t BucketCount = 40, size_t CellCount = 16, typename TTraits = THashTraits<T>>
class TFMSummator {
#define TFMSummator_EPS 1E-7f

public:
    TFMSummator(float maxValue = 1e6, float minValue = 1e-6)
        : MaxValue(maxValue)
        , MinValue(minValue)
    {
        CalcBordersAndMedians();
    }

    void Reset() {
        for (size_t i = 0; i < BucketCount; ++i) {
            Buckets[i].Reset();
        }
    }

    void Add(const char* data, size_t dataSize, float value) {
        float* res = std::lower_bound(Borders, Borders + BucketCount + 1, value);
        if (res == Borders)
            ++res;
        else if (res == Borders + BucketCount + 1)
            --res;

        size_t index = res - Borders - 1;
        Buckets[index].Add(data, dataSize);
    }

    void Add(const char* data, float value) {
        Add(data, strlen(data), value);
    }

    float Sum() const {
        float result(0.0f);
        for (size_t i = 0; i < BucketCount; ++i) {
            result += Buckets[i].Count() * Medians[i];
        }
        return result;
    }

    float CalcAvgValueInInterval(float minvalue, float maxvalue) const {
        float count(0.0f), sum(0.0f);
        for (size_t i = 0; i < BucketCount; ++i) {
            float borders_i = Borders[i], borders_i1 = Borders[i + 1];
            float right = Min(maxvalue, borders_i1), left = Max(minvalue, borders_i);
            float proportion = (right - left) / (borders_i1 - borders_i);
            if (proportion > 0.0f) {
                float dcount = Buckets[i].Count() * proportion;
                count += dcount;
                sum += dcount * (right + left) / 2.0;
            }
        }

        if (count > TFMSummator_EPS) {
            return sum / count;
        } else {
            return 0.0f;
        }
    }

    float CalcQuantile(float p) const {
        if (p <= 0.0f) {
            return MinValue;
        }
        if (p >= 1.0f) {
            return MaxValue;
        }

        float counts[BucketCount];

        float totalCount(0.0f);
        for (size_t i = 0; i < BucketCount; ++i) {
            counts[i] = Buckets[i].Count();
            totalCount += counts[i];
        }

        float quantileLimit = totalCount * p;
        for (size_t i = 0; i < BucketCount; ++i) {
            quantileLimit -= counts[i];
            if (fabs(quantileLimit) < TFMSummator_EPS) {
                return Borders[i + 1];
            } else if (quantileLimit < 0.0f) {
                return Borders[i + 1] + (Borders[i + 1] - Borders[i]) * quantileLimit / counts[i];
            }
        }

        return MaxValue;
    }

    float Count() const {
        float count(0.0f);
        for (size_t i = 0; i < BucketCount; ++i) {
            count += Buckets[i].Count();
        }
        return count;
    }

    void Save(IOutputStream* out) const {
        ::Save(out, MaxValue);
        ::Save(out, MinValue);
        ::SaveArray(out, Buckets, BucketCount);
    }

    void Load(IInputStream* inp) {
        ::Load(inp, MaxValue);
        ::Load(inp, MinValue);
        ::LoadArray(inp, Buckets, BucketCount);
        CalcBordersAndMedians();
    }

    void Save(TString& out) const {
        TStringOutput fout(out);
        Save(&fout);
    }

    void Load(const TString& inp) {
        TStringInput finp(inp);
        Load(&finp);
    }

    void Merge(const TFMSummator<T, BucketCount, CellCount, TTraits>& x) {
        for (size_t i = 0; i < BucketCount; ++i) {
            Buckets[i].Merge(x.Buckets[i]);
        }
    }

private:
    float MaxValue, MinValue;
    TFMCounter<T, CellCount, TTraits> Buckets[BucketCount];
    float Borders[BucketCount + 1];
    float Medians[BucketCount];

    void CalcBordersAndMedians() {
        float k = MinValue, step = exp(log(MaxValue / MinValue) / (float)BucketCount);
        Borders[0] = k;
        for (size_t i = 0; i < BucketCount; ++i) {
            k *= step;
            Medians[i] = (Borders[i] + k) / 2.0f;
            Borders[i + 1] = k;
        }
    }
};

using TFMCounter32 = TFMCounter<ui32>;
using TFMCounter64 = TFMCounter<ui64>;

using TFMSummator32 = TFMSummator<ui32>;
using TFMSummator64 = TFMSummator<ui64>;
