#pragma once

#include <util/generic/string.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>
#include "mirrors_trie.h"
#include "mirrors_wrapper.h"

class TBadUrlFilter
{
protected:
    static const size_t MAX_URL_LEN = 1024;
    THolder< TTrieSetWriter<> > BadUrlsBuilder;
    THolder<TTrieSet> BadUrls;
    TBufferOutput TrieBuf;
    bool TrieNeedsRelease;
    bool WriterNeedsRelease;

protected:
    void FindUrlBounds(const char* url, size_t* urlStart, size_t* urlEnd) const
    {
        size_t len = strlen(url);
        *urlStart = 0;
        *urlEnd = len;

        while (*urlEnd > 0 && IsWhitespace(url[*urlEnd - 1]))
            (*urlEnd)--;
        while (*urlEnd > 0 && IsUrlTerminator(url[*urlEnd - 1]))
            (*urlEnd)--;
        while (*urlStart < *urlEnd && IsWhitespace(url[*urlStart]))
            (*urlStart)++;
    }

    bool IsUrlTerminator(const char& ch) const
    {
        return (ch == '/' || ch == '?' || ch == '&' || ch == ':');
    }

    bool IsWhitespace(const char& ch) const
    {
        return (ch == ' ' || ch == '\t' ||  ch == '\n');
    }

    TString InvertHost(const char * url, size_t len) const
    {
        if (len > MAX_URL_LEN)
            len = MAX_URL_LEN;
        size_t hostEnd = GetHostEnd(url, len);

        TString invertedHost;
        invertedHost.reserve(len);

        for (size_t i = 0; i < hostEnd; ++i)
            invertedHost.push_back((char)tolower(url[hostEnd - i - 1]));
        for (size_t i = hostEnd; i < len; ++i)
            invertedHost.push_back((char)tolower(url[i]));

        return invertedHost;
    }

    virtual void InternalWrite(const char * url, size_t len)
    {
        if (!BadUrlsBuilder)
            ythrow yexception() << "url filter is not ready for writing (already prepared for reading?)";

        TString invertedHost = InvertHost(url, len);
        BadUrlsBuilder->Add(invertedHost.data(), invertedHost.size());
    }

    size_t GetHostEnd(const char * url, size_t len) const
    {
        size_t pos = 0;
        while (pos < len && url[pos] != '/' && url[pos] != ':')
            pos++;
        return pos;
    }

    bool WriteUrlImpl(const char * url, IMirrors * mir, bool onlyMainMirrors = false)
    {
        size_t urlStart, urlEnd;
        FindUrlBounds(url, &urlStart, &urlEnd);
        if (urlStart == urlEnd)
            return false;
        TString CurUrl(url, urlStart, urlEnd - urlStart);
        CurUrl.to_lower();
        if (mir)
        {
            size_t repl_len = CurUrl.find('/');
            TString host = CurUrl.substr(0, repl_len); //npos is allowed second argument
            if (!onlyMainMirrors || mir->IsMain(host.data()))
            {
                IMirrors::THosts hosts;
                mir->GetGroup(host.data(), &hosts);
                if (hosts.size())
                {
                    for (size_t i = 0; i < hosts.size(); i++)
                    {
                        CurUrl.replace(0, repl_len, hosts[i].data());
                        repl_len = hosts[i].size(); // new host len is not same as previous!
                        InternalWrite(CurUrl.data(), CurUrl.size()); //can't be optimized because of : handling
                    }
                    return true;
                }
            }
        }
        InternalWrite(CurUrl.data(), CurUrl.size());
        return true;
    }

    bool WriteUrlImplEx(const char * url, TMirrorsMappedTrie * mir, bool onlyMainMirrors = false)
    {
        size_t urlStart, urlEnd;
        FindUrlBounds(url, &urlStart, &urlEnd);
        if (urlStart == urlEnd)
            return false;
        TString CurUrl(url, urlStart, urlEnd - urlStart);
        CurUrl.to_lower();
        if (mir)
        {
            size_t repl_len = CurUrl.find('/');
            TString host = CurUrl.substr(0, repl_len); //npos is allowed second argument
            if (!onlyMainMirrors || mir->IsMain(host.data()))
            {
                TMirrorsMappedTrie::TGroup hosts = mir->GetGroup(host.data());
                if (hosts.GetSize())
                {
                    TString curHost;
                    for (size_t i = 0; i < hosts.GetSize(); i++)
                    {
                        mir->IdToString(hosts[i], curHost);
                        CurUrl.replace(0, repl_len, curHost.data());
                        repl_len = curHost.size(); // new host len is not same as previous!
                        InternalWrite(CurUrl.data(), CurUrl.size()); //can't be optimized because of : handling
                    }
                    return true;
                }
            }
        }
        InternalWrite(CurUrl.data(), CurUrl.size());
        return true;
    }

