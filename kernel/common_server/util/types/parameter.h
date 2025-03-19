#pragma once

#include "interval.h"

#include <library/cpp/json/json_value.h>

#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/lazy_value.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>
#include <util/system/yassert.h>

namespace NUtil {
    template <class TOwner>
    class IParameter : TNonCopyable {
    private:
        TOwner& Owner;
        const TString Name;

    protected:
        virtual bool Defined() const = 0;

    public:
        inline IParameter(TOwner& owner,
            const TString& name)
            : Owner(owner)
            , Name(name)
        {
            Owner.RegisterParameter(Name, this);
        }

        inline IParameter(TOwner& owner,
            const IParameter<TOwner>& parameter)
            : Owner(owner)
            , Name(parameter.Name)
        {
            Owner.RegisterParameter(Name, this);
        }

        virtual inline ~IParameter() {}

        inline const TString& GetName() const
        {
            return Name;
        }

        inline bool operator! () const
        {
            return !Defined();
        }

        virtual void CopyFrom(const IParameter<TOwner>& other) {
            Parse(other.ToString());
        }

        virtual void Parse(const TString& text) = 0;
        virtual TString ToString() const = 0;
    };

    template <class TOwner, class TType, bool Strict = true>
    class TBaseParameter : public IParameter<TOwner> {
    protected:
        using IBase = IParameter<TOwner>;
        using IBase::GetName;
        using IBase::IBase;

        TCopyPtr<const TType> Value;

    protected:
        template <class TParser>
        inline void CheckedParse(const TString& text, TParser parser)
        {
            try {
                if (Value && Strict) {
                    throw yexception() << "Parameter already has value: " << TType(*Value);
                }

                Value.Reset(new TType(TType(parser(text))));
            } catch (const yexception& exc) {
                throw yexception() << "Error while parsing " << text.Quote()
                    << " for parameter " << GetName().Quote() << ":\n\t"
                    << exc.what();
            }
        }

    public:
        virtual void Parse(const TString& text) override
        {
            TType(*func)(const TString&) = FromString<TType>;
            CheckedParse(text, func);
        }
    };

    template <class TOwner, class TType, bool Strict = true>
    class TParameter : public TBaseParameter<TOwner, TType, Strict> {
    private:
        using TBase = TBaseParameter<TOwner, TType, Strict>;
        using TBase::TBase;
        using TBase::GetName;
        using TBase::Value;

        const TCopyPtr<const TType> DefaultValue;

    public:
        inline TParameter(TOwner& owner,
            const TString& name,
            typename TTypeTraits<TType>::TFuncParam defaultValue)
            : TBase(owner, name)
            , DefaultValue(new TType(defaultValue))
        {
        }

        inline TParameter(TOwner& owner,
            const TParameter<TOwner, TType>& parameter)
            : TBase(owner, parameter)
            , DefaultValue(parameter.DefaultValue)
        {
            Value = parameter.Value;
        }

        virtual TString ToString() const override
        {
            return ::ToString(operator typename TTypeTraits<TType>::TFuncParam());
        }

        virtual bool Defined() const override
        {
            return Value || DefaultValue;
        }

        inline operator typename TTypeTraits<TType>::TFuncParam() const
        {
            if (!!Value) {
                return *Value;
            } else if (!!DefaultValue) {
                return *DefaultValue;
            } else {
                throw yexception() << "No value set for parameter "
                    << GetName().Quote();
            }
        }

        inline TParameter<TOwner, TType, Strict>& operator= (typename TTypeTraits<TType>::TFuncParam value)
        {
            Value.Reset(new TType(value));
            return *this;
        }
    };

    template <class TOwner, class TType, bool Strict = true>
    class TSynonymParameter : public TParameter<TOwner, TType, Strict> {
    private:
        const TVector<TString> SynonymNames;
    private:
        inline const TVector<TString>& GetSynonymNames() const
        {
            return SynonymNames;
        }

        void RegisterSynonyms(TOwner& owner) {
            for (const auto& name : SynonymNames) {
                owner.RegisterParameter(name, this);
            }
        }

    public:
        template <class... TArgs>
        inline TSynonymParameter(TOwner& owner, const TString& name, typename TTypeTraits<TType>::TFuncParam defaultValue, const TArgs&... synonymNames)
            : TParameter<TOwner, TType, Strict>(owner, name, defaultValue)
            , SynonymNames(std::initializer_list<TString>{ synonymNames... })
        {
            RegisterSynonyms(owner);
        }

        inline TSynonymParameter(TOwner& owner,
            const TParameter<TOwner, TType>& parameter)
            : TParameter<TOwner, TType, Strict>(owner, parameter)
            , SynonymNames(parameter.GetSynonymNames())
        {
            RegisterSynonyms(owner);
        }
    };

    template <class TOwner, class TType, bool Strict = true>
    class TLazyDefaultParameter : public TBaseParameter<TOwner, TType, Strict> {
    private:
        using TBase = TBaseParameter<TOwner, TType, Strict>;
        using TBase::Value;
        const TLazyValue<TType> DefaultValue;

