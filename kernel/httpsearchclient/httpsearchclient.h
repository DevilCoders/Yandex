#pragma once

#include "hostinfo.h"
#include "requesthash.h"
#include "conniterator.h"

#include <kernel/httpsearchclient/config/boptions.h>

#include <util/generic/ptr.h>

namespace NHttpSearchClient {
    extern size_t SmoothThreshold;

    struct TClientOptions {
        const TString* Script;
        const TBalancingOptions* Options;
        const TString* GroupId;
        bool IsMain;
    };

    struct TSeed {
    public:
        TSeed(TRequestHash hash = 0)
            : Hash(hash)
        {
        }
    public:
        TRequestHash Hash;
        bool AllowRandomGroupSelection = true;
        bool LinearHash = false;
    };

    class THttpSearchClientBase {
    public:
        class TGenericIterator;
        typedef TIntrusivePtr<TGenericIterator> TGenericIteratorRef;
        typedef NHttpSearchClient::THostInfo THostInfo;

        class TGenericIterator: public IConnIterator, public TAtomicRefCount<TGenericIterator> {
            public:
                inline TGenericIterator() noexcept {
                }

                ~TGenericIterator() override {
                }

                void SetClientStatus(const TConnData*, const THostInfo&, const TErrorDetails&) override {
                }

                virtual bool Eof() const = 0;
        };
    public:
        virtual ~THttpSearchClientBase() {
        }

        THolder<IConnIterator> PossibleConnections(TRequestHash hash) const;
        THolder<IConnIterator> PossibleConnections(TSeed seed) const;

        virtual TGenericIteratorRef CreateGenericIterator(TSeed seed) const = 0;
        virtual void DoReportStats(IOutputStream& out) const = 0;
    
    protected:
        inline THttpSearchClientBase(const TString& script, const TBalancingOptions& options)
            : Script_(script)
            , Options_(options)
        {
        }

    protected:
        const TString Script_;
        const TBalancingOptions Options_;
    };

    class TEmptySearchClient : public THttpSearchClientBase {
    public:
        class TEmptyIterator : public TGenericIterator {
        public:
            using TGenericIterator::TGenericIterator;

            bool Eof() const override { return true; }
            const TConnData* Next(size_t) override { return nullptr; }
        };

        TEmptySearchClient(): THttpSearchClientBase({}, {}) {}

        TGenericIteratorRef CreateGenericIterator(TSeed) const override { return new TEmptyIterator{}; }
        void DoReportStats(IOutputStream&) const override {};
    };

    struct TGroupData {
        TString Group;
        TString GroupWeights;
        TString GroupOptions;
        TString Ip;

        inline void Clear() noexcept {
            Group.clear();
            GroupWeights.clear();
            GroupOptions.clear();
            Ip.clear();
        }
    };

    bool SplitToGroups(const TString& str, TVector<TGroupData>& groups, bool rawIpAddrs);

    using TGroupSplitter = std::function<decltype(SplitToGroups)>;
    using TClientCreator = std::function<THolder<THttpSearchClientBase>(const TClientOptions&, TConnGroup*, const TString&, TGroupSplitter)>;

    THolder<THttpSearchClientBase> CreateClient(const TClientOptions& opts, TConnGroup* parent, TGroupSplitter groupSplitter = SplitToGroups,
                                                TClientCreator clientCreator = {});
    THolder<THttpSearchClientBase> CreateWeightedClient(const TVector<TGroupData>& groups, const TClientOptions& opts, TConnGroup* parent,
                                                        TGroupSplitter groupSplitter = SplitToGroups, TClientCreator clientCreator = {});
}