    static TMirrors* GetMirrors(mirrors * mir) {return new TMirrors(mir);}
    static TMirrorsHashed* GetMirrors(mirrors_mapped * mir) {return new TMirrorsHashed(mir);}

public:
    bool WriteUrl(const char * url)
    {
        return WriteUrlImpl(url, nullptr);
    }

    template <typename T>
    bool WriteUrl(const char * url,/*const */T * mir, bool onlyMainMirrors = false) //const is not used because of mirrors declarations
    {
        if (!mir)
            return WriteUrlImpl(url, nullptr);
        THolder<IMirrors> mirror;
        mirror.Reset(GetMirrors(mir));
        return WriteUrlImpl(url, mirror.Get(), onlyMainMirrors);
    }

    bool WriteUrl(const char * url,/*const */TMirrorsMappedTrie * mir, bool onlyMainMirrors = false) //const is not used because of mirrors declarations
    {
        if (!mir)
            return WriteUrlImplEx(url, nullptr);

        return WriteUrlImplEx(url, mir, onlyMainMirrors);
    }

    void PrepareForReading(void)
    {
        if (!BadUrlsBuilder.Get()) //Ignoring unnecessary call
            return;
        BadUrlsBuilder->Save(TrieBuf);
        if (WriterNeedsRelease)
            Y_UNUSED(BadUrlsBuilder.Release());
        else
            BadUrlsBuilder.Destroy();
        WriterNeedsRelease = false;
        BadUrls.Reset(new TTrieSet(TBlob::NoCopy(TrieBuf.Buffer().Data(), TrieBuf.Buffer().Size())));
    }

    TBadUrlFilter()
        : TrieNeedsRelease(false)
        , WriterNeedsRelease(false)
    {
        BadUrlsBuilder.Reset(new TTrieSetWriter<>);
    }

    TBadUrlFilter(TTrieSet* trie)
        : BadUrls(trie)
        , TrieNeedsRelease(true)
        , WriterNeedsRelease(false)
    {
    }

    TBadUrlFilter(TTrieSetWriter<>* trie)
        : BadUrlsBuilder(trie)
        , TrieNeedsRelease(false)
        , WriterNeedsRelease(true)
    {
    }

    bool CheckUrl(const char * url) const
    {
        if (!BadUrls)
            ythrow yexception() << "url filter is not ready for reading, call PrepareForReading() first";
        size_t urlStart, urlEnd;
        FindUrlBounds(url, &urlStart, &urlEnd);
        if (urlStart == urlEnd)
            return false;
        TString invertedHost = InvertHost(url + urlStart, urlEnd - urlStart);
        size_t prefixLen;
        if (!BadUrls->FindLongestPrefix(invertedHost.data(), invertedHost.size(), &prefixLen))
            return false;
        if (invertedHost.size() == prefixLen || IsUrlTerminator(invertedHost[prefixLen]))
            return true;
        return (invertedHost[prefixLen] == '.' && prefixLen < GetHostEnd(url, invertedHost.size()));
    }

    virtual ~TBadUrlFilter()
    {
        if (TrieNeedsRelease)
            Y_UNUSED(BadUrls.Release());
        if (WriterNeedsRelease)
            Y_UNUSED(BadUrlsBuilder.Release());
    }

    template<typename T>
    void Init(const char * filterBasePath, T * mir, bool onlyMainMirrors = false)
    {
        char str[8192];
        FILE * fInp = fopen(filterBasePath, "rt");
        if (fInp == nullptr)
            throw TIoSystemError() << "failed to fopen " << filterBasePath;
        setvbuf ( fInp , nullptr , _IOFBF , 8192 );
        while (fgets(str, 8191, fInp))
        {
            if (str[0] == 0 || str[0] == '#')
                continue;
            WriteUrl(str, mir, onlyMainMirrors);
        }
        fclose(fInp);
        PrepareForReading();
    }

    void Init(const char * filterBasePath, const char * mirrorsBasePath = nullptr, bool onlyMainMirrors = false)
    {
        THolder<mirrors> mir;
        if ((mirrorsBasePath != nullptr) && (mirrorsBasePath[0] != 0))
            mir.Reset(new mirrors(false, mirrorsBasePath));

        Init(filterBasePath, mir.Get(), onlyMainMirrors);
    }
};
