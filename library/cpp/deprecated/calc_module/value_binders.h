#pragma once

#include <util/generic/string.h>

/* Allow point initializing in the following manner:
      : FuncPointName(FuncPointName.Bind(this).To<&Method>(names))
      , CopyPointName(CopyPointName.Bind(this, value, names))

   For template modules first variant should look like:
      : FuncPointName(FuncPointName.Bind(this).template To<&TThisModule::Method>(names))
*/

namespace NPointValueBinders {
    template <class TBinderData>
    class TOwnerValueBinder {
    private:
        typedef typename TBinderData::TOwner TOwner;
        typedef typename TBinderData::TValue TValue;

        TOwner* const Owner;
        const TValue Value;
        TString Names;

    public:
        TOwnerValueBinder(TOwner* owner, TValue value, const TString& names = TString())
            : Owner(owner)
            , Value(value)
            , Names(names)
        {
        }

        typename TBinderData::TValue GetValue() const noexcept {
            return Value;
        }

        TOwner* GetOwner() const noexcept {
            return Owner;
        }
        const TString& GetNames() const {
            return Names;
        }
    };

    template <class TBinderData>
    class TOwnerBinder {
    private:
        typedef typename TBinderData::TOwner TOwner;
        TOwner* const Owner;

    public:
        TOwnerBinder(TOwner* owner)
            : Owner(owner)
        {
        }

        template <typename TBinderData::TMethod f>
        TOwnerValueBinder<TBinderData> To(const TString& names = TString()) {
            return TOwnerValueBinder<TBinderData>(Owner, &TBinderData::template Call<f>, names);
        }
        TOwnerValueBinder<TBinderData> To(typename TBinderData::TValue value, const TString& names = TString()) {
            return TOwnerValueBinder<TBinderData>(Owner, value, names);
        }
    };

}
