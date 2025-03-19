#include "querydata.h"
#include "qd_key_impl.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/common/qd_util.h>

#include <library/cpp/scheme/scheme.h>

#include <util/draft/datetime.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/memory/pool.h>
#include <util/folder/dirut.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/system/hostname.h>
#include <util/system/fasttime.h>

namespace NQueryData {

    using TSourceFactor = std::pair<TKey, TStringBuf>;

}

template <>
struct THash<NQueryData::TSourceFactor> {
    size_t operator()(const NQueryData::TSourceFactor& v) const {
        return CombineHashes(v.first.Hash(), ComputeHash(v.second));
    }
};

namespace NQueryData {

    class TQueryDataWrapper::TImpl {
    public:
        explicit TImpl(const TQueryData& qd, bool hold)
        {
            if (hold) {
                GetMutableData() = qd;
            } else {
                DataPtr = &qd;
            }
            Init();
        }

        explicit TImpl(TStringBuf serialized)
        {
            Y_PROTOBUF_SUPPRESS_NODISCARD GetMutableData().ParseFromArray(serialized.data(), serialized.size());
            Init();
        }

        void Init() {
            const TQueryData& data = GetData();
            const size_t nsrc = data.SourceFactorsSize();

            for (size_t i = 0; i < nsrc; ++i) {
                const TSourceFactors& src = data.GetSourceFactors(i);

                TFactorsVec* fvec = nullptr;
                if (src.GetCommon() || /*deprecated*/(src.HasSourceKey() && src.GetSourceKey().empty())) {
                    fvec = &CommonFactors[src.GetSourceName()];
                } else {
                    TKeyFactors::iterator it = KeyFactors.find(src.GetSourceName());
                    if (it == KeyFactors.end())
                        it = KeyFactors.insert(std::make_pair(TStringBuf(src.GetSourceName()), new TFactors(&Pool))).first;

                    fvec = &(*it->second)[MakeKey(src)];
                }

                const size_t nfacts = src.FactorsSize();

                fvec->Timestamp = GetTimestampFromVersion(src.GetVersion());

                for (size_t j = 0; j < nfacts; ++j) {
                    const TFactor& fact = src.GetFactors(j);
                    fvec->push_back(&fact);
                }

                if (src.HasJson()) {
                    fvec->Json = &src.GetJson();
                }
            }
        }

        const TKey* GetKey(TStringBuf src) const {
            TKeyFactors::const_iterator it = KeyFactors.find(src);
            if (it == KeyFactors.end())
                return nullptr;

            return it->second->size() == 1 ? &it->second->begin()->first : nullptr;
        }

        const TFactor* FindFactor(TStringBuf src, const TKey& key, TStringBuf name) const {
            TKeyFactors::const_iterator it = KeyFactors.find(src);
            if (it == KeyFactors.end())
                return nullptr;

            TFactors::const_iterator kit = it->second->find(key);
            if (kit == it->second->end())
                return nullptr;

            return NQueryData::FindFactor(kit->second, name);
        }

        const TFactor* FindCommonFactor(TStringBuf src, TStringBuf name) const {
            TCommonFactors::const_iterator it = CommonFactors.find(src);
            if (it == CommonFactors.end())
                return nullptr;

            return NQueryData::FindFactor(it->second, name);
        }

        void FindSources(TStringBufs& names) const {
            for (TKeyFactors::const_iterator it = KeyFactors.begin(); it != KeyFactors.end(); ++it) {
                names.push_back(it->first);
            }
        }

        void FindKeys(TStringBuf src, TKeys& keys) const {
            TKeyFactors::const_iterator it = KeyFactors.find(src);
            if (it == KeyFactors.end())
                return;

            for (TFactors::const_iterator kit = it->second->begin(); kit != it->second->end(); ++kit) {
                keys.push_back(&kit->first);
            }
        }

        bool HasSource(TStringBuf src) const {
            return KeyFactors.contains(src) || CommonFactors.contains(src);
        }

        bool HasKeyInSource(TStringBuf src) const {
            return KeyFactors.contains(src);
        }

        bool GetCommonJson(TStringBuf src, TStringBuf& sc) const {
            if (const TFactorsVec* fv = CommonFactors.FindPtr(src))
                return fv->GetJson(sc);
            return false;
        }

        bool GetJson(TStringBuf src, const TKey& key, TStringBuf& sc) const {
            if (const TSimpleSharedPtr<TFactors>* kf = KeyFactors.FindPtr(src)) {
                if (!!*kf) {
                    if (const TFactorsVec* fv = (*kf)->FindPtr(key))
                        return fv->GetJson(sc);
                }
            }

            return false;
        }

