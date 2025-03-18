#pragma once

#include <library/cpp/tvmauth/type.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

#include <map>

namespace NTvmknife {
    class TCache {
    public:
        struct TSrvTick {
            NTvmAuth::TTvmId Src = 0;
            NTvmAuth::TTvmId Dst = 0;
            TString Login;

            time_t Ts = 0;

            bool operator<(const TSrvTick& o) const {
                if (Src < o.Src) {
                    return true;
                }
                if (Dst < o.Dst) {
                    return true;
                }
                return Login < o.Login;
            }
        };

    public:
        static TCache& Get();

        TString GetSrvTicket(const NTvmAuth::TTvmId src,
                             const NTvmAuth::TTvmId dst,
                             const TString& login = {});
        void SetSrvTicket(const TString& body,
                          const NTvmAuth::TTvmId src,
                          const NTvmAuth::TTvmId dst,
                          const TString& login = {});

    public: // public - for tests
        static TString ReadFromDisk(const TString& path, TDuration lifetime);
        void WriteToDisk(const TString& path, TStringBuf body);

        static TString SerializeSrvTickets(const std::map<TSrvTick, TString>& ticks);
        static std::map<TSrvTick, TString> ParseSrvTickets(const TString& ticks);

        TCache(); // only for tests

        void SetCacheDir(const TString& dir);

        TString GetServiceTicketsPath() const;

        void Reset();

    protected:
        std::map<TSrvTick, TString> SrvTickets_;
        TString CacheDir_;
    };
}
