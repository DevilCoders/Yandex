#include "util.h"

#include "reqbundle.h"
#include "serializer.h"
#include "request_splitter.h"

#include <kernel/reqbundle/proto/reqbundle.pb.h>
#include <kernel/reqbundle/merge.h>
#include <kernel/reqbundle/rearrange_helpers.h>
#include <kernel/reqbundle/request_pure.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/compressor/factory.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/scan/scan.h>

using namespace NReqBundle;

namespace {
    NReqBundle::TRequestSplitter::TUnpackOptions MakeSplitterOpts(const NReqBundle::TReqBundleFromRichTreeOpts& opts) {
        NReqBundle::TRequestSplitter::TUnpackOptions options;
        options.FilterOffBadAttributes = opts.FilterOffBadAttribute;

        if (!opts.TrHitsCompatibilityMode)
            return options;
        options.UnpackConstraints = true;
        options.UnpackAttributes = true;
        options.UnpackAndBlocks = true;
        return options;
    }

    THashMap<TStringBuf, TStringBuf> ParseRelevCgi(const TCgiParameters& params) {
        THashMap<TStringBuf, TStringBuf> relevParsed;
        for (size_t i = 0; TStringBuf relev = params.Get("relev", i); ++i) {
            {
                ScanKeyValue<true, ';', '='>(relev, [&relevParsed] (TStringBuf key, TStringBuf val) {
                    relevParsed[key] = val;
                });
            }
        }
        return relevParsed;
    }

    static const THashMap<TStringBuf, EMergeScope> stringToMergeScope {
        {"ByRequest", EMergeScope::ByRequest},
        {"ByWord", EMergeScope::ByWord}
    };

    static const THashMap<TStringBuf, EMergeCondition> stringToMergeCondition {
        {"Always", EMergeCondition::Always},
        {"IfEmpty", EMergeCondition::IfEmpty}
    };
}

TReqBundlePtr NReqBundle::ReqBundleFromRichTree(const TRichTreeConstPtr& richTree, const TReqBundleFromRichTreeOpts& opts) {
    THolder<TSequence> sequencePtr = MakeHolder<TSequence>();
    TRequestSplitter splitter(*sequencePtr, MakeSplitterOpts(opts));

    TRequestPtr requestPtr = opts.TrHitsCompatibilityMode ? splitter.SplitRequestForTrIterator(*richTree->Root) : splitter.SplitRequest(*richTree->Root);
    if (!requestPtr) {
        return nullptr;
    }
    requestPtr->Facets().Set(TFacetId(TExpansion::OriginalRequest, TRegionId::World()), 1.0f);
    THolder<TReqBundle> bundlePtr = MakeHolder<TReqBundle>(sequencePtr.Release());
    bundlePtr->AddRequest(requestPtr);
    return bundlePtr.Release();
}

TString NReqBundle::Base64QBundleFromRichTree(const TRichTreeConstPtr& richTree) {
    TReqBundlePtr bundlePtr = ReqBundleFromRichTree(richTree);
    Y_ENSURE(bundlePtr, "failed to make ReqBundleFromRichTree");
    TString binary;
    TStringOutput binaryOutput(binary);
    TReqBundleSerializer::TOptions opts;
    opts.BinMode = TReqBundleSerializer::EnforceBinary;
    TReqBundleSerializer ser;
    ser.Serialize(*bundlePtr, &binaryOutput);
    TString base64;
    Base64EncodeUrl(binary, base64);
    return base64;
}

bool NReqBundle::AddLboostExpansion(TReqBundle& qbundle,
                        EExpansionType expansionType,
                        const TLangMask& langs,
                        const TString& expansionText,
                        float expansionWeight)
{
    TRequestSplitter reqSplitter(qbundle.Sequence());
    TRequestPtr request = reqSplitter.SplitRequest(expansionText, langs, {{MakeFacetId(expansionType), expansionWeight}});
    if (request) {
        if (Y_LIKELY(NDetail::IsValidRequest(*request, qbundle.GetSequence()))) {
            qbundle.AddRequest(request);
            return true;
        } else {
            // TODO(@gotmanov)
            // The following assert is not satisfied.
            // Currently it is possible to construct an invalid request;
            // example - request with >1000 words.
            // Invalid request in reqbundle doesn't break search,
            // because basesearch will validate it before using.
            //
            // Y_ASSERT(false);
        }
    }
    return false;
}

namespace {
    TMaybe<NReqBundle::TRequestAccBase<const NReqBundle::NDetail::TRequestData>> GetOriginalRequest(const TReqBundle& qbundle) {
        for (const auto& req : qbundle.GetRequests()) {
            if (req.HasExpansionType(NLingBoost::TExpansion::OriginalRequest)) {
                return req;
            }
        }
        return {};
    }
}

