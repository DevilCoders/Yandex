#include "data_utils.h"

#include <yweb/news/fetcher_lib/fetcher.h>

#include <util/string/strip.h>

namespace NSegutils {

void TColumnMetaData::Process(TString meta, THtmlFile& doc) const {
    StripInPlace(meta);

    if (HtmlComment) {
        if (meta.StartsWith("<!--"))
            meta = meta.substr(4);
        if (meta.EndsWith("-->"))
            meta = meta.substr(0, meta.size() - 3);

        StripInPlace(meta);
    }

    TVector<TString> vs = SplitString(meta, "\t", 0, KEEP_EMPTY_TOKENS);

    size_t urlcol = 0;
    size_t dcol = 0;
    size_t rdcol = 0;

    if (!Column(urlcol, Url.Col, vs.size()))
        ythrow TDataException() << "no url in " << urlcol << " column of metadata '" << meta << "'";

    doc.Url = AddSchemePrefix(vs[urlcol]);

    if (Column(dcol, Date.Col, vs.size()) && dcol != urlcol) {
        if (DateIsTimeT) {
            doc.Time.SetTimestamp(FromString<time_t>(vs[dcol]));
        } else {
            doc.Time.SetDate(ScanDateSimple(vs[dcol]));
        }
    }

    if (Column(rdcol, RealDate.Col, vs.size()) && rdcol != urlcol) {
        doc.RealDate = ScanDateSimple(vs[rdcol]);
    }
}

void THtmlFileReader::InitMapping(IInputStream& is) {
    TString line;
    while (is.ReadLine(line)) {
        StripInPlace(line);
        if (line.empty())
            continue;

        TVector<TString> vs = SplitString(line, "\t", 0, KEEP_EMPTY_TOKENS);
        size_t fname = 0;

        if (!TColumnMetaData::Column(fname, FileNameColumn, vs.size()))
            continue;

        File2MetaData[vs[fname]] = line;
        FileList.push_back(vs[fname]);

        {
            size_t urlcol = 0;
            if (!TColumnMetaData::Column(urlcol, MetaData.Url.Col, vs.size()))
                ythrow TDataException() << "no url in " << urlcol << " column of metadata '" << line << "'";

            Url2File[AddSchemePrefix(vs[urlcol])] = vs[fname];
        }
    }
}

TString THtmlFileReader::GetUrl(const TString& fname) const {
    THtmlFile f = Read(fname, false);
    return f.Url;
}

THtmlFile THtmlFileReader::Read(const TString& fname, bool readbody) const {
    THtmlFile f;
    f.FileName = CommonPrefix + fname;

    THolder<IInputStream> is0;
    THolder<IInputStream> is;

    if (readbody || MDM_FirstLine == MetaMode) {
        if (fname.EndsWith(".gz")) {
            is0.Reset(new TUnbufferedFileInput(f.FileName));
            is.Reset(new TZLibDecompress(is0.Get()));
        } else {
            is.Reset(new TUnbufferedFileInput(f.FileName));
        }
    }

    f.Time.Timestamp = 0;
    if (MetaMode != MDM_None) {
        TString meta;

        if (MDM_Mapping == MetaMode) {
            TMapping::const_iterator it = File2MetaData.find(fname);

             if (it == File2MetaData.end())
                ythrow TDataException() << "no metadata for file " << fname;

             meta = it->second;
        } else if (MDM_FirstLine == MetaMode) {
            meta = is->ReadLine();
        }

        MetaData.Process(meta, f);
    }

    f.Url = AddSchemePrefix(f.Url);
    if (!f.Time.Timestamp)
        f.Time = CommonTime;

    if (readbody)
        f.Html = is->ReadAll();

    return f;
}

void Fetch(THtmlDocument& doc, time_t tout, size_t maxsize, bool onlyhtml) {
    NHttpFetcher::TRequestRef req = new NHttpFetcher::TRequest(doc.Url.data(), true, TDuration::Seconds(tout), NHttpFetcher::DEFAULT_REQUEST_FRESHNESS);
    req->UserAgent = "Mozilla/5.0 Gecko/20110303 Firefox/3.6.15";
    NHttpFetcher::TResultRef fetchResult = NHttpFetcher::FetchNowWithRedirects(req);

    if (!fetchResult->Success())
        ythrow TDataException() << ExtHttpCodeStr(fetchResult->Code);
    MimeTypes mime = mimeByStr(fetchResult->MimeType);
    if (onlyhtml && mime != MIME_HTML)
        ythrow TDataException() << "Not a text/html http response (" << fetchResult->MimeType << ")";

    doc.Html = fetchResult->DecodeData();
    if (doc.Html.size() > maxsize)
        doc.Html = doc.Html.substr(0, maxsize);
    doc.HttpMime = mime;
    if (fetchResult->Encoding.Defined())
        doc.HttpCharset = fetchResult->Encoding.GetRef();
    if (!doc.Time.Timestamp)
        doc.Time.SetTimestamp(time(nullptr));
}

}
