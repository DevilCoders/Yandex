#include <util/charset/wide.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/unpackers.h>

void PrintMiscInformation(TDocDescr& docDescr) {
    Cout << "HOST_ID=" << docDescr.get_hostid() << Endl;
    Cout << "URL_ID=" << docDescr.get_urlid() << Endl;
    Cout << "SIZE=" << docDescr.get_size() << Endl;
    Cout << "MTIME=" << docDescr.get_mtime() << Endl;
    Cout << "HOST=" << docDescr.get_host() << Endl;
    Cout << "URL=" << docDescr.get_url() << Endl;

    if (const char* mime_str = strByMime(docDescr.get_mimetype()))
        Cout << "MIMETYPE=" << mime_str << Endl;

    ECharset e = docDescr.get_encoding();
    if (CODES_UNKNOWN < e && e < CODES_MAX)
        Cout << "CHARSET=" << NameByCharset(e) << Endl;

    TDocInfos docInfos;
    docDescr.ConfigureDocInfos(docInfos);

    for (TDocInfos::const_iterator i = docInfos.begin(); i != docInfos.end(); ++i) {
        Cout << TString(i->first) << "=" << i->second << Endl;
    }
}

template<typename TSpan>
void OnSpan(const TSpan& s) {
    Cout << s.Begin.Sent() << ":" << s.End.Sent() << "=" << Endl;
}

template<>
void OnSpan<NSegm::TSegmentSpan> (const NSegm::TSegmentSpan& s) {
    Cout << NSegm::GetSegmentName((NSegm::ESegmentType) s.Type) << ":"
            << s.Begin.Sent() << ":" << s.End.Sent() << " ( w:" << s.Words << ", h:" << s.IsHeader << " ) =" << Endl;
}

template<typename TSpan>
void PrintSentsForSpans(const NSegm::TArchiveSents& s, const TVector<TSpan>& spans) {
    typedef TVector<TSpan> TSpans;
    NSegm::TArchiveSents::const_iterator sentit = s.begin();

    for (typename TSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
        while (sentit != s.end() && sentit->Number < it->Begin.Sent())
            ++sentit;

        OnSpan<TSpan> (*it);

        for (; sentit != s.end() && it->ContainsSent(sentit->Number); ++sentit) {
            if (IsSpace(sentit->OnlyText.c_str(), sentit->OnlyText.size()))
                continue;

            TUtf16String w = sentit->OnlyText;
            Strip(w);
            Cout << sentit->Number << "=" << WideToUTF8(w) << Endl;
        }
    }
}

int main(int argc, const char ** argv) {
    if (argc < 2 || !strcmp("--help", argv[1])) {
        Clog << "A demo application using the segment markup from a text archive. Watch the hands!"
                << Endl;
        Clog << "Usage: " << argv[0] << " <arcfile>" << Endl;
        exit(0);
    }

    TArchiveIterator it;
    it.Open(argv[1]);

    while (TArchiveHeader* h = it.NextAuto()) {
        try {
            Cout << "DOC_ID=" << h->DocId << Endl;

            TBlob extinfo = it.GetExtInfo(h);
            TDocDescr docDescr;
            docDescr.UseBlob(extinfo.Data(), extinfo.Size());

            if (!docDescr.IsAvailable())
                continue;

            PrintMiscInformation(docDescr);

            TBlob text = it.GetDocText(h);

            TArchiveMarkupZones z;
            GetArchiveMarkupZones((const ui8*) text.Data(), &z);

            NSegm::TArchiveSents s;
            GetSentencesByNumbers((const ui8*) text.Data(), TVector<int> (), &s, nullptr, 1);

            Cout << "HEADERS:" << Endl;
            PrintSentsForSpans(s, NSegm::GetHeadersFromArchive(z));
            Cout << "MAIN_HEADERS:" << Endl;
            PrintSentsForSpans(s, NSegm::GetMainHeadersFromArchive(z));
            Cout << "MAIN_CONTENTS:" << Endl;
            PrintSentsForSpans(s, NSegm::GetMainContentsFromArchive(z));
            Cout << "SEGMENTS:" << Endl;
            PrintSentsForSpans(s, NSegm::GetSegmentsFromArchive(z));
            Cout << Endl << Endl;
        } catch (...) {
            Clog << "EXCEPTION!!!" << Endl;
            exit(13);
        }
    }

    Clog << "FINISHED";
}

