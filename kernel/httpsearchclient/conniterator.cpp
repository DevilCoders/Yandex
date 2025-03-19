#include "conniterator.h"

#include <library/cpp/dns/cache.h>
#include <library/cpp/uri/uri.h>

#include <util/string/split.h>
#include <util/generic/hash.h>

namespace NHttpSearchClient {
    TAddress MakeAddress(const TConnData& baseConnData, TStringBuf scheme, ui32 port, TStringBuf suffix) {
        TString portStr = ToString(port);
        TStringBuf path = TStringBuf(baseConnData.Path()).Before('?');
        TString loggedAddress = TString::Join(scheme, "://", baseConnData.Host(), ":", portStr, "/", path, suffix);

        TString realAddress;
        if (baseConnData.HasUnresolvedHost()) {
            realAddress = loggedAddress;
        } else {
            TStringBuf l, r;
            if (baseConnData.Endpoint().IsIpV6()) {
                l = "[";
                r = "]";
            }

            realAddress = TString::Join(scheme, "://", l, baseConnData.Ip(), r, ":", portStr, "/", path, suffix);
        }

        return {realAddress, loggedAddress};
    }
}

namespace {
    static inline TString CompleteUriLine(TString uri) {
        size_t pos = uri.find('/');

        if (pos == TString::npos || uri[pos + 1] != '/') {
            ythrow yexception() << "bad URI string(" << uri.Quote() << ")";
        }

        // For unixsocket format: http+unix://[/tmp/unixsocket]/uri_path
        if (pos + 2 < uri.size() && uri[pos + 2] == '[') {
            pos = uri.find(']', pos + 2);
            if (pos == TString::npos) {
                ythrow yexception() << "bad URI string(" << uri.Quote() << ")";
            }
        } else {
            pos += 2;
        }

        pos = uri.find('/', pos);

        if (pos == TString::npos) {
            uri.append('/');
        }

        pos = uri.find('?');

        if (pos == TString::npos) {
            uri.append('?');
        } else {
            const char lastChar = uri.back();
            if (lastChar != '?' && lastChar != '&') {
                uri.append('&');
            }
        }

        return uri;
    }

    static inline TStringBuf GetSchemeFromUrl(TStringBuf url) noexcept {
        size_t pos = url.find(TStringBuf("://"));
        if (pos != TStringBuf::npos) {
            return url.SubStr(0, pos);
        }
        return "";
    }

    static inline TStringBuf GetHostPortFromUrl(TStringBuf url) noexcept {
        size_t pos = url.find(TStringBuf("://"));
        if (pos != TStringBuf::npos) {
            url.Skip(pos + 3);
        }
        // For unixsocket format: http+unix://[/tmp/unixsocket]/uri_path
        if (url.size() > 0 && url[0] =='[') {
            pos = url.find(']');
            if (pos == TString::npos) {
                return "";
            }
            return url.SubStr(0, pos + 1);
        }
        return url.Before('/');
    }

    static inline TStringBuf GetPathFromUrl(TStringBuf url) noexcept {
        size_t pos = url.find(TStringBuf("//"));
        if (pos != TStringBuf::npos) {
            url.Skip(pos+2);
        }

         // For unixsocket format: http+unix://[/tmp/unixsocket]/uri_path
        if (url.size() > 0 && url[0] =='[') {
            pos = url.find(']');
            if (pos == TString::npos) {
                return "";
            }
            url.Skip(pos);
        }

        return url.After('/');
    }
}

TConnGroup::TConnData::IConnDataExtension::~IConnDataExtension() {}

