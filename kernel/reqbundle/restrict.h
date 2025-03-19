#pragma once

#include "blocks_counter.h"
#include "blocks_remapper.h"
#include "reqbundle.h"

#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/explicit_type.h>

#include <array>

namespace NReqBundle {
    class TRestrictOptions {
    public:
        TMaybe<bool> Enabled;
        TMaybe<size_t> MaxBlocks;
        TMaybe<size_t> MaxRequests;

        bool operator == (const TRestrictOptions& other) const {
            return Enabled == other.Enabled &&
                MaxBlocks == other.MaxBlocks &&
                MaxRequests == other.MaxRequests;
        }

        TRestrictOptions() {}
        TRestrictOptions(size_t maxBlocks, size_t maxRequests)
            : MaxBlocks(maxBlocks), MaxRequests(maxRequests)
        {}

        TRestrictOptions& SetEnabled(bool f = true) {
            Enabled = f;
            return *this;
        }
        TRestrictOptions& SetMaxBlocks(size_t maxBlocks) {
            MaxBlocks = maxBlocks;
            return *this;
        }
        TRestrictOptions& SetMaxRequests(size_t maxRequests) {
            MaxRequests = maxRequests;
            return *this;
        }
        TRestrictOptions& MergeFrom(const TRestrictOptions& other, bool commonFieldsOnly = false) {
            if ((!commonFieldsOnly || Enabled.Defined()) && other.Enabled.Defined()) {
                Enabled = other.Enabled;
            }
            if ((!commonFieldsOnly || MaxBlocks.Defined()) && other.MaxBlocks.Defined()) {
                MaxBlocks = other.MaxBlocks;
            }
            if ((!commonFieldsOnly || MaxRequests.Defined()) && other.MaxRequests.Defined()) {
                MaxRequests = other.MaxRequests;
            }
            return *this;
        }

        void FromJson(const TExplicitType<NJson::TJsonValue>& value);
        NJson::TJsonValue ToJson() const;

        int operator & (IBinSaver& f) {
            f.Add(2, &Enabled);
            f.Add(2, &MaxBlocks);
            f.Add(2, &MaxRequests);
            return 0;
        }
    };

    class TFacetRestrict {
    public:
        TFacetId Facet;
        TRestrictOptions Restrict;

        TFacetRestrict() = default;
        TFacetRestrict(const TFacetId& facet, const TRestrictOptions& restrict)
            : Facet(facet), Restrict(restrict)
        {}

        bool operator == (const TFacetRestrict& other) const {
            return Facet == other.Facet && Restrict == other.Restrict;
        }

        void FromJson(const TExplicitType<NJson::TJsonValue>& value);
        NJson::TJsonValue ToJson() const;

        int operator & (IBinSaver& f) {
            f.Add(2, &Facet);
            f.Add(2, &Restrict);
            return 0;
        }
    };

    class TAllRestrictOptions {
    public:
        using TData = TList<TFacetRestrict>;
        using TConstIterator = typename TData::const_iterator;
        using TIterator = typename TData::iterator;

    public:
        TAllRestrictOptions() = default;

        TAllRestrictOptions(std::initializer_list<TFacetRestrict> items) {
            Add(items);
        }

        template <typename... Args>
        explicit TAllRestrictOptions(Args&&... args) {
            Add(std::forward<Args>(args)...);
        }

        template <typename IterType>
        void Add(IterType begin, IterType end) {
            Data.insert(Data.end(), begin, end);
        }

        template <typename ContType>
        void Add(const ContType& items) {
            Add(items.begin(), items.end());
        }

        void Add(std::initializer_list<TFacetRestrict> items) {
            Add(items.begin(), items.end());
        }

        void Add(const TFacetId& id, const TRestrictOptions& opts) {
            Data.emplace_back(id, opts);
        }

        TRestrictOptions& Add(const TFacetId& id) {
            Data.emplace_back(id, TRestrictOptions{});
            return Data.back().Restrict;
        }

        TFacetRestrict& Add() {
            Data.emplace_back();
            return Data.back();
        }

        TDeque<TRestrictOptions> FindAll(const TFacetId& id) const;

        TDeque<TFacetRestrict> FindAllApplicable(const TFacetId& id) const;

        TConstIterator begin() const {
            return Data.begin();
        }

        TIterator begin() {
            return Data.begin();
        }

