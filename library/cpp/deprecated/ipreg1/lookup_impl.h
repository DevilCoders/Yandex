#pragma once

#include <library/cpp/deprecated/ipreg1/internal_struct.h>
#include <library/cpp/deprecated/ipreg1/struct.h>

#include <library/cpp/json/json_reader.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/split.h>

namespace NIpreg {
    namespace NDetails {
        template <typename Setup>
        class TLookupImpl {
        public:
            explicit TLookupImpl(const char* path, bool bigNetsOnly);
            virtual ~TLookupImpl();

            Net GetNet(const TString& ip, const typename Setup::TDict& headers) const;
            Net GetNet(const TString& ip) const;

            bool IsYandex(const TString& ip) const;

        private:
            using TNet6 = TSubnet;
            using TNet6Vec = TVector<TNet6>;

            bool InCont(const TNet6Vec& cont, const TString& ip) const;

            TString FindHeader(const typename Setup::TDict& headers, const TString& header) const;
            void LoadData(const char* jsonFname, bool bigNetsOnly);

            TNet6Vec Subnets;
            TNet6Vec Turbo;
            TNet6Vec Users;
        };

        template <typename Setup>
        TLookupImpl<Setup>::TLookupImpl(const char* filename, bool bigNetsOnly) {
            LoadData(filename, bigNetsOnly);

            Sort(Subnets);

            if (Turbo.empty()) {
                ythrow yexception() << "'" << filename << "': turbo-list is empty";
            }
            Sort(Turbo);

            if (Users.empty()) {
                ythrow yexception() << "'" << filename << "': users-list is empty";
            }
            Sort(Users);
        }

        template <typename Setup>
        TLookupImpl<Setup>::~TLookupImpl() {
        }

        template <typename Setup>
        void TLookupImpl<Setup>::LoadData(const char* jsonFname, bool bigNetsOnly) {
            TUnbufferedFileInput jsonFile(jsonFname);
            NJson::TJsonValue ranges;
            NJson::ReadJsonTree(&jsonFile, &ranges);

            if (!ranges.IsArray()) {
                ythrow yexception() << "failed to parse json '" << jsonFname << "'; wanted array with nets descriptions";
            }

            const auto& ranges_arr = ranges.GetArray();
            const auto elems_amount = ranges_arr.size();
            for (int index = 0; index != elems_amount; ++index) {
                const auto& range = ranges_arr[index];
                if (!range.Has("low") || !range.Has("high")) {
                    continue;
                }

                const auto& lo = range["low"].GetString();
                const auto& hi = range["high"].GetString();
                const auto& flags = range["flags"];

                const TNet6 net(hi, lo);
                if (bigNetsOnly) {
                    if (!flags["sub"].GetBoolean()) {
                        Subnets.push_back(net);
                        continue;
                    }
                }
                if (flags["turbo"].GetBoolean() && !flags["t-sub"].GetBoolean()) {
                    Turbo.push_back(net);
                }
                if (flags["user"].GetBoolean() && !flags["u-sub"].GetBoolean()) {
                    Users.push_back(net);
                }
            }

            if (Subnets.empty()) {
                ythrow yexception() << "'" << jsonFname << "': subnets not loaded";
            }
        }

        template <typename Setup>
        bool TLookupImpl<Setup>::InCont(const TNet6Vec& cont, const TString& ip) const {
            const TNet6 net(ip);
            TNet6Vec::const_iterator low = std::lower_bound(cont.begin(), cont.end(), net);
            return cont.end() != low && low->IsIn(net.Lo);
        }

        template <typename Setup>
        inline TString TLookupImpl<Setup>::FindHeader(const typename Setup::TDict& headers, const TString& header) const {
            typename Setup::TDict::const_iterator it = headers.find(header);
            return (headers.end() != it ? it->second : "");
        }

        template <typename Setup>
        Net TLookupImpl<Setup>::GetNet(const TString& ip) const {
            return {
                InCont(Subnets, ip),
                ip,
                InCont(Users, ip)};
        }

        template <typename Setup>
        Net TLookupImpl<Setup>::GetNet(const TString& ip, const typename Setup::TDict& headers) const {
            if (InCont(Turbo, ip)) {
                const TString& header = FindHeader(headers, "X-Forwarded-For");
                if (!header.empty()) {
                    TVector<TString> addrs;
                    StringSplitter(header).Split(',').AddTo(&addrs);
                    for (const auto& addr : addrs) {
                        return GetNet(addr);
                    }
                }
            }
            return GetNet(ip);
        }

        template <typename Setup>
        bool TLookupImpl<Setup>::IsYandex(const TString& ip) const {
            return InCont(Subnets, ip);
        }
    }
}