        bool GetTimestamp(TStringBuf src, const TKey& key, ui64& ts) const {
            if (const TSimpleSharedPtr<TFactors>* kf = KeyFactors.FindPtr(src)) {
                if (!!*kf) {
                    if (const TFactorsVec* fv = (*kf)->FindPtr(key)) {
                        ts = fv->Timestamp;
                        return true;
                    }
                }
            }

            return false;
        }

        bool GetCommonTimestamp(TStringBuf src, ui64& ts) const {
            if (const TFactorsVec* fv = CommonFactors.FindPtr(src)) {
                ts = fv->Timestamp;
                return true;
            }
            return false;
        }

        bool GetJson(TStringBuf src, TStringBuf& sc) const {
            if (const TSimpleSharedPtr<TFactors>* kf = KeyFactors.FindPtr(src)) {
                if (!!*kf && (*kf)->size() == 1)
                    return (*kf)->begin()->second.GetJson(sc);
            }

            return false;
        }

        bool GetTimestamp(TStringBuf src, ui64& ts) const {
            if (const TSimpleSharedPtr<TFactors>* kf = KeyFactors.FindPtr(src)) {
                if (!!*kf && (*kf)->size() == 1) {
                    ts = (*kf)->begin()->second.Timestamp;
                    return true;
                }
            }

            return false;
        }

        void FindNames(TStringBuf src, const TKey& key, TStringBufs& names) const {
            TKeyFactors::const_iterator it = KeyFactors.find(src);
            if (it == KeyFactors.end())
                return;

            TFactors::const_iterator jt = it->second->find(key);

            if (jt == it->second->end())
                return;

            for (TFactorsVec::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
                names.push_back((*kt)->GetName());
            }
        }

        void FindCommonFactorNames(TStringBuf src, TStringBufs& names) const {
            TCommonFactors::const_iterator it = CommonFactors.find(src);
            if (it == CommonFactors.end())
                return;

            for (TFactorsVec::const_iterator kt = it->second.begin(); kt != it->second.end(); ++kt) {
                names.push_back((*kt)->GetName());
            }
        }

        void Serialize(IOutputStream& out) const {
            GetData().SerializeToArcadiaStream(&out);
        }

        void Deserialize(TStringBuf in) {
            TQueryData& data = GetMutableData();
            data.Clear();
            Y_PROTOBUF_SUPPRESS_NODISCARD data.ParseFromArray(in.data(), in.size());
            Init();
        }

    private:
        TMemoryPool Pool{4048};
        THolder<TQueryData> DataHolder;
        const TQueryData* DataPtr = nullptr;

        const TQueryData& GetData() const {
            return !DataPtr ? Default<TQueryData>() : *DataPtr;
        }

        TQueryData& GetMutableData() {
            if (!DataHolder) {
                DataHolder.Reset(new TQueryData);
                DataPtr = DataHolder.Get();
            }

            return *DataHolder;
        }

    public:
        TKeyFactors KeyFactors{&Pool};
        TCommonFactors CommonFactors{&Pool};
    };

    TQueryDataWrapper::TQueryDataWrapper() {}

    TQueryDataWrapper::~TQueryDataWrapper() { }

    TQueryDataWrapper::TQueryDataWrapper(const TQueryDataWrapper& b)
        : Impl(b.Impl)
    {
    }

    TQueryDataWrapper::TQueryDataWrapper(const TQueryData& b, bool hold)
    {
        Init(b, hold);
    }

    TQueryDataWrapper& TQueryDataWrapper::operator = (const TQueryDataWrapper& b) {
        Impl = b.Impl;
        return *this;
    }

    void TQueryDataWrapper::Init(const TQueryData& qd, bool hold) {
        Impl.Reset(new TImpl(qd, hold));
    }

    void TQueryDataWrapper::Init(TStringBuf serialized) {
        Impl.Reset(new TImpl(serialized));
    }

    bool TQueryDataWrapper::HasSource(TStringBuf src) const {
        return !!Impl && Impl->HasSource(src);
    }

    bool TQueryDataWrapper::HasKeyInSource(TStringBuf src) const {
        return !!Impl && Impl->HasKeyInSource(src);
    }

    void TQueryDataWrapper::GetSourceNames(TStringBufs& names) const {
        names.clear();

        if (!Impl)
            return;

        Impl->FindSources(names);
    }

