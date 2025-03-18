#include <library/cpp/deprecated/ipreg1/lookup.h>
#include <library/cpp/deprecated/ipreg1/lookup_impl.h>

namespace NIpreg {
    TLookup::TLookup(const char* filename, bool bigNetsOnly)
        : Impl(new NDetails::TLookupImpl<TCppSetup>(filename, bigNetsOnly))
    {
    }

    TLookup::~TLookup() {
    }

    Net TLookup::GetNet(const TString& ip) const {
        return Impl->GetNet(ip);
    }

    Net TLookup::GetNet(const TString& ip, const TCppSetup::TDict& headers) const {
        return Impl->GetNet(ip, headers);
    }

    bool TLookup::IsYandex(const TString& ip) const {
        return Impl->IsYandex(ip);
    }
}