TVector<float> NReqBundle::ExtractQueryWordWeights(const TReqBundle& qbundle) {
    TVector<float> result;
    if (auto req = GetOriginalRequest(qbundle)) {
        for (const auto& word : req->GetWords()) {
            result.emplace_back(word.GetIdf());
        }
    }
    return result;
}

TMaybe<ui32> NReqBundle::GetTrCompatibleWordCount(const TReqBundle& qbundle) {
    if (auto req = GetOriginalRequest(qbundle)) {
        if (!req->GetTrCompatibilityInfo().Defined()) {
                return {};
        }
        return req->GetTrCompatibilityInfo()->WordCount;
    }
    return {};
}

ui32 NReqBundle::GetOriginalQueryWordCount(const TReqBundle& qbundle) {
    if (auto req = GetOriginalRequest(qbundle)) {
        return req->GetWords().size();
    }
    return 0;
}

ui64 NReqBundle::GetStopWordsMask(const TReqBundle& qbundle) {
    ui64 mask = 0;
    TSet<TString> stopWords;
    if (auto req = GetOriginalRequest(qbundle)) {
        for (const NReqBundle::TConstMatchAcc& match : req->GetMatches()) {
            Y_ASSERT(qbundle.GetSequence().HasBlock(match.GetBlockIndex()));
            Y_ENSURE(qbundle.GetSequence().HasBlock(match.GetBlockIndex()),
                        "We assume PrepareAllBlocks call before this function invocation");
            for (const NReqBundle::TConstWordAcc& word : qbundle.GetSequence().
                        GetBlock(match.GetBlockIndex()).GetWords()) {
                if (word.IsStopWord()) {
                    stopWords.insert(word.GetText());
                }
            }
        }

        ui32 idx = 0;
        for (const auto& word : req->GetWords()) {
            mask += static_cast<ui64>(stopWords.contains(word.GetTokenText())) << idx;
            ++idx;
        }
    }
    return mask;
}

ui32 NReqBundle::GetNonStopWordsCount(const TReqBundle& qbundle) {
    ui32 numNonStopWords = 0;
    for (const NReqBundle::TRequest& req : qbundle.GetRequests()) {
        for (const NReqBundle::TConstMatchAcc& match : req.GetMatches()) {
            bool isMatchNonStop = true;
            Y_ASSERT(qbundle.GetSequence().HasBlock(match.GetBlockIndex()));
            Y_ENSURE(qbundle.GetSequence().HasBlock(match.GetBlockIndex()),
                        "We assume PrepareAllBlocks call before this function invocation");
            for (const NReqBundle::TConstWordAcc& word : qbundle.GetSequence().
                        GetBlock(match.GetBlockIndex()).GetWords()) {
                if (word.IsStopWord()) {
                    isMatchNonStop = false;
                }
            }
            if (isMatchNonStop) {
                ++numNonStopWords;
            }
        }
    }
    return numNonStopWords;
}

ui64 NReqBundle::GetNonStopWordsMask(const TReqBundle& qbundle) {
    if (auto req = GetOriginalRequest(qbundle)) {
        ui64 stopWordsMask = GetStopWordsMask(qbundle);
        auto wordCount = req->GetWords().size();
        Y_ASSERT(wordCount <= 64);
        return ((static_cast<ui64>(1) << wordCount) - 1) ^ stopWordsMask;
    }

    return 0;
}

TReqBundlePtr NReqBundle::Base64UnpackBundle(TStringBuf packedBundle) {
    TReqBundleDeserializer::TOptions deserOpts;
    deserOpts.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
    TReqBundleDeserializer deser(deserOpts);
    NReqBundle::TReqBundlePtr bundle = new NReqBundle::TReqBundle;
    TReqBundleDeserializer::TOptions dsOpts;
    dsOpts.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
    TReqBundleDeserializer rbDeser(dsOpts);
    rbDeser.Deserialize(Base64StrictDecode(packedBundle), *bundle);
    bundle->Sequence().PrepareAllBlocks(deser);
    NReqBundle::FillRevFreqs(*bundle);
    return bundle;
}

TVector<TString> NReqBundle::FetchPackedBundlesFromCgi(TStringBuf cgi) {
    TCgiParameters paramsInput(cgi);
    TVector<TString> parts;
    if (paramsInput.Get("qbundles")) {
        Split(paramsInput.Get("qbundles"), ";", parts);
    }
    if (paramsInput.Get("qbundle")) {
        parts.push_back(paramsInput.Get("qbundle"));
    }
    {
        THashMap<TStringBuf, TStringBuf> relevParsed = ParseRelevCgi(paramsInput);
        if (relevParsed.contains("lbqbundle")) {
            parts.push_back(TString{relevParsed.at("lbqbundle")});
        }
        if (relevParsed.contains("wizqbundle")) {
            parts.push_back(TString{relevParsed.at("wizqbundle")});
        }
    }
    return parts;
}

