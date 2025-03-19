#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/datetime/base.h>

class TRemapTable;

void GetExpGroupIndices(size_t count, size_t groupCount, size_t groupGrowFactor, TVector<size_t>* indices);
TRemapTable GetRemapExp(const TVector<float> &data, size_t groupCount, size_t groupGrowFactor);

void DoLinearRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues);
void DoGroupRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues, size_t groupCount);
void DoExpRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues, size_t groupCount, size_t groupGrowFactor);
void DoLogRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues);

float OrderRemap(const TVector<size_t>& indices, const TVector<float>& sortedData, float value);

class IRemap {
public:
    virtual void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) = 0;
    virtual ~IRemap() {}
};

typedef TSimpleSharedPtr<IRemap> TRemapPtr;

class TLinearRemap : public IRemap {
public:
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

class TGroupRemap : public IRemap {
private:
    size_t GroupCount;

public:
    TGroupRemap(size_t groupCount);
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

class TExpRemap : public IRemap {
private:
    size_t GroupCount;
    size_t GroupGrowFactor;

public:
    TExpRemap(size_t groupCount, size_t groupGrowFactor);
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

class TLogRemap : public IRemap {
public:
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

class TOrderExpGroupRemap : public IRemap {
private:
    size_t GroupCount;
    size_t GroupGrowFactor;

public:
    TOrderExpGroupRemap(size_t groupCount, size_t groupGrowFactor);
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

class TTrivialRemap : public IRemap {
public:
    void Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) override;
};

TRemapPtr GetRemap(const TString& remapParameters);

extern const char* remapsHelpStrings[];
extern const size_t remapsHelpStringCount;

inline ui32 RemapTime(time_t curTime, i32 addTime) {
    if (curTime >= addTime) {
        i32 timeMap = (i32)floor(sqrtf(float(curTime - addTime))/38);
        timeMap = ClampVal(timeMap, 0, 255);
        return timeMap;
    } else {
        return 0;
    }
}

inline ui32 RemapTimeFromCurrent(i32 addTime) {
    return RemapTime(TInstant::Now().TimeT(), addTime);
}

template <typename F, typename T = ui32, ui32 groups = 256>
class TPrototypeGroupRemap {
private:
    TVector<T> Borders;
    F Funk;
public:
    TPrototypeGroupRemap()
        : Funk(F())
    {
        Borders.reserve(groups);

        for (size_t i = 0; i < groups - 1; ++i) {
            Borders.push_back((T)Funk(i));
        }

        Borders.push_back(Max<T>());
    }

    void operator() (const TVector<T*>& remappingValues) {
        for (typename TVector<T*>::const_iterator it = remappingValues.begin(); it != remappingValues.end(); ++it) {
            **it = LowerBound(Borders.begin(), Borders.end(), **it) - Borders.begin();
        }
    }

    ui32 operator() (const T& remapValue) {
        return LowerBound(Borders.begin(), Borders.end(), remapValue) - Borders.begin();
    }
};

template <ui32 growKoef>
class TGrowingExponent {
public:
    inline double operator() (ui32 x) {
        return pow(x, 1 + (double)x/growKoef);
    }
};

typedef TPrototypeGroupRemap<TGrowingExponent<140> > THostSizeRemap;
