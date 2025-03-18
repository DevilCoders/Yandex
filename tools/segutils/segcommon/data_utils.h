#pragma once

#include <kernel/segutils_dater/numerator_utils.h>
#include <kernel/segutils/numerator_utils.h>

#include <util/charset/wide.h>
#include <util/folder/filelist.h>
#include <util/stream/buffered.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/generic/strbuf.h>

#include <library/cpp/http/fetch/httpload.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace NSegutils {

typedef THashMap<TString, TString> TMapping;

class TDataException: public yexception {
};

inline TString FormatPosting(TPosting p) {
    return Sprintf("[%u.%u.%u.%u]", GetBreak(p), GetWord(p), (ui32) GetRelevLevel(p),
            GetPostingNForm(p));
}

struct TColumnInfo {
    i32 Col;

    TColumnInfo(i32 col = Min<i32>())
        : Col(col)
    {}

    bool Have() const {
        return Col != Min<i32>();
    }
};

struct THtmlFile: public THtmlDocument {
    TString FileName;
    NDater::TDaterDate RealDate;
};

struct TColumnMetaData {
    TColumnInfo Url;
    TColumnInfo Date;
    TColumnInfo RealDate;

    bool DateIsTimeT;
    bool HtmlComment;

    TColumnMetaData()
        : DateIsTimeT()
        , HtmlComment()
    {}

    void Process(TString meta, THtmlFile& doc) const;

    static bool Column(size_t& res, int col, size_t sz) {
        res = col >= 0 ? col : sz + col;
        return res < sz;
    }
};

struct THtmlFileReader {
    enum EMetaDataMode {
        MDM_None,
        MDM_FirstLine,
        MDM_Mapping
    };

    TColumnMetaData MetaData;

    TMapping File2MetaData;
    TMapping Url2File;

    TVector<TString> FileList;
    TString CommonPrefix;
    TDateTime CommonTime;

    i32 FileNameColumn;
    EMetaDataMode MetaMode;

    THtmlFileReader(EMetaDataMode m = MDM_Mapping)
        : FileNameColumn()
        , MetaMode(m)
    {
        MetaData.Url = MetaMode == MDM_Mapping;
    }

    void SetDirectory(const TString& dir) {
        if (!dir.empty())
            CommonPrefix = dir + "/";
        else
            CommonPrefix = "";
    }

    void InitMapping(const TString& file) {
        TUnbufferedFileInput fi(file);
        InitMapping(fi);
    }

    void InitMapping(IInputStream& is);

    THtmlFile Read(const TString& fname, bool readbody = true) const;

    TString GetUrl(const TString& fname) const;
};


void Fetch(THtmlDocument& doc, time_t tout = 15, size_t maxsize = 512 * 1024, bool onlyhtml = true);

inline THtmlDocument Fetch(TStringBuf url, time_t tout = 15, size_t maxsize = 512 * 1024, bool onlyhtml = true) {
    THtmlDocument doc;
    doc.Url = url;
    Fetch(doc, tout, maxsize, onlyhtml);
    return doc;
}

NDater::TDaterDate ScanDateSimple(TStringBuf);

}
