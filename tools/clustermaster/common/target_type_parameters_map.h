#pragma once

#include "target_type_parameters.h"

#include <util/generic/bt_exception.h>

template <typename TValue>
class TTargetTypeParametersMap {
public:
    typedef TTargetTypeParameters::TId TKey;
private:
    const TTargetTypeParameters* /* const */ Parameters;
    TVector<TValue> Map;
public:
    TTargetTypeParametersMap(const TTargetTypeParameters* parameters)
        : Parameters(parameters)
        , Map(parameters->GetCount())
    { }

    const TTargetTypeParameters* GetParameters() const {
        return Parameters;
    }

    void Swap(TTargetTypeParametersMap& that) {
        // Parameters are not swapped intentionally // nga@
        // DoSwap(Parameters, that.Parameters);
        DoSwap(Map, that.Map);
    }

    TValue& At(const TKey& key) {
        if (key.GetParameters() != Parameters)
            ythrow TWithBackTrace<yexception>() << "invalid key: parameters from another task";
        return Map.at(key.GetN());
    }

    const TValue& At(const TKey& key) const {
        return const_cast<const TValue&>(const_cast<TTargetTypeParametersMap*>(this)->At(key));
    }

    // legacy
    template <typename TAnyKey>
    TValue& operator[](const TAnyKey& key) {
        return At(key);
    }

    // legacy
    template <typename TAnyKey>
    const TValue& operator[](const TAnyKey& key) const {
        return At(key);
    }

    // legacy
    const TValue& At(ui32 n) const {
        return At(Parameters->GetId(n));
    }

    // legacy
    TValue& At(ui32 n) {
        return At(Parameters->GetId(n));
    }

    const TValue& operator[](const TKey& key) const {
        return At(key);
    }

    ui32 Size() const {
        return Parameters->GetCount();
    }

    bool Empty() const {
        return Parameters->Empty();
    }

    class TConstEnumerator {
    private:
        const TTargetTypeParametersMap* Map;
        TTargetTypeParameters::TIterator ParameterIterator;
    public:
        TConstEnumerator(
                const TTargetTypeParametersMap* map,
                const TTargetTypeParameters::TIterator& parameterIterator)
            : Map(map), ParameterIterator(parameterIterator)
        { }

        bool Next() {
            return ParameterIterator.Next();
        }

        TTargetTypeParameters::TId CurrentN() const {
            return ParameterIterator.CurrentN();
        }

        TTargetTypeParameters::TPath CurrentPath() const {
            return ParameterIterator.CurrentPath();
        }

        const TTargetTypeParameters::TIdPath& CurrentIdPath() const {
            return *ParameterIterator;
        }

        const TValue& operator*() const {
            return Map->At(ParameterIterator.CurrentN());
        }

        const TValue* operator->() const {
            return &**this;
        }
    };

    class TEnumerator: public TConstEnumerator {
    public:
        TEnumerator(
                TTargetTypeParametersMap* map,
                const TTargetTypeParameters::TIterator& parameterIterator)
            : TConstEnumerator(map, parameterIterator)
        { }

        TValue& operator*() const {
            return const_cast<TValue&>(TConstEnumerator::operator*());
        }

        TValue* operator->() const {
            return &**this;
        }
    };

    TConstEnumerator Enumerator() const {
        return TConstEnumerator(this, Parameters->Iterator());
    }

    TEnumerator Enumerator() {
        return TEnumerator(this, Parameters->Iterator());
    }

};
