#pragma once

#include "defs.h"

#include <kernel/mango/common/protos.h>
#include <kernel/mango/common/constraints.h>

#include <library/cpp/binsaver/bin_saver.h>

#include <util/generic/map.h>
#include <util/generic/singleton.h>

#include <type_traits>

namespace NMango
{
    class TFactors;

    class TFactorsManager
    {
        THashMap<TString, int> FactorMap;
        THashMap<int, TString> NameById;

    private:
        void Init();

    public:
        class TDescriptor
        {
            int Id;
            TString Name;

        public:
            TDescriptor(int id = -1, const TString &name = TString())
                : Id(id)
                , Name(name)
            {}
            int GetId() const { return Id; }
            const TString& GetName() const { return Name; }

            int operator &(IBinSaver& f);
        };

        void Register(const TString &name, int id);
        static const TFactorsManager* InstanceConst() { static const TFactorsManager fm; return &fm; }

        // returns -1 if factor is not exists
        int GetId(const TString &name, bool throwOnFail = false) const;
        const TString& GetName(int id, bool throwOnFail = true) const;

        void List(TVector<TDescriptor>& factors) const;

        TFactorsManager() {
            Init();
        }
    };

    template<TFactorGroups Group, typename TFactorsRef, typename TFactor>
    class TAccessorBase
    {
        TFactorsRef Owner;
    public:
        TAccessorBase(TFactorsRef owner)
            : Owner(owner)
        {}

        template <typename TLineType>
        static Y_FORCE_INLINE int GetGlobalId(TLineType id)
        {
            static_assert(std::is_same<TLineType, typename TGroupTraits<Group>::TLineType>::value, "ERROR__INVALID_FACTOR_GROUP_TYPE");
            Y_ASSERT(id >= 0 && id < static_cast<TLineType>(TGroupTraits<Group>::Size));
            return TGroupTraits<Group>::GetGlobalId(id);
        }

        template <typename TLineType>
        Y_FORCE_INLINE TFactor operator[] (TLineType id)
        {
            return Owner[GetGlobalId(id)];
        }

        void DumpFactorValues(TVector<float>& factors) const
        {
            factors.reserve(factors.size() + TGroupTraits<Group>::Size);
            for (int i = TGroupTraits<Group>::Offset; i < TGroupTraits<Group>::EndOffset; ++i) {
                factors.push_back(Owner[i]);
            }
        }

        static void DumpFactorNames(TVector<TString>& factorNames)
        {
            factorNames.reserve(factorNames.size() + TGroupTraits<Group>::Size);
            for (int i = TGroupTraits<Group>::Offset; i < TGroupTraits<Group>::EndOffset; ++i) {
                factorNames.push_back(TFactorsManager::InstanceConst()->GetName(i));
            }
        }
    };

    template<TFactorGroups Group>
    class TAccessor : public TAccessorBase<Group, float*, float&>
    {
    public:
        TAccessor(float* owner)
            : TAccessorBase<Group, float*, float&>(owner)
        {
        }

        template<TFactorGroups othGroup>
        void CopyFrom(TAccessor<othGroup>& other)
        {
            static_assert(std::is_same<typename TGroupTraits<Group>::TLineType, typename TGroupTraits<othGroup>::TLineType>::value, "ERROR__INVALID_FACTOR_GROUP_TYPE");
            for (int i = 0; i < TGroupTraits<Group>::Size; ++i) {
                (*this)[static_cast<typename TGroupTraits<Group>::TLineType>(i)] = other[static_cast<typename TGroupTraits<Group>::TLineType>(i)];
            }
        }
    };

    template<TFactorGroups Group>
    class TConstAccessor : public TAccessorBase<Group, const float*, float>
    {
    public:
        TConstAccessor(const float* owner)
            : TAccessorBase<Group, const float*, float>(owner)
        {
        }
    };


    class TFactors
    {
        TVector<float> Values;
    public:
        TFactors();
        TFactors(const float *values);

        static size_t GetFactorsCount();

        int operator &(IBinSaver& f);
        float& operator[] (int id);
        float operator[] (int id) const;

        template<TFactorGroups Group, typename TLineType>
        float Get(TLineType id) const {
            return TConstAccessor<Group>(this->Get())[id];
        }

        template<TFactorGroups Group, typename TLineType>
        void Set(TLineType id, float value)
        {
            TAccessor<Group>(this->Get())[id] = value;
        }

        template<TFactorGroups Group>
        void Apply(const TFactors &other)
        {
            typedef typename TGroupTraits<Group>::TLineType TLineType;
            TConstAccessor<Group> source(other.Get());
            TAccessor<Group> dest(this->Get());
            for (int i = 0; i < TGroupTraits<Group>::Size; ++i)
                dest[static_cast<TLineType>(i)] = source[static_cast<TLineType>(i)];
        }
        void ApplyAll(const TFactors &other);

        void Clear();
        size_t Size() const { return Values.size(); };
        void Swap(TFactors& other);
        bool IsDefined(int id) const;

        void Save(IOutputStream& out) const;
        void Load(IInputStream& inp);
        void Save(TString& out) const;
        void Load(const TString& inp);
        void CopyToArray(float *dest);
        float* Get() { return Values.data(); }
        const float* Get() const { return Values.data(); }

        void Print() const;
        TString ToString() const;

        static void ClearArray(float *factors);
    };


    template<class TContainer>
    void ExtractFactors(const TContainer& container, TFactors& factors)
    {
        for (size_t factor_id = 0, factor_end = container.FactorsSize(); factor_id < factor_end; ++factor_id) {
            const NMango::TIndexFactor& factor = container.factors(factor_id);
            if (factor.HasId() && factor.HasValue()) {
                factors[factor.GetId()] = factor.GetValue();
            }
        }
    }

    class TDecayCalculator
    {
        float FinishTime, K;
        mutable float LastResult;
        mutable time_t LastTime;

    public:
        TDecayCalculator() {}
        TDecayCalculator(time_t start, time_t finish, float fading);
        float CalcDecay(time_t time) const;
    };
}