    bool TQueryDataWrapper::GetCommonJson(TStringBuf src, TStringBuf& sc) const {
        return !!Impl && Impl->GetCommonJson(src, sc);
    }

    bool TQueryDataWrapper::GetJson(TStringBuf src, const TKey& key, TStringBuf& sc) const {
        return !!Impl && Impl->GetJson(src, key, sc);
    }

    bool TQueryDataWrapper::GetTimestamp(TStringBuf src, const TKey& key, ui64& ts) const {
        return !!Impl && Impl->GetTimestamp(src, key, ts);
    }

    bool TQueryDataWrapper::GetCommonTimestamp(TStringBuf src, ui64& ts) const {
        return !!Impl && Impl->GetCommonTimestamp(src, ts);
    }

    void TQueryDataWrapper::GetSourceKeys(TStringBuf src, TKeys& keys, bool append) const {
        if (!append) {
            keys.clear();
        }

        if (!Impl)
            return;

        Impl->FindKeys(src, keys);
    }

    void TQueryDataWrapper::GetFactorNames(TStringBuf src, const TKey& key, TStringBufs& names) const {
        names.clear();

        if (!Impl)
            return;

        Impl->FindNames(src, key, names);
    }

    void TQueryDataWrapper::GetCommonFactorNames(TStringBuf src, TStringBufs& names) const {
        names.clear();

        if (!Impl)
            return;

        Impl->FindCommonFactorNames(src, names);
    }

    bool TQueryDataWrapper::GetJson(TStringBuf src, TStringBuf& sc) const {
        return !!Impl && Impl->GetJson(src, sc);
    }

    bool TQueryDataWrapper::GetTimestamp(TStringBuf src, ui64& ts) const {
        return !!Impl && Impl->GetTimestamp(src, ts);
    }

    template <typename T>
    bool DoGetFactor(const TQueryDataWrapper& wr, const TQueryDataWrapper::TImpl* impl,
                     TStringBuf src, TStringBuf name, T& val) {
        if (const TKey* key = !impl ? nullptr : impl->GetKey(src))
            return wr.GetFactor(src, *key, name, val);
        return false;
    }

    template <typename T>
    struct TFactorGetter {
    };

    template <>
    struct TFactorGetter<i64> {
        static bool Get(const TFactor* fact, i64& res) {
            if (!(fact && fact->HasIntValue()))
                return false;
            res = fact->GetIntValue();
            return true;
        }
    };

    template <>
    struct TFactorGetter<float> {
        static bool Get(const TFactor* fact, float& res) {
            if (!(fact && fact->HasFloatValue()))
                return false;
            res = fact->GetFloatValue();
            return true;
        }
    };

    template <>
    struct TFactorGetter<TString> {
        static bool Get(const TFactor* fact, TString& res) {
            if (!fact)
                return false;

            if (fact->HasStringValue()) {
                res = fact->GetStringValue();
                return true;
            }

            if (fact->HasBinaryValue()) {
                Base64Decode(fact->GetBinaryValue(), res);
                return true;
            }

            return false;
        }
    };

    bool TQueryDataWrapper::GetFactor(TStringBuf src, TStringBuf name, i64& val) const {
        return DoGetFactor(*this, Impl.Get(), src, name, val);
    }

    bool TQueryDataWrapper::GetFactor(TStringBuf src, TStringBuf name, float& val) const {
        return DoGetFactor(*this, Impl.Get(), src, name, val);
    }

    bool TQueryDataWrapper::GetFactor(TStringBuf src, TStringBuf name, TString& val) const {
        return DoGetFactor(*this, Impl.Get(), src, name, val);
    }

    bool TQueryDataWrapper::GetFactor(TStringBuf src, const TKey& key, TStringBuf name, i64& val) const {
        return TFactorGetter<i64>::Get(!Impl ? nullptr : Impl->FindFactor(src, key, name), val);
    }

    bool TQueryDataWrapper::GetFactor(TStringBuf src, const TKey& key, TStringBuf name, float& val) const {
        return TFactorGetter<float>::Get(!Impl ? nullptr : Impl->FindFactor(src, key, name), val);
    }

    bool TQueryDataWrapper::GetFactor(TStringBuf src, const TKey& key, TStringBuf name, TString& val) const {
        return TFactorGetter<TString>::Get(!Impl ? nullptr : Impl->FindFactor(src, key, name), val);
    }

    bool TQueryDataWrapper::GetCommonFactor(TStringBuf src, TStringBuf name, i64& val) const {
        return TFactorGetter<i64>::Get(!Impl ? nullptr : Impl->FindCommonFactor(src, name), val);
    }