TVector<TReqBundlePtr> NReqBundle::FetchBundlesFromCgi(TStringBuf cgi) {
    TVector<TString> parts = FetchPackedBundlesFromCgi(cgi);

    TVector<TReqBundlePtr> res;
    for (const auto& part : parts) {
        res.push_back(Base64UnpackBundle(part));
    }
    return res;
}

TReqBundlePtr NReqBundle::MergeBase64Bundles(const TVector<TString>& packedBundles) {
    THolder<NReqBundle::TReqBundleMerger> merger = MakeHolder<NReqBundle::TReqBundleMerger>();
    for (const auto& bundleStr : packedBundles) {
        NReqBundle::TReqBundlePtr bundle = nullptr;
        try {
            bundle = Base64UnpackBundle(bundleStr);
        } catch (yexception& e) {
            ythrow yexception{} << "failed to deserialize reqbundle[" << bundleStr << "]"
                << "\n<error message> " << e.what();
        }
        merger->AddBundle(*bundle);
    }
    return merger->GetResult();
}


TString NReqBundle::PackAsBinary(NReqBundle::TConstReqBundleAcc bundle) {
    TString binary;
    {
        TStringOutput binaryOutput(binary);
        TReqBundleSerializer ser;
        ser.Serialize(bundle, &binaryOutput);
    }
    return binary;
}
TString NReqBundle::PackAsText(NReqBundle::TConstReqBundleAcc bundle) {
    return Base64EncodeUrl(PackAsBinary(bundle));
}

void NReqBundle::TOriginalReqBundleFetchingOptions::Parse(TStringBuf jsonOpts) {
    NJson::TJsonValue val = NJson::ReadJsonFastTree(jsonOpts);
    if (val.Has("EnableAttributesInReqBundle")) {
        EnableAttributesInReqBundle = val["EnableAttributesInReqBundle"].GetBoolean();
    }
    if (val.Has("UseThesBundle")) {
        UseThesBundle = val["UseThesBundle"].GetBoolean();
    }
    if (val.Has("MergeOptions.AlignTokens")) {
        MergeOptions.AlignTokens = val["MergeOption.AlignTokens"].GetBoolean();
    }
    if (val.Has("MergeOptions")) {
        const auto& mergeOptionsJson = val["MergeOptions"].GetMap();
        if (mergeOptionsJson.contains("Scope")) {
            auto &scope = mergeOptionsJson.at("Scope").GetString();
            auto it = stringToMergeScope.find(scope);
            if (it != stringToMergeScope.cend()) {
                MergeOptions.Scope = it->second;
            }
        }
        if (mergeOptionsJson.contains("Condition")) {
            auto &condition = mergeOptionsJson.at("Condition").GetString();
            auto it = stringToMergeCondition.find(condition);
            if (it != stringToMergeCondition.cend()) {
                MergeOptions.Condition = it->second;
            }
        }
    }
}

TReqBundlePtr NReqBundle::FetchAndMergeOriginalReqBundleFromCgi(TStringBuf cgi, TOriginalReqBundleFetchingOptions options) {
    TCgiParameters paramsInput(cgi);
    THashMap<TStringBuf, TStringBuf> relevParsed = ParseRelevCgi(paramsInput);

    NReqBundle::TReqBundlePtr wizardBundle;
    if (relevParsed.contains("wizqbundle")) {
        TReqBundleSearchParser::TOptions opts {
            .FilterOffBadAttributes = options.EnableAttributesInReqBundle
        };
        TReqBundleSearchParser parser{opts};
        parser.AddBase64(relevParsed.at("wizqbundle"));
        wizardBundle = parser.GetPreparedForSearch();
    } else if (auto& qtree = paramsInput.Get("qtree")) {
        TRichTreePtr richTree = DeserializeRichTree(DecodeRichTreeBase64(qtree));

        NReqBundle::TReqBundleFromRichTreeOpts opts {
            .TrHitsCompatibilityMode = true,
            .FilterOffBadAttribute = options.EnableAttributesInReqBundle
        };
        wizardBundle = ReqBundleFromRichTree(richTree, opts);
    }

    NReqBundle::TReqBundlePtr mergedBundle;
    if (wizardBundle && relevParsed.contains("thesqbundle") && options.UseThesBundle) {

        TString errValString;
        TReqBundlePtr thesBundle = NDetail::PrepareThesaurusReqBundle(TString{relevParsed.at("thesqbundle")}, NDetail::GetParamsForThesaurusReqBundle(), errValString, false);
        if (!thesBundle) {
            return wizardBundle;
        }

        TMergeSynonymsOptions mergeOptions = options.MergeOptions;
        mergedBundle = MergeSynonymsToReqBundle(*wizardBundle, *thesBundle, mergeOptions);
        if (mergedBundle) {
            NReqBundle::NSer::TDeserializer deser;
            mergedBundle->Sequence().PrepareAllBlocks(deser);
        }
    } else {
        mergedBundle = wizardBundle;
    }
    return mergedBundle;
}
