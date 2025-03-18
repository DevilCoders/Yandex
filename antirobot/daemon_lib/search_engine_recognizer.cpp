#include "search_engine_recognizer.h"

#include "eventlog_err.h"

#include <antirobot/lib/search_crawlers.h>
#include <antirobot/lib/ip_map.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/network/socket.h>
#include <util/system/mutex.h>

#if defined(_unix_) || defined(_cygwin_)
    #include <netdb.h>
#else
    #error "Function ReverseDnsLookup() is implemented only for UNIX"
#endif

namespace NAntiRobot {

namespace {

class TReverseDnsLookupError : public yexception {
};

TString ReverseDnsLookup(const TAddr& addr) {
    char host[NI_MAXHOST];
    int ret = getnameinfo(addr.Addr(), addr.Len(), host, Y_ARRAY_SIZE(host), nullptr, 0, NI_NAMEREQD);
    if (ret != 0) {
        ythrow TReverseDnsLookupError();
    }
    return TString(host);
}

class TSearchEngineBotsSet {
public:
    TSearchEngineBotsSet() {
        try {
            Load();
        } catch (TFileError&) {
            /*
             * Если не удалось открыть файл с адресами поисковых роботов, мы продолжаем
             * работать. Например, при первом запуске на машине ещё нет файла с дампом,
             * это не повод ронять роботоловилку.
             *
             * Если же удалось открыть файл, но возникли ошибки при его чтении, мы
             * должны сообщить об этом наружу и аварийно завершиться. Именно поэтому здесь
             * перехватывается только TFileError.
             */
        }
    }

    TVector<TCrawlerCandidate> EvaluateCurrentBots(const TVector<TCrawlerCandidate>& newBots) {
        TGuard<TMutex> guard(Mutex);

        AddBots(newBots);
        RemoveExpired();
        Save();

        TVector<TCrawlerCandidate> result;
        result.reserve(Bots.size());
        for (TBots::const_iterator i = Bots.begin(); i != Bots.end(); ++i) {
            result.push_back({i->second.Crawler, i->first});
        }
        return result;
    }

private:
    void AddBots(const TVector<TCrawlerCandidate>& newBots) {
        TInstant validUntil = TInstant::Now() + ANTIROBOT_DAEMON_CONFIG.SearchBotsLiveTime;

        for (size_t i = 0; i < newBots.size(); ++i) {
            Bots[newBots[i].Address] = {validUntil, newBots[i].Crawler};
        }
    }

    void RemoveExpired() {
        TVector<TAddr> toRemove;
        TInstant now = TInstant::Now();
        for (TBots::const_iterator i = Bots.begin(); i != Bots.end(); ++i) {
            if (i->second.TTL < now) {
                toRemove.push_back(i->first);
            }
        }
        for (size_t i = 0; i < toRemove.size(); ++i) {
            Bots.erase(toRemove[i]);
        }
    }

    static const char* PARTS_DELIM;

    void Save() {
        TFixedBufferFileOutput out(ANTIROBOT_DAEMON_CONFIG.SearchBotsFile);
        for (TBots::const_iterator i = Bots.begin(); i != Bots.end(); ++i) {
            out << i->first.ToString()
                << PARTS_DELIM
                << i->second.TTL.GetValue()
                << PARTS_DELIM
                << i->second.Crawler
                << Endl;
        }
    }

    void Load() {
        TFileInput in(ANTIROBOT_DAEMON_CONFIG.SearchBotsFile);
        for (TString line; in.ReadLine(line); ) {
            TVector<TString> parts = SplitString(line, PARTS_DELIM);

            TAddr botAddr(parts[0]);
            TInstant validUntil = TInstant::MicroSeconds(FromString<TInstant::TValue>(parts[1]));
            ECrawler crawler = ECrawler::None;
            if (parts.size() == 3) {
                crawler = FromString<ECrawler>(parts[2]);
            }

            Bots[botAddr] = {validUntil, crawler};
        }
    }

    /*
     * If two "reloaddata" commands are sent to an Antirobot instance simultaneously we
     * will get a race-condition on Bots. So we use mutex to access Bots just in case it
     * happens.
     */
    TMutex Mutex;

