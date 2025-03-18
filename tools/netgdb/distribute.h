#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/guard.h>
#include <util/system/spinlock.h>
#include <util/digest/murmur.h>

#include "comm.h"

void SetStoragePath(TString path);
TString GetPathForFile(const TString& file, const TString& user);
void CleanUp();

bool StoreFile(const TString& fn, const TString& hash, TVector<char>::const_iterator start, TVector<char>::const_iterator end);

class TLazyContentsGetter {
    TString Src;
    TAdaptiveLock Lock;
    TPack Contents;
public:
    TLazyContentsGetter(TString src)
        : Src(src)
    {}

    TVector<char>* GetContents()
    {
        TGuard lock(Lock);
        if (!Contents) {
            TVector<char>* contents = new TVector<char>;
            {
                char buffer[4096];
                TIFStream file(Src);
                int read;
                while ((read = file.Read(buffer, sizeof(buffer))) > 0)
                    contents->insert(contents->end(), buffer, buffer + read);
            }
            Contents = contents;
        }
        return Contents.Get();
    }
};

struct THostStatus {
    TString Host;
    NNetliba_v12::TUdpAddress Address;

    THostStatus(TString host)
        : Host(host), Address(NNetliba_v12::CreateAddress(host.data(), RemotePort(host.data())))
    {}
};

void Cluster(int clustCount, TVector<THostStatus>& items, TVector<TVector<THostStatus*> >* chunks);
void Distribute(const TString& file, const TString& hash, TLazyContentsGetter& content, const TVector<TString>& hosts, TVector<TString>* fails, const TString& id);
void Distribute(const TString& src, const TVector<TString>& hosts, TVector<TString>* fails, TString* hash, const TString& id);

inline TString GetHashForFileContents(const TString& fileName, TVector<char>::const_iterator start, TVector<char>::const_iterator end)
{
    const TString hash = ::ToString<ui32>((ui32)MurmurHash(start, end - start, MurmurHash(fileName.begin(), fileName.size(), (ui32)0)));
    return hash;
}
