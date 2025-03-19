#pragma once

#include "qd_cgi_strings.h"

#include <kernel/querydata/common/querydata_traits.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NQueryData {

    class TRequestSplitLimits {
    public:
        ui32 MaxParts = 32;  // max requests to generate
        ui32 MaxItems = 75;  // max items to be requested in a single request
        ui32 MaxLength = 7500; // max request length

    public:
        TRequestSplitLimits() = default;

        TRequestSplitLimits(ui32 maxParts, ui32 maxItems, ui32 maxLength = 7500u)
            : MaxParts(maxParts)
            , MaxItems(maxItems)
            , MaxLength(maxLength)
        {}

        static TRequestSplitLimits WithMaxItems(ui32 maxItems) {
            TRequestSplitLimits res;
            res.MaxItems = maxItems;
            return res;
        }
    };

    class TRequestSplitMeasure {
    public:
        ui32 SplittableCount = 0;
        ui32 SplittableSize = 0;
        ui32 SharedSize = 0;
    };

    ui32 CalculatePartsCount(const TRequestSplitMeasure&, const TRequestSplitLimits& limits);

    ui32 CountSize(const TStringBufs& items);
    ui32 CountSize(const TVector<TString>& items);

    std::pair<ui32, ui32> GetIndexRange(ui32 size, ui32 part, ui32 parts);

    TString VectorToString(const TStringBufs& vec, char sep);
    TString VectorToString(const TVector<TString>& vec, char sep);

    TString VectorToStringSplit(const TStringBufs& vec, char sep, ui32 part, ui32 parts);
    TString VectorToStringSplit(const TVector<TString>& vec, char sep, ui32 part, ui32 parts);

    void SetCgi(TString& res, TStringBuf name, TString buf, char sep);
    void SetCgiRoot(TString& res, TStringBuf name, const TString& val);
    void SetCgiRoot(TString& res, TStringBuf name, TStringBuf val);
    void SetCgiSub(TString& res, TStringBuf name, const TString& val);
    void SetCgiSub(TString& res, TStringBuf name, TStringBuf val);

    bool GetCgiParameter(NSc::TValue& val, TStringBuf sc, const TCgiParameters& par, TStringBuf cgi);

    int ParseCompressedCgiPropName(TStringBuf& name, ECgiCompression& comprType, TStringBuf cgiName);
    TString FormCompressedCgiPropName(const TStringBuf name, const ECgiCompression comprType, const int ver = 1);

    TString CompressCgi(const TStringBuf in, const ECgiCompression comprType);
    TString CompressCgi(const TStringBuf in, const TStringBuf comprType);

    bool DecompressCgi(TString& out, const TStringBuf in, const ECgiCompression comprType);
    bool DecompressCgi(TString& out, const TStringBuf in, const TStringBuf comprType);

    TString DecompressCgi(const TStringBuf in, const ECgiCompression comprType);
    TString DecompressCgi(const TStringBuf in, const TStringBuf comprType);

    bool NextToken(TStringBuf& tok, TStringBuf& lst, char sep);

    template <typename T>
    TVector<T> Split(TStringBuf v, char sep) {
        TVector<T> res;
        TStringBuf t;
        while (NextToken(t, v, sep)) {
            res.emplace_back();
            res.back() = t;
        }
        return res;
    }

    void InsertToSet(TSet<TString>&, const TStringBufs&);

    template <typename V, typename T>
    void GetDictKeys(TVector<V>& vs, const T& t) {
        vs.reserve(vs.size() + t.size());
        for (const auto& kv : t) {
            vs.emplace_back();
            vs.back() = kv.first;
        }
    }

    template <typename V, typename T>
    TVector<V> GetDictKeys(const T& t) {
        TVector<V> vs;
        GetDictKeys(vs, t);
        return vs;
    }

    template <typename V, typename T>
    void GetDictValues(TVector<V>& vs, const T& t) {
        vs.reserve(vs.size() + t.size());
        for (const auto& kv : t) {
            vs.emplace_back();
            vs.back() = kv.second;
        }
    }

    template <typename V, typename T>
    TVector<V> GetDictValues(const T& t) {
        TVector<V> vs;
        GetDictValues(vs, t);
        return vs;
    }

    template <typename V, typename T>
    TVector<V> GetValues(const T& t) {
        TVector<V> vs;
        vs.reserve(t.size());
        for (const auto& v : t) {
            vs.emplace_back();
            vs.back() = v;
        }
        return vs;
    }

    template <typename T>
    bool GetCgiParameter(T&& val, const TCgiParameters& par, TStringBuf cgi) {
        TCgiParameters::const_iterator it = par.find(cgi);

        if (par.end() != it) {
            val = it->second;
            return true;
        }

        return false;
    }

    template <typename T>
    void Minus(TVector<T>& result, const TVector<T>& toRemove) {
        result.resize(SetDifference(result.begin(), result.end(), toRemove.begin(), toRemove.end(), result.begin()) - result.begin());
    }

    TString JsonToString(const NSc::TValue& val);

}
