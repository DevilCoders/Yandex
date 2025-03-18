#pragma once

#include "addr_list.h"
#include "cbb_id.h"
#include "request_params.h"
#include "tvm.h"

#include <antirobot/lib/addr.h>
#include <antirobot/lib/future.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/stats_writer.h>

#include <util/generic/vector.h>
#include <util/string/vector.h>

#include <functional>

namespace NAntiRobot {
    class TCbbCache;

    class ICbbIO : public TNonCopyable {
    public:
        virtual TFuture<TString> ReadList(
            TCbbGroupId cbbFlag,
            const TString& format
        ) = 0;

        virtual TFuture<TVector<TInstant>> CheckFlags(TVector<TCbbGroupId> ids) = 0;

        virtual TFuture<void> AddRangeBlock(TCbbGroupId /* cbbFlag */,
            const TAddr& /* srcAddr */,
            const TAddr& /* dstAddr */,
            TInstant /* expireTime */,
            const TString& /*description */) = 0;

        virtual void PrintStats(TStatsWriter& /* out */) const = 0;
        virtual TFuture<void> AddIps(TCbbGroupId /* cbbFlag */, const TVector<TAddr>& /* addrs */) = 0;

        virtual ~ICbbIO() = default;
        virtual size_t QueueSize() const = 0;

    };

    class TCbbCacheIO : public ICbbIO {
    private:
        TCbbCache* Cache = nullptr;

    public:
        explicit TCbbCacheIO(TCbbCache* cache);

        TFuture<TString> ReadList(
            TCbbGroupId cbbFlag,
            const TString& format
        ) override;

        TFuture<TVector<TInstant>> CheckFlags(TVector<TCbbGroupId> ids) override;

        TFuture<void> AddRangeBlock(TCbbGroupId /* cbbFlag */,
            const TAddr& /* srcAddr */,
            const TAddr& /* dstAddr */,
            TInstant /* expireTime */,
            const TString& /*description */) override {
            return {};
        }

        void PrintStats(TStatsWriter& /* out */) const override {}
        TFuture<void> AddIps(TCbbGroupId /* cbbFlag */, const TVector<TAddr>& /* addrs */) override {
            return {};
        }

        size_t QueueSize() const override {
            return 0;
        }

    };

    class TCbbIO : public ICbbIO {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        struct TOptions {
            THostAddr CbbApiHost;
            TDuration Timeout;
            TCbbCache* Cache = nullptr;
        };

    public:
        TCbbIO(const TOptions& options, const TAntirobotTvm*);
        ~TCbbIO() override;

        TFuture<void> AddBlock(TCbbGroupId cbbFlag, const TAddr& addr, TInstant expireTime, const TString& description);
        TFuture<void> AddIps(TCbbGroupId cbbFlag, const TVector<TAddr>& addrs) override;
        TFuture<void> AddRangeBlock(TCbbGroupId cbbFlag, const TAddr& srcAddr, const TAddr& dstAddr, TInstant expireTime, const TString& description) override;
        TFuture<void> AddTxtBlock(TCbbGroupId cbbFlag, const TString& text, const TString& description);

        TFuture<TVector<TInstant>> CheckFlags(TVector<TCbbGroupId> ids) override;

        TFuture<TString> ReadList(
            TCbbGroupId cbbFlag,
            const TString& format
        ) override;

        TFuture<TMaybe<TCbbAddrSet>> ReadExpiredList(TCbbGroupId cbbFlag, TInstant timeStamp = TInstant::Zero());
        TFuture<TMaybe<TVector<TString>>> ReadReList(TCbbGroupId flag);
        TFuture<TMaybe<TVector<TString>>> ReadTextList(TCbbGroupId flag);
        TFuture<void> RemoveTxtBlock(TCbbGroupId cbbFlag, const TString& txtBlock);
        void PrintStats(TStatsWriter& out) const override;
        size_t QueueSize() const override;
    };
} // namespace NAntiRobot
