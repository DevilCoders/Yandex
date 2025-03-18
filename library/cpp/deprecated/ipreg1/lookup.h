#pragma once

#include <library/cpp/deprecated/ipreg1/struct.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NIpreg {
    namespace NDetails {
        template <typename Setup>
        class TLookupImpl;

        struct NetImpl;
    }

    struct TCppSetup {
        typedef TMap<TString, TString> TDict;
    };

    class TLookup {
    public:
        explicit TLookup(const char* path, bool bigNetsOnly = true);
        virtual ~TLookup();

        Net GetNet(const TString& ip, const TCppSetup::TDict& headers) const;
        Net GetNet(const TString& ip) const;

        bool IsYandex(const TString& ip) const;

    private:
        TLookup(const TLookup&) = delete;
        TLookup& operator=(const TLookup&) = delete;

        TSimpleSharedPtr<NDetails::TLookupImpl<TCppSetup>> Impl;
    };
}
