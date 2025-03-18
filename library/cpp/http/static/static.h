#pragma once

#include <library/cpp/http/server/http_ex.h>

#include <library/cpp/charset/ci_string.h>
#include <util/system/file.h>
#include <util/system/fstat.h>
#include <util/system/defaults.h>
#include <util/generic/hash.h>

#include <ctime>
#include <cstring>
#include <util/generic/noncopyable.h>

class TStaticDoc {
public:
    TStaticDoc()
        : data(nullptr)
        , mtime(0)
        , size(0)
        , mime(nullptr)
        , autodel(false)
    {
    }
    ~TStaticDoc() {
        if (autodel)
            delete[] data;
    }
    void SetData(const ui8* d, ui32 s, time_t t, const char* m, bool a) {
        data = d;
        size = s;
        mtime = t;
        mime = m;
        autodel = a;
    }
    void Print(time_t ims, TClientRequest* clr, const char* const* add_headers = nullptr) const;
    void Print(time_t ims, IOutputStream& outStream, const char* const* add_headers = nullptr) const;

private:
    const ui8* data;
    time_t mtime;
    ui32 size;
    const char* mime;
    bool autodel;

private:
    TStaticDoc(const TStaticDoc&);
    TStaticDoc& operator=(const TStaticDoc&);
};

class TLocalDoc {
public:
    TLocalDoc(const char* local, const char* mime)
        : m_file(local, OpenExisting | RdOnly | Seq)
        , m_mime(mime)
    {
        if (m_file.IsOpen()) {
            TFileStat fs(m_file);
            m_mtime = fs.MTime;
            m_size = fs.Size;
        }
    }

    bool Print(time_t IfModSince, TClientRequest*, const char* const* add_headers = nullptr);
    bool Print(time_t IfModSince, IOutputStream& outStream, const char* const* add_headers = nullptr);

private:
    TFile m_file;
    const char* m_mime;
    time_t m_mtime;
    ui64 m_size;

private:
    TLocalDoc(const TLocalDoc&);
    TLocalDoc& operator=(const TLocalDoc&);
};

class CHttpServerStatic : TNonCopyable {
public:
    CHttpServerStatic() = default;
    void PathHandle(TStringBuf path, TClientRequest* clr, time_t ims = 0);
    void PathHandle(TStringBuf path, IOutputStream& outStream, time_t ims = 0);
    void AddMime(const char* ext, const char* mime);
    bool AddDoc(TStringBuf webpath, const char* locpath);
    bool AddDoc(TStringBuf webpath, ui8* d, ui32 s, const char* m);
    bool HasDoc(TStringBuf webpath) const;
    void InitImages(const char* locpath, const char* webpath = "/");

protected:
    THashMap<TString, TStaticDoc> StaticDocs;
    THashMap<TCiString, TString> Mimes;
};
