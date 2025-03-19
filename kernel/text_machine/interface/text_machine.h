#pragma once

#include "query.h"
#include "hit.h"
#include "feature.h"
#include "data_extractors.h"

#include <library/cpp/binsaver/bin_saver.h>

#include <util/memory/pool.h>
#include <util/generic/map.h>
#include <util/generic/deque.h>
#include <util/generic/vector.h>

namespace NJson {
    class TJsonValue;
}

namespace NTextMachine {
    class TMachineOptions {
    public:
        bool EnableCompactEnums = true;
    };

    class TOneExpansionOptions {
    public:
        bool Enabled = false;
    };

    class TExpansionOptions
        : public TMap<EExpansionType, TOneExpansionOptions>
    {
    public:
        using TBase = TMap<EExpansionType, TOneExpansionOptions>;

        bool IsEnabled(EExpansionType expansionType) const {
            auto iter = find(expansionType);
            return iter != end() && iter->second.Enabled;
        }

        int operator & (IBinSaver& f) {
            return f.Add(2, static_cast<TBase*>(this));
        }
    };

    class TFeatureOptions {
    private:
        using TIds = TDeque<const TFFIdWithHash*>;
        using TStorage = TDeque<TFFIdWithHash>;

    public:
        using TConstIterator = TIds::const_iterator;

    public:
        void Add(const TFFIdWithHash* id) {
            Ids.push_back(id);
        }
        void Add(const TFFIdWithHash& id) {
            Storage.push_back(id);
            Ids.push_back(&Storage.back());
        }
        void Clear() {
            Storage.clear();
            Ids.clear();
        }

        size_t Size() const {
            return Ids.size();
        }

        TConstIterator begin() const {
            return Ids.begin();
        }
        TConstIterator end() const {
            return Ids.end();
        }

    private:
        TIds Ids;
        TStorage Storage;
    };

    class IHitsConsumer {
    public:
        // Accepted hits config
        //
        virtual const TVector<EExpansionType>& GetAllowedExpansions() const = 0;
        virtual const TVector<EStreamType>& GetAllowedStreams() const = 0;

        template <typename ContType>
        void SaveAllowedExpansions(ContType& cont) const {
            cont.insert(GetAllowedExpansions().begin(),
                GetAllowedExpansions().end());
        }

        template <typename ContType>
        void SaveAllowedStreams(ContType& cont) const {
            cont.insert(GetAllowedStreams().begin(),
                GetAllowedStreams().end());
        }

        // Add hits
        //
        virtual void AddMultiHits(const TMultiHit* buf, size_t count) = 0;
    };

    class IInfoCollector {
    public:
        virtual ~IInfoCollector() = default;

        virtual void AddQueryFeatures(
            EExpansionType expansionType,
            size_t index,
            const TFFIds& ids,
            TArrayRef<const float> features) = 0;
    };

    class ITextMachine
        : public IHitsConsumer
    {
    public:
        virtual ~ITextMachine() = default;

        // Configuration methods
        //
        const TVector<EExpansionType>& GetAllowedExpansions() const override {
            return TExpansion::GetValuesVector(); // TODO: Remove. Here to avoid diffs for now.
        }
        const TVector<EStreamType>& GetAllowedStreams() const override {
            return TStream::GetValuesVector(); // TODO: Remove. Here to avoid diffs for now.
        }

        virtual void ConfigureMachine(const TMachineOptions& options) = 0;
        virtual void ConfigureFeatures(const TFeatureOptions& options) = 0;
        virtual void ConfigureExpansions(const TExpansionOptions& options) = 0;
        virtual void NarrowByLevel(const TStringBuf& levelName) = 0;
        virtual void SetInfoCollector(IInfoCollector* collector) = 0;

        // Feature calculation, must be defined
        // in every implementation
        //
        virtual void NewQueryInPool(const TMultiQuery& multiQuery, TMemoryPool& pool) = 0;
        virtual void NewDoc() = 0;
        // AddMultiHits from IHitsConsumer
        virtual void SaveOptFeatures(TOptFeatures& features) = 0;
        virtual void SaveFeatureIds(TFFIds& ids) const = 0;

        // Helper methods, implemented by
        // default in TTextMachineBase using
        // more general purely virtual analogs
        //
        virtual void NewQuery(const TMultiQuery& multiQuery) = 0;
        virtual void SaveFeatures(TFeatures& features) = 0;
        virtual void CalcFeatures(TFeatures& features) = 0;
        virtual const TFFIds& GetFeatureIds() const = 0;

        // Other methods
        //
        virtual void SaveToJson(NJson::TJsonValue&) const = 0;

        virtual TDataExtractorsList FetchExtractors(EDataExtractorType Type) {
            //TODO: make this method to be pure-virtual
            Y_UNUSED(Type);
            return {};
        }

        template<EDataExtractorType Type>
        void FetchExtractors(TTypedDataExtractorsList<Type>& dst) {
            TDataExtractorsList result = FetchExtractors(Type);
            for(auto ptr: result) {
                Y_ASSERT(ptr);
                Y_ASSERT(ptr->GetType() == Type);
                dst.push_back(static_cast<IDataExtractorImpl<Type>*>(ptr));
            }
        }
    };

