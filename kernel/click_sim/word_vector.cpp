#include "word_vector.h"

#include <util/string/split.h>

#include <cmath>

namespace NClickSim {

    TWordVector::TWordVector(const TVector<TString> &words) {
        Y_ENSURE(!words.empty());

        for (const TString &word : words) {
            Weights[word] += 1;
        }
        Normalize();
    }

    double TWordVector::CalcL2Norm() const {
        double norm = 0.0;
        for (const auto &p : Weights) {
            norm += p.second * p.second;
        }
        return std::sqrt(norm);
    }

    void TWordVector::Normalize() {
        const double norm = CalcL2Norm();
        for (auto &p : Weights) {
            p.second /= norm;
        }
    }

    double TWordVector::operator*(const TWordVector &other) const {
        const THashMap<TString, double> *first;
        const THashMap<TString, double> *second;
        if (Weights.size() < other.Weights.size()) {
            first = &Weights;
            second = &other.Weights;
        } else {
            first = &other.Weights;
            second = &Weights;
        }

        double result = 0;
        for (const auto &p : *first) {
            auto it = second->find(p.first);
            if (it != second->end()) {
                result += p.second * it->second;
            }
        }

        return result;
    }

    void TWordVector::operator*=(double val) {
        for (auto& weight : Weights) {
            weight.second *= val;
        }
    }

    void TWordVector::operator+=(const TWordVector &other) {
        Y_ENSURE(this != &other);

        for (auto &p : other.Weights) {
            Weights[p.first] += p.second;
        }
    }

    TWordVector::TWordVector(THashMap<TString, double> &&weights)
            : Weights(weights) {
    }

    size_t TWordVector::Size() const {
        return Weights.size();
    }

    void TWordVector::Strip(size_t limit) {
        if (Size() <= limit) {
            return;
        }

        TVector<TPair> elements;
        elements.reserve(Weights.size());
        elements.insert(elements.end(), Weights.begin(), Weights.end());
        Y_ASSERT(elements.size() == Weights.size());

        std::nth_element(elements.begin(), elements.begin() + limit, elements.end(), Comparer);

        THashMap<TString, double> newWeights(elements.begin(), elements.begin() + limit);
        Weights.swap(newWeights);
    }

    void TWordVector::StripByMinWeight(double minWeight) {
        Y_ASSERT(minWeight >= 0);

        for (auto it = Weights.begin(); it != Weights.end();) {
            if (it->second < minWeight) {
                Weights.erase(it++);
            } else {
                ++it;
            }
        }
    }

    void TWordVector::StripByMinL2Share(double l2Share) {
        Y_ASSERT(l2Share > 0 && l2Share <= 1);
        TVector<TPair> elements;
        elements.reserve(Weights.size());
        elements.insert(elements.end(), Weights.begin(), Weights.end());
        Y_ASSERT(elements.size() == Weights.size());
        std::sort(elements.begin(), elements.end(), Comparer);
        TVector<double> normSum;
        normSum.reserve(elements.size());
        double sum = 0;
        for (const TPair& e : elements) {
            sum += e.second * e.second;
            normSum.push_back(sum);
        }

        const double limitVal = sum * l2Share * l2Share;
        const size_t limit = std::upper_bound(normSum.begin(), normSum.end(), limitVal) - normSum.begin() + 1;
        if (limit >= Weights.size()) {
            return;
        }

        THashMap<TString, double> newWeights(elements.begin(), elements.begin() + limit);
        Weights.swap(newWeights);

    }

    const THashMap<TString, double>& TWordVector::GetWeights() const {
        return Weights;
    }
}
