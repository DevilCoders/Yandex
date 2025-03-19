#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <utility>

class IInputStream;
class IOutputStream;

/// A class which returns owners by the given URLs
/// Extended owner is a domain with some path prefix
class TOwnerExtractor {
public:
    /// Constructor.
    /// \param areas Free hosting, blogs, geographical domains, etc... list files, separated by colons.
    explicit TOwnerExtractor(const TString& areasFilenames = "/Berkanavt/urlrules/areas.lst");

    explicit TOwnerExtractor(const TVector<TString>& areas);
    explicit TOwnerExtractor(IInputStream& input);
    ~TOwnerExtractor();

    // Returns an owner by given URL.
    // Please keep in mind that a pointer to a range inside given argument is returned,
    // so watch your argument's lifetime.
    TStringBuf GetOwner(TStringBuf url) const;
    TStringBuf GetOwner(const char* begin, const char*end) const {
        return GetOwner(TStringBuf(begin, end));
    }
    TStringBuf GetOwner(const char* url) const {
        return GetOwner(TStringBuf(url));
    }

    // Returns an (extended) owner with domain part in lower case
    TString GetOwnerNormalized(TStringBuf url) const;

    // Returns an owner by given host and path.
    // Host and path may point to different memory ranges
    // If extended owner found, it will be returned in a pair of buffers
    // Ordinary owner will be returned in first buffer
    std::pair<TStringBuf, TStringBuf> GetOwnerFromParts(TStringBuf host, TStringBuf path) const;

    // Checks given hostname to be the prefix any extended owners, defined in areas
    bool IsExtendable(TStringBuf host) const;

    static TOwnerExtractor& Instance() {
        return *Singleton<TOwnerExtractor>();
    }

    void Save(IOutputStream* s) const;
    void Load(IInputStream* s);

private:
    class TAreas;
    TAutoPtr<TAreas> Areas;
};