    using TTextMachinePtr = THolder<ITextMachine>;
    using TTextMachinePoolPtr = THolder<ITextMachine, TDestructor>;

    class TTextMachineBase : public ITextMachine {
    private:
        static const size_t PoolInitSize = 1UL << 20;

    public:
        void ConfigureMachine(const TMachineOptions& /*options*/) override {
            Y_ASSERT(false); // config method not implemented
        }
        void ConfigureExpansions(const TExpansionOptions& /*options*/) override {
            Y_ASSERT(false); // config method not implemented
        }
        void ConfigureFeatures(const TFeatureOptions& /*options*/) override {
            Y_ASSERT(false); // config method not implemented
        }
        void NarrowByLevel(const TStringBuf& /*levelName*/) override {
            Y_ASSERT(false); // config method not implemented
        }
        void SetInfoCollector(IInfoCollector* collector) override {
            Y_ASSERT(!collector); // not implemented
        }

        void NewQuery(const TMultiQuery& multiQuery) override {
            if (!QueryPool) {
                QueryPool.Reset(new TMemoryPool(PoolInitSize));
            }
            QueryPool->ClearKeepFirstChunk();
            NewQueryInPool(multiQuery, *QueryPool);
        }
        void SaveOptFeatures(TOptFeatures& /*features*/) override {
            Y_ASSERT(false); // optimized features interface is not implemented
        }
        void SaveFeatures(TFeatures& features) override {
            TOptFeatures optFeatures;
            SaveOptFeatures(optFeatures);
            features.insert(features.end(), optFeatures.begin(), optFeatures.end());
        }
        void CalcFeatures(TFeatures& features) override {
            features.clear();
            SaveFeatures(features);
        }
        const TFFIds& GetFeatureIds() const override {
            if (Ids.empty()) {
                SaveFeatureIds(Ids);
            }
            return Ids;
        }

        void SaveToJson(NJson::TJsonValue&) const override {
        }

    protected:
        void ClearSavedIds() {
            Ids.clear();
        }

    private:
        THolder<TMemoryPool> QueryPool;
        mutable TFFIds Ids;
    };

    class TIdleTextMachine
        : public TTextMachineBase
    {
    public:
        const TVector<EExpansionType>& GetAllowedExpansions() const override {
            return TExpansion::GetValuesVector();
        }
        const TVector<EStreamType>& GetAllowedStreams() const override {
            return TStream::GetValuesVector();
        }

        void AddMultiHits(const TMultiHit*, size_t) override {};
        void NewDoc() override {}
        void NewQueryInPool(const TMultiQuery&, TMemoryPool&) override {}
        void SaveOptFeatures(TOptFeatures&) override {}
        void SaveFeatureIds(TFFIds&) const override {};
    };

    inline void SaveFeatureIdsHelper(ITextMachine& machine, TFFIds& ids) {
        TMultiQuery mQuery;
        MakeDummyQuery(mQuery);

        machine.NewQuery(mQuery);
        machine.NewDoc();

        TVector<TFeature> features;
        machine.SaveFeatures(features);

        ids.reserve(ids.size() + features.size());
        for (const auto& f : features) {
            ids.push_back(f.GetId());
        }
    }
}
