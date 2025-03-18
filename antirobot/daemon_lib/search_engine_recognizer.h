#pragma once
#include <antirobot/lib/addr.h>
#include <antirobot/lib/ip_map.h>
#include <antirobot/lib/search_crawlers.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAntiRobot {

class TRequest;
class TIpList;

struct TCrawlerCandidate {
    ECrawler Crawler;
    TAddr Address;

    bool operator == (const TCrawlerCandidate& rhs) const {
        return Crawler == rhs.Crawler && Address == rhs.Address;
    }
};

struct TCandidateHash {
    ui32 operator()(const TCrawlerCandidate& candidate) const {
        return candidate.Address.Hash() ^ ui32(candidate.Crawler);
    }
};

/// SEPE-4700
class TSearchEngineRecognizer {
public:
    TSearchEngineRecognizer();
    ~TSearchEngineRecognizer();

    /**
     * Checks whether the request's user agent matches one of the search engine bots
     * user agent strings. If it does the address from which the request was sent is
     * added to search engine bot candidates.
     */
    void ProcessRequest(const TRequest& req);

    /**
     * Iterates over search engine bot candidates and checks each of them whether it is
     * actually a search engine bot address.
     *
     * After checking is complete candidates set is cleared.
     *
     * @return Search engine bots addresses.
     * @warning This is a very expensive method since it makes forward and reverse
     *          DNS lookups.
     */
    TVector<TCrawlerCandidate> EvaluateBotsIps();

    /**
     * Compress Addr to TIpInterval
     */
    using TCrawlerRange = TVector<std::pair<TIpInterval, ECrawler>>;
    static TCrawlerRange CandidatesToRanges(const TVector<TCrawlerCandidate>& crawlers);

private:
    class TImpl;
    THolder<TImpl> Impl;
};

} /* namespace NAntiRobot */
