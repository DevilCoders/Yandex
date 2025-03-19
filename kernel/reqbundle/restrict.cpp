#include "restrict.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/json/domscheme_traits.h>

#include <util/generic/algorithm.h>

#include <kernel/reqbundle/scheme/options.sc.h>

namespace {
    using namespace NReqBundle;

    using TRestrictSchemeConst = NReqBundleScheme::TRestrictSchemeConst<TJsonTraits>;
    using TRestrictScheme = NReqBundleScheme::TRestrictScheme<TJsonTraits>;

    THolder<TRestrictSchemeConst> ParseValidate(const NJson::TJsonValue& value) {
        THolder<TRestrictSchemeConst> scheme = MakeHolder<TRestrictSchemeConst>(&value);
        scheme->Validate("", true, [&value](const TString& /*path*/, const TString& err){
            ythrow yexception()
                << "failed to parse JSON value '" << value << "'"
                << "\n" << err;
        });

        return scheme;
    }

    void ParseRestrict(
        const TRestrictSchemeConst& scheme,
        TRestrictOptions& opts)
    {
        if (scheme.HasMaxRequests()) {
            Y_ENSURE(
                !scheme.HasMaxRequestSchemas(),
                "both \"MaxRequests\" and \"MaxRequestSchemas\" options are present"
            );
            opts.MaxRequests = scheme.MaxRequests();
        } else if (scheme.HasMaxRequestSchemas()) {
            opts.MaxRequests = scheme.MaxRequestSchemas();
        }

        if (scheme.HasMaxBlocks()) {
            opts.MaxBlocks = scheme.MaxBlocks();
        }

        if (scheme.HasEnabled()) {
            opts.Enabled = scheme.Enabled();
        }
    }

    void ParseFacetRestrict(
        const TRestrictSchemeConst& scheme,
        TFacetRestrict& opts)
    {
        TFacetId id;
        if (scheme.HasType()) {
            Y_ENSURE(
                !scheme.HasFacet(),
                "both \"Type\" and \"Facet\" options are present"
            );
            TStringBuf typeString = scheme.Type();

            EExpansionType type = TExpansion::ExpansionMax;
            Y_ENSURE(
                TryFromString(typeString, type),
                "unknown expansion: " << typeString
            );
            id = TFacetId(type);
        } else if (scheme.HasFacet()) {
            id = FacetIdFromJson(*(scheme.Facet().GetRawValue()));
        }

        opts.Facet = id;
        ParseRestrict(scheme, opts.Restrict);
    }

    void SaveRestrict(
        const TRestrictOptions& opts,
        TRestrictScheme& scheme)
    {
        if (opts.MaxBlocks.Defined()) {
            scheme.MaxBlocks().Set(opts.MaxBlocks.GetRef());
        }
        if (opts.MaxRequests.Defined()) {
            scheme.MaxRequests().Set(opts.MaxRequests.GetRef());
        }
        if (opts.Enabled.Defined()) {
            scheme.Enabled().Set(opts.Enabled.GetRef());
        }
    }

    void SaveFacetRestrict(
        const TFacetRestrict& opts,
        TRestrictScheme& scheme)
    {
        *(scheme.Facet().GetRawValue()) = FacetIdToJson(opts.Facet);
        SaveRestrict(opts.Restrict, scheme);
    }

    struct TIsSubSet
        : public NStructuredId::NDetail::TIsEqual
    {
        using NStructuredId::NDetail::TIsEqual::IsSubValue;

        inline bool IsSubValue(const TRegionId&, const TRegionId&) {
            return true;
        }
    };

    class TRestrictTracker {
    public:
        TRestrictTracker() = default;

        TRestrictTracker(const TRestrictOptions& opts, size_t initialNumBlocks)
            : Opts(opts)
            , NumBlocks(initialNumBlocks)
        {}

        void MergeOverridenOptions(const TRestrictOptions& other) {
            Opts = TRestrictOptions(other).MergeFrom(Opts);
        }