TConnGroup::TConnData::TConnData(const TString& script, bool enableIpV6, bool enableUnresolvedHostname
                                 , bool enableCachedResolve
                                 , TString id, bool isMain, TConnGroup* parent, const TString& ip)
    : Parent_(parent)
    , Main_(isMain)
{
    SearchScript_ = CompleteUriLine(script);
    GroupDescr_ = id;
    ui16 port = 0;

    Scheme_ = GetSchemeFromUrl(SearchScript_);
    if (Scheme_.EndsWith("+unix")) {
        Host_ = GetHostPortFromUrl(SearchScript_);
    } else {
        ::NUri::TUri pUrl;

        if (pUrl.Parse(SearchScript_, NUri::TFeature::FeatureSchemeFlexible) != 0 || pUrl.IsNull(::NUri::TUri::FlagHost)) {
            ythrow yexception() << "http connection init error(can not parse url " << SearchScript_ << ")";
        }

        Scheme_ = pUrl.GetField(::NUri::TUri::FieldScheme);
        Host_ = pUrl.GetField(::NUri::TUri::FieldHost);
        port = pUrl.GetPort();
    }

    try {
        const TString& host = ip ? ip : Host_;
        if (!Scheme_.EndsWith("+unix")) {
            if (enableCachedResolve) {
                NetworkAddress_ = NDns::CachedResolve({ host, port })->Addr;
            } else {
                NetworkAddress_.ConstructInPlace(host, port);
            }
            EndpointData_ = TEndpoint(GetFirstAddr(*NetworkAddress_, enableIpV6));

            static TEndpoint endpointNone;
            HasUnresolvedHost_ = (EndpointData_.Endpoint == endpointNone);
        } else {
            HasUnresolvedHost_ = false;
        }
    } catch (const TNetworkResolutionError& err) {
        if (!enableUnresolvedHostname) {
            throw;
        }
        try {
            Cerr << TStringBuf("invalid host/port in search script: ") << err.what() << Endl;
        } catch (...) {
        }
    }

    Path_ = GetPathFromUrl(SearchScript_);
    Address_ = NHttpSearchClient::MakeAddress(*this, Scheme(), Port(), "");
}

TEndpoint::TAddrRef TConnGroup::TConnData::GetFirstAddr(const TNetworkAddress& addr, bool enableIpV6) {
    for (TNetworkAddress::TIterator ai = addr.Begin(); ai != addr.End(); ai++) {
        if (ai->ai_family == AF_INET || (enableIpV6 && ai->ai_family == AF_INET6)) {
            return TEndpoint::TAddrRef(new NAddr::TAddrInfo(&*ai));
        }
    }
    ythrow yexception() << TStringBuf("can not find right ip address for destination")
                        << (!enableIpV6 ? TStringBuf(", - hint: ipv6 disabled") : TStringBuf());
}

TConnGroup::TConnData::TEndpointData::TEndpointData(const TEndpoint& endpoint)
    : Endpoint(endpoint)
    , Port(endpoint.Port())
    , Ip(endpoint.IpToString())
{
}

void TConnGroup::TConnData::UpdateIndexGeneration(ui32 indGen) {
    Y_ASSERT(indGen != UndefIndGenValue);

    TAtomicBase newIndGen = indGen;
    bool indGenChanged = AtomicSwap(&IndexGeneration_, newIndGen) != newIndGen;
    if (indGenChanged && Parent_ && Parent_->IndexGeneration() != indGen) {
        Parent_->UpdateIndexGeneration();
    }
}

void TConnGroup::TConnData::UpdateSourceTimestamp(ui32 ts) {
    TAtomicBase newTs = ts;
    bool tsChanged = AtomicSwap(&SourceTimestamp_, newTs) != newTs;
    if (tsChanged && Parent_ && Parent_->SourceTimestamp() != ts) {
        Parent_->UpdateSourceTimestamp();
    }
}

void TConnGroup::TConnData::Register() {
    if (Parent_) {
        Parent_->RegisterConnData(this);
        Registered_ = true;
    }
}

ui32 TConnGroup::RecalcClientIndexGeneration() {
    THashMap<ui32, size_t> indGens;

    for (TConnDataList::const_iterator it = ConnDatas_.begin(); it != ConnDatas_.end(); ++it) {
        ui32 ig = (*it)->IndexGeneration();

        if (ig != UndefIndGenValue) {
            ++indGens[ig];
        }
    }

    if (indGens.empty()) {
        return UndefIndGenValue;
    }

    THashMap<ui32, size_t>::const_iterator res = indGens.begin();

    for (THashMap<ui32, size_t>::const_iterator it = indGens.begin(); it != indGens.end(); ++it) {
        if (it->second > res->second) {
            res = it;
        }
    }

    return res->first;
}

ui32 TConnGroup::RecalcClientSourceTimestamp() {
    ui32 ts = 0;
    bool init = true;

    for (TConnDataList::const_iterator it = ConnDatas_.begin(); it != ConnDatas_.end(); ++it) {
        ui32 srcTs = (*it)->SourceTimestamp();
        if (srcTs != 0) {
            ts = init ? srcTs :  Min<ui32>(ts, srcTs);
            init = false;
        }
    }

    return ts;
}

void UpdateConnData(const TConnData* conn, const IConnIterator::THostInfo& props) {
    if (conn) {
        TConnData* cd = const_cast<TConnData*>(conn);

        if (props.IndexGeneration != UndefIndGenValue) {
            cd->UpdateIndexGeneration(props.IndexGeneration);
        }

        if (props.IsSearch) {
            cd->UpdateSourceTimestamp(props.SourceTimestamp);
        }
    }
}