    struct TBotsData {
        TInstant TTL;
        ECrawler Crawler;
    };

    using TBots = THashMap<TAddr, TBotsData, TAddrHash>;
    TBots Bots;
};

const char* TSearchEngineBotsSet::PARTS_DELIM = "\t";

class TResolveHost {
public:
    TResolveHost(const TString& host) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int error = getaddrinfo(host.c_str(), nullptr, &hints, &AddrInfoHolder.Info);
        if (error) {
            ythrow TNetworkResolutionError(error) << ": can not resolve " << host;
        }
    }

    const addrinfo* GetAddrInfo() const {
        return AddrInfoHolder.Info;
    }

private:
    struct TAddrInfoHolder {
        struct addrinfo* Info = nullptr;

        ~TAddrInfoHolder() {
            if (Info) {
                freeaddrinfo(Info);
            }
        }
    };

    TAddrInfoHolder AddrInfoHolder;
};

}

class TSearchEngineRecognizer::TImpl {
public:
    void ProcessRequest(const TRequest& req) {
        if (req.UserAgent().empty()) {
            return;
        }

        ECrawler se = UserAgentToCrawler(req.UserAgent());
        if (se != ECrawler::None) {
            TGuard<TMutex> guard(CandidatesMutex);

            if (Candidates.size() < ANTIROBOT_DAEMON_CONFIG.MaxSearchBotsCandidates) {
                Candidates.insert({se, req.UserAddr});
            }
        }
    }

    TVector<TCrawlerCandidate> FilterCandidates() {
        TCandidates copy;
        {
            TGuard<TMutex> guard(CandidatesMutex);
            copy.swap(Candidates);
        }

        return Bots.EvaluateCurrentBots(RecognizeSearchBots(copy));
    }

private:
    bool IsCrawlerAddr(ECrawler se, const TAddr& addr) const {
        try {
            TString host = ReverseDnsLookup(addr);
            ECrawler host2se = HostToCrawler(host);
            if (host2se != se) {
                return false;
            }

            TResolveHost rh(host);
            for (auto cur = rh.GetAddrInfo(); cur; cur = cur->ai_next) {
                if (addr == TAddr(NAddr::TOpaqueAddr(cur->ai_addr))) {
                    return true;
                }
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    typedef THashSet<TCrawlerCandidate, TCandidateHash> TCandidates;

    TVector<TCrawlerCandidate> RecognizeSearchBots(const TCandidates& candidates) {
        TVector<TCrawlerCandidate> result;
        for (const TCrawlerCandidate& c : candidates) {
            if (IsCrawlerAddr(c.Crawler, c.Address)) {
                result.push_back(c);
                const auto header = MakeEvlogHeader(c.Address);
                EVLOG_MSG << header << c.Address << " is " << c.Crawler;
            }
        }
        EVLOG_MSG << "Recognized " << result.size() << " crawlers out of "
                  << candidates.size() << " candidates";
        return result;
    }

    TMutex CandidatesMutex;
    TCandidates Candidates;

    TSearchEngineBotsSet Bots;
};

TSearchEngineRecognizer::TSearchEngineRecognizer()
    : Impl(new TImpl())
{
}

TSearchEngineRecognizer::~TSearchEngineRecognizer() {
}

void TSearchEngineRecognizer::ProcessRequest(const TRequest& req) {
    Impl->ProcessRequest(req);
}

TVector<TCrawlerCandidate> TSearchEngineRecognizer::EvaluateBotsIps() {
    return Impl->FilterCandidates();
}

TSearchEngineRecognizer::TCrawlerRange TSearchEngineRecognizer::CandidatesToRanges(const TVector<TCrawlerCandidate>& crawlers) {
    THashMap<ECrawler, TVector<TAddr>> data;
    for (const auto& c : crawlers) {
        data[c.Crawler].push_back(c.Address);
    }

    TCrawlerRange result;
    for (const auto& it: data) {
        TIpList ipList;
        ipList.AddAddresses(it.second);
        for (const auto& ipIntervalsIt : ipList) {
            result.push_back({{ipIntervalsIt.IpBeg, ipIntervalsIt.IpEnd}, it.first});
        }
    }

    return result;
}

} /* namespace NAntiRobot */
