#include "qd_search.h"
#include "qd_source.h"

#include <kernel/querydata/common/qd_source_names.h>
#include <kernel/querydata/request/qd_genrequests.h>
#include <kernel/querydata/scheme/qd_scheme.h>
#include <kernel/querydata/tries/qd_trie.h>

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/searchlog/errorlog.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

namespace NQueryData {

    static void FillFactor(TFactor* fac, const TRawFactor& f, const TFileDescription& fd) {
        fac->SetName(fd.GetFactorsMeta(f.GetId()).GetName());

        if (f.HasIntValue()) {
            fac->SetIntValue(f.GetIntValue());
        }
        if (f.HasFloatValue()) {
            fac->SetFloatValue(f.GetFloatValue());
        }
        if (f.HasStringValue()) {
            fac->SetStringValue(f.GetStringValue());
        }
        if (f.HasBinaryValue()) {
            fac->SetBinaryValue(Base64Encode(f.GetBinaryValue()));
        }
    }

    struct TFillOpts {
        TString TrieName;
        bool AddDebugInfo = false;
        bool PatchKeys = true;
    };

    static TSourceFactors* FillCommonFactors(TQueryData& obj, const TFileDescription& fd, TFillOpts opts) {
        if (!fd.CommonFactorsSize() && !fd.HasCommonJson() /* || !fd.GetAllowedCommon() */) {
            return nullptr;
        }

        TSourceFactors* facts = obj.AddSourceFactors();
        facts->SetSourceName(fd.GetSourceName());
        facts->SetVersion(GetTimestampFromVersion(fd.GetVersion()));
        facts->SetCommon(true);

        if (opts.AddDebugInfo) {
            facts->SetTrieName(opts.TrieName);
        }

        if (fd.HasCommonJson()) {
            facts->SetJson(fd.GetCommonJson());
        }

        for (ui32 i = 0; i < fd.CommonFactorsSize(); ++i) {
            TFactor* fac = facts->AddFactors();
            fac->CopyFrom(fd.GetCommonFactors(i));
        }

        return facts;
    }

    static void FillSourceFactors(TQueryData& obj, const TRawQueryData& data, const TSearchBuffer& buffer,
                                             const TRawMemoryTuple& tuple, const TFileDescription& fd, TFillOpts opts)
    {
        TSourceFactors* facts = obj.AddSourceFactors();
        try {
            facts->SetSourceName(fd.GetSourceName());
            facts->SetVersion(GetTimestampFromVersion(data.HasVersion() ? data.GetVersion() : fd.GetVersion()));

            if (opts.AddDebugInfo) {
                facts->SetTrieName(opts.TrieName);
                if (fd.HasShardNumber()) {
                    facts->SetShardNumber(fd.GetShardNumber());
                }
                if (fd.HasShards()) {
                    facts->SetShardsTotal(fd.GetShards());
                }
            }

            FillSourceFactorsKeys(*facts, tuple, buffer.Subkeys, buffer.KeyTypes, opts.PatchKeys);

            if (data.HasJson()) {
                facts->SetJson(data.GetJson());
            }

            const size_t nfacts = data.FactorsSize();
            for (size_t i = 0; i < nfacts; ++i) {
                TFactor* fac = facts->AddFactors();
                FillFactor(fac, data.GetFactors(i), fd);
            }
        } catch (const yexception& e) {
            SEARCH_ERROR << "Exception while filling response: " << e.what();
            obj.MutableSourceFactors()->RemoveLast();
        }
    }

    static void ProcessResult(TQueryData& result, TQDTrie::TValue& fb, TStringBuf rawData,
                                       const TSearchBuffer& buffer, const TRawMemoryTuple& tuple, const TSource& src, TFillOpts opts)
    {
        TRawQueryData data;
        if (!data.ParseFromArray(rawData.data(), rawData.size())) {
            SEARCH_ERROR << "Could not parse " << src.GetFileBaseName() << " response";
            return;
        }

        if (data.HasKeyRef()) {
            if (src.Find(data.GetKeyRef(), fb)) {
                rawData = fb.Get();
                if (!data.ParseFromArray(rawData.data(), rawData.size())) {
                    SEARCH_ERROR << "Could not parse " << src.GetFileBaseName() << " response";
                    return;
                }
            } else {
                return;
            }
        }

        FillSourceFactors(result, data, buffer, tuple, src.GetDescr(), opts);
    };