        bool Check(
            TConstRequestAcc request,
            const NReqBundle::NDetail::TBlocksCounter& usedBlocks)
        {
            if (Opts.Enabled.Defined() && false == Opts.Enabled.GetRef()) {
                return false;
            }

            if (Opts.MaxRequests.Defined()
                && NumRequests + 1 > Opts.MaxRequests.GetRef())
            {
                return false;
            }

            NewBlocks.clear();

            for (auto match : request.GetMatches()) {
                const size_t blockIndex = match.GetBlockIndex();

                if (0 == usedBlocks[blockIndex]) {
                    NewBlocks.insert(blockIndex);
                }
            }

            if (Opts.MaxBlocks.Defined()
                && NumBlocks + NewBlocks.size() > Opts.MaxBlocks.GetRef())
            {
                return false;
            }
            return true;
        }

        void Update() {
            NumRequests += 1;
            NumBlocks += NewBlocks.size();
            NewBlocks.clear();
        }

    private:
        TRestrictOptions Opts;

        TSet<size_t> NewBlocks;
        size_t NumRequests = 0;
        size_t NumBlocks = 0;
    };

    class TAllRestrictsTracker {
        const TAllRestrictOptions* Opts = nullptr;
        const size_t InitialNumBlocks = 0;
        TMap<TFacetId, TDeque<TRestrictTracker>> TrackersById;

    public:
        TAllRestrictsTracker(const TAllRestrictOptions* opts, size_t initialNumBlocks)
            : Opts(opts)
            , InitialNumBlocks(initialNumBlocks)
        {
        }

        bool CheckAndUpdate(
            const TFacetId& id,
            TConstRequestAcc request,
            const NReqBundle::NDetail::TBlocksCounter& usedBlocks)
        {
            TDeque<TRestrictTracker>* trackersPtr = TrackersById.FindPtr(id);

            if (!trackersPtr) {
                trackersPtr = &CreateTrackers(id);
            }
            Y_ASSERT(!!trackersPtr);

            for (TRestrictTracker& tracker : *trackersPtr) {
                if (!tracker.Check(request, usedBlocks)) {
                    return false;
                }
            }

            for (TRestrictTracker& trackerPtr : *trackersPtr) {
                trackerPtr.Update();
            }

            return true;
        }

    private:
        TDeque<TRestrictTracker>& CreateTrackers(const TFacetId& id) {
            Y_ASSERT(!!Opts);

            TDeque<TFacetRestrict> entries = Opts->FindAllApplicable(id);
            TDeque<TRestrictTracker>& trackers = TrackersById[id];

            if (entries.empty()) {
                return trackers;
            }

            TRestrictOptions optsForId;
            for (size_t i : xrange(entries.size())) {
                const TFacetId& restrictId = entries[i].Facet;
                const TRestrictOptions& restrictOpts = entries[i].Restrict;

                Y_ASSERT(NStructuredId::IsSubId(id, restrictId, TIsSubRegion{}));
                optsForId.MergeFrom(restrictOpts);
            }

            trackers.emplace_back(optsForId, InitialNumBlocks);

            return trackers;
        }
    };
} // namespace

namespace NReqBundle {
    void TRestrictOptions::FromJson(const TExplicitType<NJson::TJsonValue>& value) {
        ParseRestrict(
            *ParseValidate(value),
            *this);
    }

