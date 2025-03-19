#pragma once

#include "qd_key.h"
#include "qd_merge.h"

#include <library/cpp/scheme/scheme.h>

#include <util/generic/buffer.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NQueryData {

    class TQueryData;

    class TQueryDataWrapper {
    public:
        class TImpl;
        TQueryDataWrapper();
        TQueryDataWrapper(const TQueryDataWrapper& b);
        TQueryDataWrapper(const TQueryData& qd, bool makecopy = true);
        ~TQueryDataWrapper();

        TQueryDataWrapper& operator = (const TQueryDataWrapper& b);

    public:
        void Init(TStringBuf serialized);
        void Init(const TQueryData&, bool makecopy = true);
        void Clear();

        bool HasSource(TStringBuf) const;
        bool HasKeyInSource(TStringBuf source) const;

        void GetSourceNames(TStringBufs& names) const;

        bool GetCommonJson(TStringBuf src, TStringBuf&) const;
        bool GetJson(TStringBuf src, const TKey& key, TStringBuf&) const;

        // returns false if there is no such source.
        void GetCommonFactorNames(TStringBuf src, TStringBufs& names) const;

        EFactorType GetCommonFactorType(TStringBuf src, TStringBuf name) const;

        // the common factors for the source. Returns false if there is no such factor.
        bool GetCommonFactor(TStringBuf src, TStringBuf name, i64& val) const;
        bool GetCommonFactor(TStringBuf src, TStringBuf name, float& val) const;
        bool GetCommonFactor(TStringBuf src, TStringBuf name, TString& val) const;

        void GetSourceKeys(TStringBuf src, TKeys& keys, bool append = false) const;
        void GetFactorNames(TStringBuf src, const TKey& key, TStringBufs& names) const;

        EFactorType GetFactorType(TStringBuf src, const TKey& key, TStringBuf name) const;

        // returns false only if there is no such factor.
        bool GetFactor(TStringBuf src, const TKey& key, TStringBuf name, i64& val) const;
        bool GetFactor(TStringBuf src, const TKey& key, TStringBuf name, float& val) const;
        bool GetFactor(TStringBuf src, const TKey& key, TStringBuf name, TString& val) const;

        bool GetTimestamp(TStringBuf src, const TKey& key, ui64&) const;
        bool GetCommonTimestamp(TStringBuf src, ui64& ts) const;

        // simplified API

        bool GetJson(TStringBuf src, TStringBuf&) const;
        bool GetTimestamp(TStringBuf src, ui64&) const;

        EFactorType GetFactorType(TStringBuf src, TStringBuf name) const;

        // the src is expected to have only one key in answer. Returns false if not.
        bool GetFactor(TStringBuf src, TStringBuf name, i64& val) const;
        bool GetFactor(TStringBuf src, TStringBuf name, float& val) const;
        bool GetFactor(TStringBuf src, TStringBuf name, TString& val) const;

        i64 GetFactorRobust(TStringBuf src, TStringBuf name, i64 def = 0) const;
        double GetFactorRobust(TStringBuf src, TStringBuf name, double def = 0) const;
        TString GetFactorRobust(TStringBuf src, TStringBuf name, const TString& def = TString()) const;

        i64 GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, i64 def = 0) const;
        double GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, double def = 0) const;
        TString GetKeyFactorRobust(TStringBuf src, const TKey& key, TStringBuf name, const TString& def = TString()) const;

        template <typename T>
        T GetFactorByForce(TStringBuf src, TStringBuf name, T def = T()) const {
            return GetFactorRobust(src, name, def);
        }

        template <typename T>
        T GetKeyFactorByForce(TStringBuf src, const TKey& key, TStringBuf name, T def = T()) const {
            return GetKeyFactorRobust(src, key, name, def);
        }

        void Serialize(IOutputStream& out) const;
        void Deserialize(TStringBuf in);

    private:
        TAtomicSharedPtr<TImpl> Impl;
    };

    TString HumanReadableQueryData(const TQueryData&, bool compact = true);

}