    static void FillKeyTypes(TKeyTypes& keyTypes, const TFileDescription& descr) {
        keyTypes.clear();
        keyTypes.push_back(descr.GetKeyType());
        keyTypes.insert(keyTypes.end(), descr.GetSubkeyTypes().begin(), descr.GetSubkeyTypes().end());
    }

    static bool HasSourceName(const TSet<TString>& names, TStringBuf srcName, TStringBuf mainName) {
        return names.contains(srcName) || names.contains(mainName);
    }

    void TSearchVisitor::Visit(const TSource& src) {
        TStringBuf srcName = src.GetDescr().GetSourceName();
        TStringBuf mainSrcName = GetMainSourceName(srcName);

        if (!Request.FilterNamespaces.empty() && !HasSourceName(Request.FilterNamespaces, srcName, mainSrcName)) {
            return;
        }

        if (!Request.FilterFiles.empty() && !Request.FilterFiles.contains(src.GetFileBaseName())) {
            return;
        }

        if (HasSourceName(Request.SkipNamespaces, srcName, mainSrcName)) {
            return;
        }

        if (Request.SkipFiles.contains(src.GetFileBaseName())) {
            return;
        }

        TFillOpts fOpts;
        fOpts.AddDebugInfo = Opts.EnableDebugInfo;
        fOpts.PatchKeys = !Opts.SkipNorm;
        fOpts.TrieName = src.GetFileBaseName();

        FillKeyTypes(MainBuffer.KeyTypes, src.GetDescr());

        MainBuffer.Cache.Clear();
        SubkeysCache.ClearFakeSubkeys();

        try {
            if (!Opts.SkipNorm) {
                if (!FillSubkeysCache(SubkeysCache, MainBuffer.KeyTypes, Request, src.GetDescr().GetSourceName())) {
                    return;
                }

                if (src.CanCheckPrefix() && SubkeysCache.KeysNeedPrefixPruning(MainBuffer.KeyTypes)) {
                    const TStringBufs& prefixes = SubkeysCache.GetAllPrefixes(MainBuffer.KeyTypes);

                    for (TStringBufs::const_iterator it = prefixes.begin(); it != prefixes.end(); ++it) {
                        if (src.HasPrefix(*it)) {
                            MainBuffer.Cache.GoodPrefixes.push_back(*it);
                        }
                    }

                    SubkeysCache.FillTuplesCache(MainBuffer.Cache, MainBuffer.KeyTypes, true);
                } else {
                    SubkeysCache.FillTuplesCache(MainBuffer.Cache, MainBuffer.KeyTypes, false);
                }
            } else {
                SubkeysCache.MakeFakeTuple(MainBuffer.Cache.Tuples, Request.UserQuery);
            }
        } catch (const yexception& e) {
            SEARCH_ERROR << "Error while preparing request at " <<
                    src.GetFileBaseName() << " (" << src.GetDescr().GetSourceName() << "): " << e.what();
            return;
        }

        TQDTrie::TValue fb;

        const TRawTuples& normQuery = MainBuffer.Cache.Tuples;

        TRawTuples::const_iterator lastFound = normQuery.end();
        for (TRawTuples::const_iterator qqit = normQuery.begin(); qqit != normQuery.end(); ++qqit) {
            if ((lastFound != normQuery.end() && qqit->InferiorTo(*lastFound)) || !qqit->Size()) {
                continue;
            }

            TStringBuf currQuery = qqit->AsStrBuf();

            try {
                if (!src.Find(currQuery, fb, Opts.SkipNorm)) {
                    continue;
                }
            } catch (const yexception& e) {
                SEARCH_ERROR << "Error while searching key " << NEscJ::EscapeJ<true>(currQuery) << " in " <<
                        src.GetFileBaseName() << " (" << src.GetDescr().GetSourceName() << "): " << e.what();
            }

            lastFound = qqit;

            MainBuffer.Subkeys.clear();
            qqit->GetSubkeys(MainBuffer.Subkeys);

            ProcessResult(Result, fb, fb.Get(), MainBuffer, *qqit, src, fOpts);
        }

        if (!src.GetDescr().HasShardNumber() || src.GetDescr().GetShardNumber() == 0) {
            FillCommonFactors(Result, src.GetDescr(), fOpts);
        }
    }

}
