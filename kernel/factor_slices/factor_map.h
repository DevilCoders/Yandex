#pragma once

#include "factor_domain.h"

namespace NFactorSlices {

    template<typename Value>
    class TFactorMap {
    public:
        template<class... Args>
        explicit TFactorMap(typename TTypeTraits<Value>::TFuncParam defValue, Args&&... args)
            : Domain(std::forward<Args>(args)...)
            , DefValue(defValue)
        {
            Map.resize(Domain.Size(), DefValue);
        }

        template<class... Args>
        explicit TFactorMap(Args... args)
                : TFactorMap(Value(), args...) {
        }

        const TFactorDomain &GetDomain() const {
            return Domain;
        }

        const Value &GetDefValue() const {
            return DefValue;
        }

        size_t Size() const {
            return Map.size();
        }

        Y_FORCE_INLINE Value operator[](TFactorIndex index) const {
            return Get(index);
        }

        Y_FORCE_INLINE Value &operator[](TFactorIndex index) {
            return Ref(index);
        }

        Y_FORCE_INLINE Value operator[](const TFullFactorIndex &index) const {
            return Get(index);
        }

        Y_FORCE_INLINE Value &operator[](const TFullFactorIndex &index) {
            return Ref(index);
        }

    protected:
        Y_FORCE_INLINE Value Get(TFactorIndex index, EFactorSlice slice = EFactorSlice::ALL) const {
            return Map[Domain[slice].GetIndex(index)];
        }

        Y_FORCE_INLINE Value &Ref(TFactorIndex index, EFactorSlice slice = EFactorSlice::ALL) {
            return Map[Domain[slice].GetIndex(index)];
        }

        Y_FORCE_INLINE Value Get(const TFullFactorIndex &index) const {
            return Get(index.Index, index.Slice);
        }

        Y_FORCE_INLINE Value &Ref(const TFullFactorIndex &index) {
            return Ref(index.Index, index.Slice);
        }

    private:
        TFactorDomain Domain;
        Value DefValue = Value();
        TVector<Value> Map;
    };

} // namespace NFactorSlices
