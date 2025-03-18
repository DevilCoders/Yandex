#include "cache.h"

#include "output.h"

#include <library/cpp/tvmknife/internal/cache.pb.h>

#include <library/cpp/tvmauth/client/logger.h>
#include <library/cpp/tvmauth/client/misc/disk_cache.h>

#include <util/folder/dirut.h>
#include <util/system/sysstat.h>

namespace {
    class TLogger: public NTvmAuth::ILogger {
        void Log(int, const TString& msg) override {
            NTvmknife::NOutput::Out() << msg << Endl;
        }
    };
}

namespace NTvmknife {
    static const TDuration LIFETIME_OF_SRV_TICKETS = TDuration::Hours(1);

    TCache::TCache()
        : CacheDir_(GetHomeDir() + "/.tvmknife/cache/")
    {
    }

    TCache& TCache::Get() {
        static TCache ins;
        return ins;
    }

    TString TCache::GetSrvTicket(const NTvmAuth::TTvmId src, const NTvmAuth::TTvmId dst, const TString& login) {
        if (SrvTickets_.empty()) {
            SrvTickets_ = ParseSrvTickets(ReadFromDisk(GetServiceTicketsPath(), LIFETIME_OF_SRV_TICKETS));
        }

        TSrvTick k;
        k.Src = src;
        k.Dst = dst;
        k.Login = login;

        auto it = SrvTickets_.find(k);
        return it == SrvTickets_.end() ? TString() : it->second;
    }

    void TCache::SetSrvTicket(const TString& body,
                              const NTvmAuth::TTvmId src,
                              const NTvmAuth::TTvmId dst,
                              const TString& login) {
        TSrvTick k;
        k.Src = src;
        k.Dst = dst;
        k.Login = login;
        k.Ts = time(nullptr);

        SrvTickets_.emplace(k, body);

        TString toDisk = SerializeSrvTickets(SrvTickets_);
        if (toDisk) {
            WriteToDisk(GetServiceTicketsPath(), toDisk);
        }
    }

    TString TCache::ReadFromDisk(const TString& path, TDuration lifetime) {
        TLogger l;
        NTvmAuth::TDiskReader r(path, &l);
        if (!r.Read()) {
            return {};
        }

        if (TInstant::Now() - r.Time() > lifetime) {
            NOutput::Out() << "Local cache is too old" << Endl;
            return {};
        }

        return r.Data();
    }

    void TCache::WriteToDisk(const TString& path, TStringBuf body) {
        if (!NFs::MakeDirectoryRecursive(CacheDir_)) {
            NOutput::Out() << "Failed to create dir for local cache: " << CacheDir_.c_str() << Endl;
            return;
        }
        Chmod(CacheDir_.c_str(), 0700);

        TLogger l;
        NTvmAuth::TDiskWriter w(path, &l);
        w.Write(body);
    }

    TString TCache::SerializeSrvTickets(const std::map<TCache::TSrvTick, TString>& ticks) {
        cache::v1::SrvRows r;

        for (const auto& [srv, body] : ticks) {
            cache::v1::SrvRow& row = *r.add_rows();
            row.set_src(srv.Src);
            row.set_dst(srv.Dst);
            row.set_login(srv.Login);
            row.set_ticket(body);
            row.set_ts(srv.Ts);
        }

        return r.SerializeAsString();
    }

    std::map<TCache::TSrvTick, TString> TCache::ParseSrvTickets(const TString& ticks) {
        cache::v1::SrvRows r;
        if (!r.ParseFromString(ticks)) {
            return {};
        }

        std::map<TCache::TSrvTick, TString> res;
        for (const cache::v1::SrvRow& row : r.rows()) {
            time_t ts = row.Getts();
            if (TInstant::Now() - TInstant::Seconds(ts) > LIFETIME_OF_SRV_TICKETS) {
                continue;
            }

            TSrvTick k;
            k.Src = row.Getsrc();
            k.Dst = row.Getdst();
            k.Login = row.Getlogin();
            k.Ts = ts;

            res.emplace(k, row.Getticket());
        }

        return res;
    }

    void TCache::SetCacheDir(const TString& dir) {
        CacheDir_ = dir;
        Reset();
    }

    TString TCache::GetServiceTicketsPath() const {
        return CacheDir_ + "service_tickets";
    }

    void TCache::Reset() {
        SrvTickets_.clear();
    }
}
