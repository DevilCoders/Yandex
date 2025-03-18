#pragma once

#include <util/random/mersenne.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>

#include <cmath>
#include <cstdlib>

class IDistribution;

typedef TSimpleSharedPtr<IDistribution> IDistributionRef;

class IDistribution {
    public:
        virtual ~IDistribution() {
        }

        virtual size_t Delta() = 0;
        virtual IDistributionRef Clone() const = 0;
};

class TUniformDistribution: public IDistribution {
    public:
        inline TUniformDistribution(size_t qps = 100)
            : m(1000000 / qps)
        {
        }

        ~TUniformDistribution() override {
        }

        size_t Delta() override {
            return m / 2 + rand() % m;
        }

        IDistributionRef Clone() const override {
            return new TUniformDistribution(*this);
        }

    private:
        size_t m;
};

class TPoissonDistribution: public IDistribution {
    public:
        inline TPoissonDistribution(size_t qps = 100)
            : Rand_(17)
            , Koef_(-1000000.0 / (double)qps)
        {
        }

        ~TPoissonDistribution() override {
        }

        size_t Delta() override {
            return (size_t)(Koef_ * log(Rand_.GenRandReal3()));
        }

        IDistributionRef Clone() const override {
            return new TPoissonDistribution(*this);
        }

    private:
        TMersenne<ui64> Rand_;
        double Koef_;
};

class TDistribFactory {
        typedef TSimpleSharedPtr<IDistribution> TDistribution;
        typedef THashMap<TString, TDistribution> TDistributions;
    public:
        static inline TDistribFactory& Instance(size_t qps) {
            static TDistribFactory f(qps);

            return f;
        }

        inline IDistributionRef Find(const TString& type) {
            TDistributions::const_iterator it = Distrs_.find(type);

            if (it == Distrs_.end()) {
                ythrow yexception() << "unknown distribution type (" <<  type.data() << ")";
            }

            return it->second->Clone();
        }

    private:
        inline TDistribFactory(size_t qps) {
            Add("poisson", TDistribution(new TPoissonDistribution(qps)));
            Add("uniform", TDistribution(new TUniformDistribution(qps)));
        }

        inline ~TDistribFactory() {
        }

        inline void Add(const char* type, TDistribution distrib) {
            Distrs_[type] = distrib;
        }

    private:
        TDistributions Distrs_;
};
