#include "trie_search.h"

#include <search/meta/url.h>

#include <kernel/facts/url_expansion/url_expansion.h>
#include <kernel/querydata/client/qd_document_responses.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/url/url.h>

namespace {

    NFacts::TCredibilityDatabase::ECredibilityMark IntToCredibilityMark(int value) {
        switch (value) {
        case 0: return NFacts::TCredibilityDatabase::ECredibilityMark::NonCredible;
        case 1: return NFacts::TCredibilityDatabase::ECredibilityMark::Credible;
        default: return NFacts::TCredibilityDatabase::ECredibilityMark::Unknown;
        }
    }

    void FetchCredibilityAndKernel(const NSc::TValue::TDict& srcDict, int& credibility, float& kernelScore) {
        const int newCredibility = srcDict.Get("result").GetIntNumber(-1);
        credibility = Max(newCredibility, credibility);

        const float newKernelScore = srcDict.Get("kernel").GetNumber(NFacts::TCredibilityDatabase::DefaultKernelScore);
        if (newKernelScore != NFacts::TCredibilityDatabase::DefaultKernelScore) {
            kernelScore = newKernelScore;
        }
    }

}

namespace NFacts {

    TCredibilityDatabase::TCredibilityDatabase(
        const TString& hostTrieFileName,
        const TString& maskTrieFileName,
        const TString& urlTrieFileName
    )
    : QueryDatabase(new NQueryData::TQueryDatabase())
    {
        Y_ENSURE(!hostTrieFileName.empty() || !maskTrieFileName.empty() || !urlTrieFileName.empty());

        if (!hostTrieFileName.empty()) {
            QueryDatabase->AddSourceFile(hostTrieFileName.c_str());
        }
        if (!maskTrieFileName.empty()) {
            QueryDatabase->AddSourceFile(maskTrieFileName.c_str());
        }
        if (!urlTrieFileName.empty()) {
            QueryDatabase->AddSourceFile(urlTrieFileName.c_str());
        }

        Canonizer.LoadTrueOwners();
    }

    TCredibilityDatabase::TCredibilityDatabase(THolder<NQueryData::TQueryDatabase>&& queryDatabase)
    : QueryDatabase(std::move(queryDatabase))
    {
    }

    TCredibilityDatabase::TCredibilityData TCredibilityDatabase::FindCredibility(const TString& url, const TString& yandexTld) const {
        NQueryData::TQueryData qdBuff;
        NQueryData::TRequestRec qdRequest;
        qdRequest.YandexTLD = yandexTld;
        const TString trueUrl = NFacts::ExpandReferringUrl(url);
        const TString normalizedUrl = NMeta::MakeNormalizedUrlStr(trueUrl);
        const TStringBuf urlDomain = Canonizer.GetUrlOwner(normalizedUrl);
        qdRequest.DocItems.MutableUrls().push_back({normalizedUrl, urlDomain});
        QueryDatabase->GetQueryData(qdRequest, qdBuff);

        int credibility = -1;
        float kernelScore = DefaultKernelScore;
        for (const auto& src : qdBuff.GetSourceFactors()) {
            NSc::TValue jsonValue = NSc::TValue::FromJson(src.GetJson());
            if (jsonValue.IsDict()) {
                FetchCredibilityAndKernel(jsonValue.GetDict(), credibility, kernelScore);
            } else if (jsonValue.IsArray()) {
                for (const auto& jsonItem : jsonValue.GetArray()) {
                    FetchCredibilityAndKernel(jsonItem.GetDict(), credibility, kernelScore);
                }
            }
        }

        return {IntToCredibilityMark(credibility), kernelScore};
    }

}