    public:
        template <class F>
        inline TLazyDefaultParameter(TOwner& owner, const TString& name, F&& f)
            : TBase(owner, name)
            , DefaultValue(std::forward<F>(f))
        {
        }

        virtual TString ToString() const override
        {
            return ::ToString(operator typename TTypeTraits<TType>::TFuncParam());
        }

        virtual bool Defined() const override
        {
            return true;
        }

        inline operator typename TTypeTraits<TType>::TFuncParam() const
        {
            return Value ? *Value : *DefaultValue;
        }

        inline TParameter<TOwner, TType, Strict>& operator= (typename TTypeTraits<TType>::TFuncParam value)
        {
            Value.Reset(new TType(value));
            return *this;
        }
    };

    template <class TOwner, class TType>
    class TClampedParameter : public TParameter<TOwner, TType> {
    private:
        typedef TParameter<TOwner, TType> TBase;
        const TInterval<TType> Interval;

        class TIntervalChecker
        {
        private:
            const TInterval<TType>& Interval;

        public:
            inline TIntervalChecker(const TInterval<TType>& interval)
                : Interval(interval)
            {
            }

            inline TType operator () (const TString& text)
            {
                TType value(FromString(text));
                if (!Interval.Check(value)) {
                    throw yexception() << "Value " << value
                        << " doesn't match interval " << Interval.ToString();
                }
                return value;
            }
        };

    public:
        inline TClampedParameter(TOwner& owner, const TString& name,
            const TInterval<TType>& interval)
            : TBase(owner, name)
            , Interval(interval)
        {
        }

        inline TClampedParameter(TOwner& owner, const TString& name,
            const TInterval<TType>& interval,
            typename TTypeTraits<TType>::TFuncParam defaultValue)
            : TBase(owner, name, defaultValue)
            , Interval(interval)
        {
            Y_ASSERT(Interval.Check(defaultValue));
        }

        inline TClampedParameter(TOwner& owner,
            const TClampedParameter<TOwner, TType>& parameter)
            : TBase(owner, parameter)
            , Interval(parameter.Interval)
        {
        }

        inline void Parse(const TString& text)
        {
            this->CheckedParse(text, TIntervalChecker(Interval));
        }
    };

    template <class TOwner, class TType>
    class TSelectableParameter : public TParameter<TOwner, TType> {
    private:
        typedef TParameter<TOwner, TType> TBase;
        const TVector<TString> Values;

        class TValuesChecker
        {
        private:
            const TVector<TString>& Values;

        public:
            inline TValuesChecker(const TVector<TString>& values)
                : Values(values)
            {
            }

            inline TType operator () (const TString& text)
            {
                TVector<TString>::const_iterator iter =
                    Find(Values.begin(), Values.end(), text);
                if (iter == Values.end()) {
                    throw yexception() << text.Quote()
                        << " wasn't found in list: "
                        << JoinStrings(Values, ", ").Quote();
                }
                return TType(iter - Values.begin());
            }
        };

    public:
        inline TSelectableParameter(TOwner& owner, const TString& name,
            const TVector<TString>& values)
            : TBase(owner, name)
            , Values(values)
        {
            Y_ASSERT(Values.size());
        }

        inline TSelectableParameter(TOwner& owner, const TString& name,
            const TVector<TString>& values,
            typename TTypeTraits<TType>::TFuncParam defaultValue)
            : TBase(owner, name, defaultValue)
            , Values(values)
        {
            Y_ASSERT(Values.size());
        }

        inline TSelectableParameter(TOwner& owner,
            const TSelectableParameter<TOwner, TType>& parameter)
            : TBase(owner, parameter)
            , Values(parameter.Values)
        {
        }

        inline void Parse(const TString& text)
        {
            this->CheckedParse(text, TValuesChecker(Values));
        }
    };

    template <class TDerived>
    class TParser: NNonCopyable::TNonCopyable {
    protected:
        typedef TMap<TString, IParameter<TDerived>*> TParameters;
        typedef TDerived TThis;

        template <class T>
        using TArgument = NUtil::TParameter<TThis, T, false>;

    protected:
        TParameters Parameters;

    protected:
        void CopyFrom(const TParser<TDerived>& other) {
            for (auto&& p : other.Parameters) {
                Y_ASSERT(Parameters.has(p.first));
                Parameters[p.first]->CopyFrom(*p.second);
            }
        }

    public:
        void RegisterParameter(const TString& name, IParameter<TDerived>* parameter) {
            Y_ASSERT(!Parameters.count(name));
            Parameters[name] = parameter;
        }
    };

    template <class TDerived>
    class TJsonParser: public TParser<TDerived> {
    public:
        NJson::TJsonValue SerializeArguments() const {
            NJson::TJsonValue result;
            for (auto&& p : TParser<TDerived>::Parameters) {
                const auto& parameter = *p.second;
                if (!parameter)
                    continue;

                result.InsertValue(p.first, parameter.ToString());
            }
            return result;
        }
        void DeserializeArguments(const NJson::TJsonValue& json) {
            for (auto&& p : TParser<TDerived>::Parameters) {
                if (!json.Has(p.first))
                    continue;

                p.second->Parse(json[p.first].GetStringRobust());
            }
        }
    };
}
