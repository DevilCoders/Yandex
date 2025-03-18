#pragma once

#include "factors.h"

namespace NAntiRobot {
    class TFactorNames {
    public:
        inline TFactorNames() {
            const size_t count = TAllFactors::AllFactorsCount();
            Names.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                Names.push_back(TAllFactors::GetFactorNameByIndex(i));
            }

            for (size_t i = 0; i < FactorsCount(); ++i) {
                Indexes[GetFactorNameByIndex(i)] = i;
            }
        }

        inline const TString& GetFactorNameByIndex(size_t index) const {
            if (index < Names.size()) {
                return Names[index];
            }

            ythrow yexception() << "incorrect index " << index;
        }

        const TString& GetLocalFactorNameByIndex(size_t index) const {
            if (index < TAllFactors::NumLocalFactors) {
                return Names[index];
            }

            ythrow yexception() << "incorrect index " << index;
        }

        inline size_t FactorsCount() const noexcept {
            return Names.size();
        }

        inline size_t GetFactorIndexByName(const TString& name) const {
            if (size_t index; TryGetFactorIndexByName(name, index)) {
                return index;
            }

            Cerr << "unknown factor: " << name.Quote() << Endl;
            ythrow yexception() << "unknown factor " << name.Quote();
        }

        inline bool TryGetFactorIndexByName(const TString& name, size_t& index) const {
            if (auto it = Indexes.find(name); it != Indexes.end()) {
                index = it->second;
                return true;
            }
            return false;
        }

        static inline TFactorNames* Instance() {
            return Singleton<TFactorNames>();
        }

    private:
        TVector<TString> Names;
        THashMap<TString, size_t> Indexes;
    };
} // namespace NAntiRobot
