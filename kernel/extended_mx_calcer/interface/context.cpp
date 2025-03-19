#include "context.h"

#include "common.h"

namespace NExtendedMx {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TDebug
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TDebug::TDebug(bool enabled)
        : Enabled(enabled) {
    }

    TString TDebug::ToString() const {
        return SS.Str();
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCalcContext
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TCalcContext::TCalcContext(bool debug)
        : TBasedOn(NSc::TValue())
        , Debug(debug)
        , Success(true)
    {
    }

    TCalcContext::TCalcContext(NSc::TValue scheme)
        : TBasedOn(scheme)
        , Debug(Scheme().IsDebug())
        , Success(true)
    {
    }

    TCalcContext::TCalcContext(const TCalcContextProto& proto)
        : TCalcContext(
            proto.Value__ ?
                *proto.Value__ :
                ythrow yexception() << "nullptr based proto"
          )
    {
    }

    const NSc::TValue& TCalcContext::GetLog() const  {
        return *Scheme().Log();
    }

    NSc::TValue& TCalcContext::GetLog() {
        return *Scheme().Log().Value__;
    }

    TDebug& TCalcContext::DbgLog() {
        return Debug;
    }

    TCalcContext::TMetaProto TCalcContext::GetMeta() {
        return Scheme().Meta();
    }

    TCalcContext::TMetaConstProto TCalcContext::GetMeta() const {
        return Scheme().Meta();
    }

    TCalcContext::TResultProto TCalcContext::GetResult() {
        return Scheme().Result();
    }

    TCalcContext::TResultConstProto TCalcContext::GetResult() const {
        return Scheme().Result();
    }

    TMultiPredictConstProto TCalcContext::GetMultiPredict() const {
        return Scheme().Result().MultiPredict();
    }

    void TCalcContext::SetFailure() {
        Success = false;
    }

    bool TCalcContext::IsOk() const {
        return Success;
    }

    const NSc::TValue& TCalcContext::Root() const {
        return *Scheme()->Value__;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMeta
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void TMeta::RegisterAttr(const TKey& key, const NSc::TValue& value) {
        if (const auto* v = FindPtr(key)) {
            if (NSc::TValue::Equal(value, *v)) {
                return;
            }
            ythrow yexception() << "override same meta key: \"" << key << '"'
                << ". old value: " << *v
                << ". new value: " << value;
        }
        operator[](key) = value;
    }

    void TMeta::MergeUpdate(const TBundleConstProto& other) {
        for (const auto& kv : other.Meta()->Value__->GetDict()) {
            RegisterAttr(TString{kv.first}, kv.second);
        }
    }

    void TMeta::MergeUpdate(const TMeta& other) {
        for (const auto& item : other) {
            RegisterAttr(item.first, item.second);
        }
    }

    TMeta::TIter TMeta::begin() const {
        return TParent::begin();
    }

    TMeta::TIter TMeta::end() const {
        return TParent::end();
    }

    bool TMeta::has(const TKey& key) const {
        return TParent::contains(key);
    }

    const TMeta::TMappedType* TMeta::FindPtr(const TKey& key) const {
        return TParent::FindPtr(key);
    }

    const TDebug& TCalcContext::DbgLog() const {
        return Debug;
    }
} // NExtendedMx
