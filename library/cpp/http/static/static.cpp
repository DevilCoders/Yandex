#include "static.h"

#include <library/cpp/charset/ci_string.h>
#include <util/folder/dirut.h>
#include <library/cpp/http/misc/httpdate.h>
#include <util/datetime/base.h>

#include <locale.h>

#ifdef _win32_
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <util/folder/fts.h>
#include <netdb.h>
#endif

template <class T1>
inline void PrintContentType(const char* mime, T1& conn) {
    if (mime) {
        conn << "Content-Type: " << mime << "\r\n";
    }
}

template <class T1>
inline void PrintLastModified(time_t T, T1& conn) {
    if (T) {
        char lastmodstr[100];
        format_http_date(lastmodstr, sizeof(lastmodstr), T);

        conn << "Last-Modified: " << lastmodstr << "\r\n";
    }
}

template <class T1>
inline void PrintContentLength(ui32 size, T1& conn) {
    if (size) {
        conn << "Content-Length: " << size << "\r\n";
    }
}

template <class T1>
inline void PrintCacheControl(TDuration maxAge, T1& conn) {
    conn << "Cache-Control: max-age=" << maxAge.Seconds() << "\r\n";
}

template <class T1>
inline void PrintAddHeaders(const char* const* add_headers, T1& conn) {
    if (add_headers) {
        for (const char* p = *add_headers++; p; p = *add_headers++) {
            conn << p;
        }
    }
}

static const TDuration DefaultMaxAge = TDuration::Days(1);

void TStaticDoc::Print(time_t ims, TClientRequest* clr, const char* const* add_headers) const {
    return Print(ims, clr->Output(), add_headers);
}

void TStaticDoc::Print(time_t ims, IOutputStream& conn, const char* const* add_headers) const {
    if (mtime <= ims) {
        conn << "HTTP/1.1 304 Not modified\r\n";
        PrintCacheControl(DefaultMaxAge, conn);
        conn << "\r\n";

        return;
    }

    conn << "HTTP/1.1 200 OK\r\n";
    PrintContentType(mime, conn);
    PrintLastModified(mtime, conn);
    PrintContentLength(size, conn);
    PrintCacheControl(DefaultMaxAge, conn);
    PrintAddHeaders(add_headers, conn);
    conn << "\r\n";
    conn.Write((const char*)data, size);
}

bool TLocalDoc::Print(time_t ims, TClientRequest* clr, const char* const* add_headers) {
    return Print(ims, clr->Output(), add_headers);
}

bool TLocalDoc::Print(time_t ims, IOutputStream& conn, const char* const* add_headers) {
    if (!m_file.IsOpen()) {
        return false;
    } else {
        if (m_mtime <= ims) {
            conn << "HTTP/1.1 304 Not modified\r\n";
            PrintCacheControl(DefaultMaxAge, conn);
            conn << "\r\n";

            return true;
        }

        conn << "HTTP/1.1 200 OK\r\n";
        PrintContentType(m_mime, conn);
        PrintLastModified(m_mtime, conn);
        PrintContentLength(m_size, conn);
        PrintCacheControl(DefaultMaxAge, conn);
        PrintAddHeaders(add_headers, conn);
        conn << "\r\n";

        i32 size = 0;
        ui8 data[4096];

        while ((size = m_file.Read(&data, sizeof(data))) > 0) {
            conn.Write((const char*)data, size);
        }

        return true;
    }
}

void CHttpServerStatic::AddMime(const char* ext, const char* mime) {
    Mimes.insert(THashMap<TCiString, TString>::value_type(ext, mime));
}

#define MAX_STATIC_DOC_SIZE (1 << 24)

bool CHttpServerStatic::AddDoc(TStringBuf webpath, const char* locpath) {
    const char* mime = nullptr;
    const char* ext = strrchr(locpath, '.');
    if (ext) {
        THashMap<TCiString, TString>::const_iterator it = Mimes.find(ext);
        if (it == Mimes.end())
            return false;
        mime = it->second.data();
    } else {
        return false;
    }
    if (StaticDocs.contains(webpath))
        return false;
    TFile doc(locpath, OpenExisting | RdOnly);
    if (!doc.IsOpen())
        return false;
    i64 Len = doc.GetLength();
    if (Len > MAX_STATIC_DOC_SIZE) {
        doc.Close();
        return false;
    }
    ui32 size = (ui32)Len;
    ui8* data = new ui8[size];
    doc.Read(data, size);
    doc.Close();
    time_t mtime = 0;
    struct stat fs;
    if (!stat(locpath, &fs))
        mtime = fs.st_mtime;

    StaticDocs[webpath].SetData(data, size, mtime, mime, true);
    return true;
}

bool CHttpServerStatic::AddDoc(TStringBuf webpath, ui8* d, ui32 s, const char* m) {
    if (StaticDocs.contains(webpath))
        return false;
    StaticDocs[webpath].SetData(d, s, time(nullptr), m, false);
    return true;
}

bool CHttpServerStatic::HasDoc(TStringBuf webpath) const {
    return StaticDocs.contains(webpath);
}

void CHttpServerStatic::PathHandle(TStringBuf path, TClientRequest* clr, time_t ims) {
    PathHandle(path, clr->Output(), ims);
}

void CHttpServerStatic::PathHandle(TStringBuf path, IOutputStream& outStream, time_t ims) {
    const auto* ptr = StaticDocs.FindPtr(path);

    if (!ptr) {
        outStream << "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/plain\r\n"
                  << "Content-Length: 38\r\n\r\n"
                     "HTTP 404 - The page cannot be found.\r\n";

        return;
    }

    ptr->Print(ims, outStream);
}

void CHttpServerStatic::InitImages(const char* locpath, const char* webpath) {
    assert(locpath);
    assert(webpath);
    TString SitePath(webpath);
    if (SitePath[SitePath.size() - 1] != '/')
        SitePath.append("/");
    TString Local(locpath);
    if (Local[Local.size() - 1] != LOCSLASH_C)
        Local.append(LOCSLASH_S);
#ifdef _win32_
    char buf[512];
    sprintf(buf, "%s*.*", Local.data());
    struct _finddata_t findData;
    intptr_t hFind = _findfirst(buf, &findData);
    if (hFind != -1) {
        do {
            if (findData.attrib & _A_SUBDIR) {
                if (findData.name[0] != '.')
                    InitImages((Local + findData.name).data(), (SitePath + findData.name).data());
            } else
                AddDoc((SitePath + findData.name).data(), (Local + findData.name).data());
        } while (_findnext(hFind, &findData) == 0);
        _findclose(hFind);
    }
#else
    size_t LocalLen = Local.size();
    char* trees[2];
    trees[0] = (char*)locpath;
    trees[1] = nullptr;
    FTS* FileTree = yfts_open(trees, FTS_LOGICAL, nullptr);
    if (FileTree == nullptr)
        return;
    FTSENT* ent = nullptr;
    while ((ent = yfts_read(FileTree))) {
        if (ent->fts_info != FTS_F)
            continue;
        AddDoc((SitePath + (ent->fts_path + LocalLen)).data(), ent->fts_path);
    }
    yfts_close(FileTree);
#endif
}
