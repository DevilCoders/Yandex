#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NClickSim {
    class TWordVector {
        using TPair = std::pair<TString, double>;
        static constexpr auto Comparer = [](const TPair &p1, const TPair &p2) {
            return p1.second == p2.second
                   ? p1.first < p2.first
                   : p1.second > p2.second;
        };
        THashMap<TString, double> Weights;
    public:
        explicit TWordVector(const TVector<TString>& words);
        explicit TWordVector(THashMap<TString, double>&& weights);
        TWordVector(const TWordVector&) = default;
        TWordVector(TWordVector&& other) noexcept = default;
        TWordVector() = default;

        void Normalize();
        [[nodiscard]]
        double CalcL2Norm() const;
        [[nodiscard]]
        size_t Size() const;
        void Strip(size_t limit);
        void StripByMinWeight(double minWeight);
        // Strip by min l2 norm share to keep (1 = keep untouched)
        void StripByMinL2Share(double l2Share);

        TWordVector& operator = (TWordVector&& other) noexcept = default;
        double operator * (const TWordVector& other) const;
        void operator *= (double val);
        void operator += (const TWordVector& other);

        [[nodiscard]]
        const THashMap<TString, double>& GetWeights() const;
    };
}
