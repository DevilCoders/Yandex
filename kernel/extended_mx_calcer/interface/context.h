#pragma once

#include "based_on.h"

#include <kernel/extended_mx_calcer/proto/typedefs.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NExtendedMx {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TDebug - save debug messages if <enabled> set, else do nothing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct TDebug {
        TDebug(bool enabled);

        template <typename T>
        TDebug& operator << (const T& val) {
            if (IsEnabled()) {
                SS << val;
            }
            return *this;
        }

        TString ToString() const;

        inline bool IsEnabled() const {
            return Enabled;
        }

    private:
        bool Enabled;
        TStringStream SS;
    };

    using TFeatureResultConst = TBasedOn<TFeatureResultConstProto>; // owning dict FeatureName -> FeatureResult

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCalcContext - context for calcers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class TCalcContext : TBasedOn<TCalcContextProto> {
    public:
        using TMetaProto = TCalcContextMetaProto;
        using TMetaConstProto = TCalcContextMetaConstProto;
        using TResultProto = TCalcContextResultProto;
        using TResultConstProto = TCalcContextResultConstProto;
        explicit TCalcContext(bool debug);
        explicit TCalcContext(NSc::TValue scheme = NSc::TValue());
        explicit TCalcContext(const TCalcContextProto& proto);

        template <typename TKey, typename TValue>
        void Log(const TString& alias, const TKey& key, const TValue& value) {
            TSchemeTraits::TValueRef log = Scheme().Log().GetMutable();
            (*log)[alias][key] = value;
        }

        const NSc::TValue& GetLog() const;
        NSc::TValue& GetLog();

        TDebug& DbgLog();
        const TDebug& DbgLog() const;

        TMetaProto GetMeta();
        TMetaConstProto GetMeta() const;
        TResultProto GetResult();
        TResultConstProto GetResult() const;
        TMultiPredictConstProto GetMultiPredict() const;
        void SetFailure();
        bool IsOk() const;

        const NSc::TValue& Root() const;

    private:
        TDebug Debug;
        bool Success;
    };


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMeta - meta attributes for calcers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class TMeta : private THashMap<TString, NSc::TValue> {
        using TParent = THashMap<key_type, mapped_type>;

    public:
        using TKey = key_type;
        using TMappedType = mapped_type;
        using TIter = const_iterator;
        using TValueType = value_type;
    public:
        void MergeUpdate(const TMeta& other);
        void MergeUpdate(const TBundleConstProto& other);
        void RegisterAttr(const TKey& key, const NSc::TValue& value = NSc::TValue::DefaultValue());

        TIter begin() const;
        TIter end() const;
        bool has(const TKey& key) const;
        const TMappedType* FindPtr(const TKey& key) const;
    };
} // NExtendedMx