    NJson::TJsonValue TRestrictOptions::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TRestrictScheme scheme(&value);
        SaveRestrict(*this, scheme);
        return value;
    }

    void TFacetRestrict::FromJson(const TExplicitType<NJson::TJsonValue>& value) {
        ParseFacetRestrict(
            *ParseValidate(value),
            *this);
    }

    NJson::TJsonValue TFacetRestrict::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TRestrictScheme scheme(&value);
        SaveFacetRestrict(*this, scheme);
        return value;
    }

    TDeque<TRestrictOptions> TAllRestrictOptions::FindAll(const TFacetId& id) const {
        TDeque<TRestrictOptions> res;

        for (const auto& entry : *this) {
            if (entry.Facet == id) {
                res.push_back(entry.Restrict);
            }
        }

        return res;
    }

    TDeque<TFacetRestrict> TAllRestrictOptions::FindAllApplicable(const TFacetId& id) const {
        TDeque<TFacetRestrict> res;

        for (const auto& entry : *this) {
            if (NStructuredId::IsSubId(id, entry.Facet, TIsSubRegion{})) {
                res.push_back(entry);
            }
        }

        return res;
    }

    TAllRestrictOptions& TAllRestrictOptions::FromJson(const TExplicitType<NJson::TJsonValue>& explicitValue) {
        const NJson::TJsonValue& value = explicitValue.Value();

        clear();

        if (value.IsNull()) {
            return *this;
        }

        if (value.IsMap()) {
            Add().FromJson(value);
            return *this;
        }

        for (const NJson::TJsonValue& optsValue : value.GetArraySafe()) {
            Add().FromJson(optsValue);
        }

        return *this;
    }

    TAllRestrictOptions& TAllRestrictOptions::FromJsonString(TStringBuf text) {
        NSc::TValue value = NSc::TValue::FromJson(text);
        FromJson(value.ToJsonValue());
        return *this;
    }

    TAllRestrictOptions& TAllRestrictOptions::FromJsonStream(IInputStream& input) {
        TString text = input.ReadAll();
        FromJsonString(text);
        return *this;
    }

    NJson::TJsonValue TAllRestrictOptions::ToJson() const {
        static const TRestrictOptions defaultOpts{};

        NJson::TJsonValue value{NJson::JSON_ARRAY};
        for (const auto& entry : Data) {
            value.AppendValue(entry.ToJson());
        }

        return value;
    }

    void ScaleRestrictOptions(TAllRestrictOptions& opts, float scale)
    {
        static const TRestrictOptions defaultOpts{};

        Y_ASSERT(scale >= 0.0f);
        for (auto& entry : opts) {
            auto& typeOpts = entry.Restrict;
            if (typeOpts.MaxBlocks.Defined()) {
                typeOpts.MaxBlocks = static_cast<size_t>(Max<float>(0.0f, ceil(scale * typeOpts.MaxBlocks.GetRef())));
            }
            if (typeOpts.MaxRequests.Defined()) {
                typeOpts.MaxRequests = static_cast<size_t>(Max<float>(0.0f, ceil(scale * typeOpts.MaxRequests.GetRef())));
            }
        }
    }

    TReqBundleSubset::TReqBundleSubset(TConstReqBundleAcc bundle)
        : Bundle(bundle)
        , Blocks(bundle.GetSequence().GetNumElems())
    {
        for (TConstConstraintAcc constraint : bundle.GetConstraints()) {
            Blocks.Add(constraint);
        }
    }

    bool TReqBundleSubset::HasRequest(size_t index, const TFacetId& id) const
    {
        Y_ASSERT(IsFullyDefinedFacetId(id));
        return UsedRequests.contains(std::make_pair(index, id));
    }

    bool TReqBundleSubset::HasRequest(size_t index) const
    {
        auto it = UsedRequests.lower_bound({ index, TFacetId{} });
        return it != UsedRequests.end() && it->first == index;
    }

    bool TReqBundleSubset::HasBlock(size_t index) const
    {
        return (index < Blocks.size() && Blocks[index] > 0);
    }

    void TReqBundleSubset::KillBlock(size_t index)
    {
        if (Y_UNLIKELY(index >= Bundle.GetSequence().GetNumElems())) {
            Y_ASSERT(false);
            return;
        }
        if (KilledBlocks.empty()) {
            KilledBlocks.assign(Bundle.GetSequence().GetNumElems(), false);
        }
        KilledBlocks[index] = true;
    }

    void TReqBundleSubset::KillConstraint(size_t index)
    {
        if (Y_UNLIKELY(index >= Bundle.GetNumConstraints())) {
            Y_ASSERT(false);
            return;
        }
        if (KilledConstraints.empty()) {
            KilledConstraints.assign(Bundle.GetNumConstraints(), false);
        }
        KilledConstraints[index] = true;
    }

    void TReqBundleSubset::AddRequest(size_t index, const TFacetId& id)
    {
        Y_ASSERT(IsFullyDefinedFacetId(id));
        Y_ASSERT(index < Bundle.GetNumRequests());
        float value = 0.0;
        Bundle.GetRequest(index).GetFacets().Lookup(id, value);
        if (Requests.emplace(index, id, value).second) {
            Blocks.Add(Bundle.GetRequest(index));
        }
        UsedRequests.emplace(index, id);
    }

    void TReqBundleSubset::AddRequest(size_t index)
    {
        Y_ASSERT(index < Bundle.GetNumRequests());
        if (Requests.emplace(index).second) {
            Blocks.Add(Bundle.GetRequest(index));
        }
        UsedRequests.emplace(index, TFacetId{});
    }

    void TReqBundleSubset::ClearRequestTrInfo(size_t index)
    {
        Y_ASSERT(index < Bundle.GetNumRequests());
        if (ClearedRequestsTrInfo.empty()) {
            ClearedRequestsTrInfo.resize(Bundle.GetNumRequests(), false);
        }
        ClearedRequestsTrInfo[index] = true;
    }

    TReqBundlePtr TReqBundleSubset::GetResult(
        NDetail::TBlocksRemapper& blocksRemap,
        NDetail::TRequestsRemapper* requestsRemap) const
    {
        THolder<TReqBundle> newBundlePtr = MakeHolder<TReqBundle>();
        auto seq = Bundle.GetSequence();
        auto newSeq = newBundlePtr->Sequence();

        blocksRemap.Reset(Blocks.size());
        if (!!requestsRemap) {
            requestsRemap->Reset(Bundle.GetNumRequests());
       }

        for (size_t elemIndex : xrange(Blocks.size())) {
            if (HasBlock(elemIndex)
                && (KilledBlocks.empty() || !KilledBlocks[elemIndex]))
            {
                auto elem = seq.GetElem(elemIndex);
                blocksRemap[elemIndex] = newSeq.GetNumElems();
                NDetail::BackdoorAccess(newSeq).Elems.push_back(NDetail::BackdoorAccess(elem));
            }
        }

        for (auto iter = Requests.begin(); iter != Requests.end();) {
            size_t requestIndex = iter->Index;
            if (requestIndex >= Bundle.GetNumRequests()) {
                break;
            }

            auto request = Bundle.GetRequest(requestIndex);
            if (!blocksRemap.CanRemap(request)) {
                while (iter != Requests.end() && iter->Index == requestIndex) {
                    ++iter;
                }
                continue;
            }

            THolder<TRequest> newRequestPtr = MakeHolder<TRequest>(request);
            newRequestPtr->Facets().Clear();
            blocksRemap(*newRequestPtr);

            auto facets = request.GetFacets();

            if (TFacetId() == iter->Id) { // skip placeholder
                ++iter;
            }

            for (; iter != Requests.end() && iter->Index == requestIndex; ++iter) {
                float value = 0.0f;
                if (facets.Lookup(iter->Id, value)) {
                    newRequestPtr->Facets().Set(iter->Id, value);
                }
            }

            if (!!requestsRemap) {
                (*requestsRemap)[requestIndex] = newBundlePtr->GetNumRequests();
            }

            bool needTrCompatibilityInfo = AnyOf(newRequestPtr->GetFacets().GetEntries(), [](TConstFacetEntryAcc entry) {
                return entry.NeedTrCompatibilityInfo();
            });
            if (!needTrCompatibilityInfo || (!ClearedRequestsTrInfo.empty() && ClearedRequestsTrInfo[requestIndex])) {
                newRequestPtr->ClearTrCompatibilityInfo();
            }

            newBundlePtr->AddRequest(newRequestPtr.Release());
        }

        for (size_t i = 0; i < Bundle.GetNumConstraints(); i++) {
            TConstConstraintAcc constraint = Bundle.GetConstraint(i);
            if (i < KilledConstraints.size() && KilledConstraints[i]) {
                continue;
            }
            if (blocksRemap.CanRemap(constraint)) {
                THolder<TConstraint> newConstraintPtr = MakeHolder<TConstraint>(constraint);
                blocksRemap(*newConstraintPtr);
                newBundlePtr->AddConstraint(newConstraintPtr.Release());
            }
        }

        return newBundlePtr.Release();
    }

    TReqBundlePtr TReqBundleSubset::GetResult() const
    {
        NDetail::TBlocksRemapper remap;
        return GetResult(remap, nullptr);
    }

    TReqBundlePtr TReqBundleSubset::GetResult(NDetail::TRequestsRemapper& requestsRemap) const
    {
        NDetail::TBlocksRemapper blocksRemap;
        return GetResult(blocksRemap, &requestsRemap);
    }

    TReqBundleSubsetPtr RestrictReqBundle(TConstReqBundleAcc bundle, const TAllRestrictOptions& opts)
    {
        NDetail::TBlocksCounter usedBlocks;

        usedBlocks.resize(bundle.GetSequence().GetNumElems(), false);

        for (auto constraint : bundle.GetConstraints()) {
            usedBlocks.Add(constraint);
        }

        size_t initialNumBlocks = 0;
        for (size_t i = 0; i < usedBlocks.size(); ++i) {
            if (usedBlocks[i]) {
                ++initialNumBlocks;
            }
        }

        TAllRestrictsTracker tracker(&opts, initialNumBlocks);

        THolder<TReqBundleSubset> subBundlePtr = MakeHolder<TReqBundleSubset>(bundle);

        TVector<NDetail::TRequestWithFacet> requests;
        requests.reserve(bundle.GetNumRequests());

        for (size_t requestIndex : xrange(bundle.GetNumRequests())) {
            for (auto entry : bundle.GetRequest(requestIndex).GetFacets().GetEntries()) {
                 requests.emplace_back(requestIndex, entry.GetId(), entry.GetValue());
            }
        }
        Sort(requests.begin(), requests.end());

        bool dropOriginalRequest = false;
        for (const auto& opt : opts) {
            if (opt.Facet.IsValid<EFacetPartType::Expansion>() && opt.Facet.Get<EFacetPartType::Expansion>() == EExpansionType::OriginalRequest) {
                Y_ENSURE(opt.Restrict.Enabled.Defined());
                Y_ENSURE(!opt.Restrict.MaxBlocks.Defined());
                Y_ENSURE(!opt.Restrict.MaxRequests.Defined());
                dropOriginalRequest = !*opt.Restrict.Enabled;
            }
        }
        for (const auto& requestWithFacet : requests) {
            auto request = bundle.GetRequest(requestWithFacet.Index);
            auto id = requestWithFacet.Id;

            if (id.Get<EFacetPartType::Expansion>() == TExpansion::OriginalRequest) {
                if (!dropOriginalRequest) {
                    subBundlePtr->AddRequest(requestWithFacet.Index, id);
                    usedBlocks.Add(request);
                }
                continue;
            }

            if (tracker.CheckAndUpdate(id, request, usedBlocks)) {
                subBundlePtr->AddRequest(requestWithFacet.Index, requestWithFacet.Id);
                usedBlocks.Add(request);
            }
        }

        return subBundlePtr.Release();
    }

    TReqBundleSubsetPtr RestrictReqBundleByExpansions(TConstReqBundleAcc bundle,
        const std::initializer_list<EExpansionType> expansionsToSave)
    {
        TAllRestrictOptions restricts;

        for (auto exp : NLingBoost::TExpansion::GetValues()) {
            restricts.Add(TFacetId{exp}).SetEnabled(false);
        }
        for (auto exp : expansionsToSave) {
            restricts.Add(TFacetId{exp}).SetEnabled(true);
        }

        return RestrictReqBundle(bundle, restricts);
    }
} // NReqBundle