        TConstIterator end() const {
            return Data.end();
        }

        TIterator end() {
            return Data.end();
        }

        size_t size() const {
            return Data.size();
        }

        bool empty() const {
            return Data.empty();
        }

        void clear() {
            Data.clear();
        }

        bool operator == (const TAllRestrictOptions& other) const {
            return Data == other.Data;
        }

        TAllRestrictOptions& FromJson(const TExplicitType<NJson::TJsonValue>& value);
        TAllRestrictOptions& FromJsonStream(IInputStream& input);
        TAllRestrictOptions& FromJsonString(TStringBuf text);

        NJson::TJsonValue ToJson() const;

        int operator & (IBinSaver& f) {
            f.Add(2, &Data);
            return 0;
        }

    private:
        TData Data;
    };

    void ScaleRestrictOptions(TAllRestrictOptions& opts, float scale);

    namespace NDetail {
        struct TRequestWithFacet {
            size_t Index = 0;
            TFacetId Id;
            float Value = 0.0;

            TRequestWithFacet() = default;
            TRequestWithFacet(size_t index)
                : Index(index)
            {}
            TRequestWithFacet(size_t index, const TFacetId& id, float value = 0.0)
                : Index(index)
                , Id(id)
                , Value(value)
            {}

            auto KeyToCompare() const {
                return std::make_tuple(Id, -Value, Index);
            }

            bool operator < (const TRequestWithFacet& other) const { // used in TReqBundleSubset
                return KeyToCompare() < other.KeyToCompare();
            }
        };
    } // NDetail

    class TReqBundleSubset
        : public TAtomicRefCount<TReqBundleSubset>
    {
    public:
        TReqBundleSubset(TConstReqBundleAcc bundle);

        void AddRequest(size_t index, const TFacetId& id);
        bool HasRequest(size_t index, const TFacetId& id) const;

        void AddRequest(size_t index); // adds request, no facets
        bool HasRequest(size_t index) const; // even if it has no facets

        void KillBlock(size_t index); // never use this block
        bool HasBlock(size_t index) const;

        void KillConstraint(size_t index);

        void ClearRequestTrInfo(size_t index);

        TReqBundlePtr GetResult() const;
        TReqBundlePtr GetResult(NDetail::TRequestsRemapper& remap) const;

        TReqBundlePtr GetResult(
            NDetail::TBlocksRemapper& blocksRemap,
            NDetail::TRequestsRemapper* requestsRemap = nullptr) const;

    private:
        TConstReqBundleAcc Bundle;

        TSet<NDetail::TRequestWithFacet> Requests;
        NDetail::TBlocksCounter Blocks;
        TVector<bool> KilledBlocks;
        TVector<bool> ClearedRequestsTrInfo;
        TVector<bool> KilledConstraints;
        TSet<std::pair<size_t, TFacetId>> UsedRequests;
    };

    using TReqBundleSubsetPtr = TIntrusivePtr<TReqBundleSubset>;

    // Each restriction in options is viewed as a "pattern" that applies
    // to one or more facet ids. Note that different facet ids are
    // currently handled **idependently**, e.g.:
    //      {Facet:{Expansion:XfDtShow}, MaxRequests:10}
    // is interprteted as follows:
    //      for each XfDtShow facet, allow no more than 10 requests
    //      (however total for all XfDtShow facets may exceed 10 requests)
    // For example, a bundle with
    //      7 expansions XfDtShow_World, and
    //      7 expansions XfDtShow_Country(ru)
    // satisfies the above contraint, although total count of XfDtShow expansions is 14.
    //
    // It is possible to have several conflicting constraints in restrict options.
    // For given expansion all applicable constraints are traversed and merged
    // in order of their appearance.
    // For example:
    //      {Facet:{RegionId:Country}, Enabled: False}
    //      {Facet:{RegionId:Country(tr)}, Enabled: True}
    // Second constraint will override
    // the first for expansions with Country(tr). Effectively, having
    // two contraints like that in the beginning of a list disables
    // all Country expansions except for Turkey.
    //
    TReqBundleSubsetPtr RestrictReqBundle(TConstReqBundleAcc bundle, const TAllRestrictOptions& options);
    TReqBundleSubsetPtr RestrictReqBundleByExpansions(TConstReqBundleAcc bundle,
        const std::initializer_list<EExpansionType> expansionsToSave);
} // NReqBundle