    bool TQueryDataWrapper::GetCommonFactor(TStringBuf src, TStringBuf name, float& val) const {
        return TFactorGetter<float>::Get(!Impl ? nullptr : Impl->FindCommonFactor(src, name), val);
    }

    bool TQueryDataWrapper::GetCommonFactor(TStringBuf src, TStringBuf name, TString& val) const {
        return TFactorGetter<TString>::Get(!Impl ? nullptr : Impl->FindCommonFactor(src, name), val);
    }

    static EFactorType DoGetFactorType(const TFactor* fact) {
        if (!fact)
            return FT_NONE;

        if (fact->HasStringValue() || fact->HasBinaryValue())
            return FT_STRING;

        if (fact->HasFloatValue())
            return FT_FLOAT;

        if (fact->HasIntValue())
            return FT_INT;

        return FT_NONE;
    }

    EFactorType TQueryDataWrapper::GetCommonFactorType(TStringBuf src, TStringBuf name) const {
        return DoGetFactorType(!Impl ? nullptr : Impl->FindCommonFactor(src, name));
    }

    EFactorType TQueryDataWrapper::GetFactorType(TStringBuf src, const TKey& key, TStringBuf name) const {
        return DoGetFactorType(!Impl ? nullptr : Impl->FindFactor(src, key, name));
    }

    EFactorType TQueryDataWrapper::GetFactorType(TStringBuf src, TStringBuf name) const {
        if (!Impl)
            return FT_NONE;
        if (const TKey* key = Impl->GetKey(src))
            return GetFactorType(src, *key, name);
        return FT_NONE;
    }

    template <typename T>
    static T DoGetFactorByForce(const TFactor* fac, T def) {
        switch (DoGetFactorType(fac)) {
        default:
            return def;
        case FT_INT:
            return fac->GetIntValue();
        case FT_FLOAT:
            return fac->GetFloatValue();
        case FT_STRING:
            try {
                TString res;
                TFactorGetter<TString>::Get(fac, res);
                return FromString<T>(res);
            } catch(...) {
                return def;
            }
        }
        return def; // to make compiler happy
    }

    static TString DoGetFactorByForce(const TFactor* fac, const TString& def) {
        switch (DoGetFactorType(fac)) {
        default:
            return def;
        case FT_INT:
            return ToString(fac->GetIntValue());
        case FT_FLOAT:
            return ToString(fac->GetFloatValue());
        case FT_STRING:
            try {
                TString res;
                TFactorGetter<TString>::Get(fac, res);
                return res;
            } catch(...) {
                return def;
            }
        }
        return def; // to make compiler happy
    }

    i64 TQueryDataWrapper::GetFactorRobust(TStringBuf src, TStringBuf name, i64 def) const {
        if (const TKey* key = !Impl ? nullptr : Impl->GetKey(src))
            return GetKeyFactorRobust(src, *key, name, def);
        return def;
    }

    i64 TQueryDataWrapper::GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, i64 def) const {
        const TFactor* fac = !Impl ? nullptr : Impl->FindFactor(src, key, name);
        return DoGetFactorByForce(fac, def);
    }

    double TQueryDataWrapper::GetFactorRobust(TStringBuf src, TStringBuf name, double def) const {
        if (const TKey* key = !Impl ? nullptr : Impl->GetKey(src))
            return GetKeyFactorRobust(src, *key, name, def);
        return def;
    }

    double TQueryDataWrapper::GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, double def) const {
        const TFactor* fac = !Impl ? nullptr : Impl->FindFactor(src, key, name);
        return DoGetFactorByForce(fac, def);
    }

    TString TQueryDataWrapper::GetFactorRobust(TStringBuf src, TStringBuf name, const TString& def) const {
        if (const TKey* key = !Impl ? nullptr : Impl->GetKey(src))
            return GetKeyFactorRobust(src, *key, name, def);
        return def;
    }

    TString TQueryDataWrapper::GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, const TString& def) const {
        const TFactor* fac = !Impl ? nullptr : Impl->FindFactor(src, key, name);
        return DoGetFactorByForce(fac, def);
    }

    void TQueryDataWrapper::Clear() {
        Impl = nullptr;
    }

    TString HumanReadableQueryData(const TQueryData& d, bool compact) {
        return HumanReadableProtobuf(d, compact);
    }

    void TQueryDataWrapper::Serialize(IOutputStream& out) const {
        Impl->Serialize(out);
    }

    void TQueryDataWrapper::Deserialize(TStringBuf in) {
        Impl->Deserialize(in);
    }

}
