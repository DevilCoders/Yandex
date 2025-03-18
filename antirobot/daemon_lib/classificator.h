#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>


namespace NAntiRobot {


class TProcessorLinearizedFactors : public TVector<float> {
public:
    using TVector<float>::TVector;
};

class  TCacherLinearizedFactors : public TVector<float> {
public:
    using TVector<float>::TVector;
};

template<typename T>
class TClassificator : TNonCopyable {
public:
    TClassificator() = default;
    virtual ~TClassificator() = default;

    double operator()(const T& lf) const {
        T remappedFactors;
        Remap(lf, remappedFactors);
        return Classify(remappedFactors);
    }

    class TLoadError : public yexception {
    };

protected:
    TVector<size_t> FactorsRemap;

private:
    void Remap(const T& lf, T& rf) const {
        rf.reserve(FactorsRemap.size());

        for (size_t i = 0; i < FactorsRemap.size(); ++i) {
            rf.push_back(lf[FactorsRemap[i]]);
        }
    }

    virtual double Classify(const T& remappedFactors) const = 0;
};


TClassificator<TProcessorLinearizedFactors>* CreateProcessorClassificator(const TString& formulaFilename);
TClassificator<TCacherLinearizedFactors>* CreateCacherClassificator(const TString& formulaFilename);

} // namespace NAntiRobot
