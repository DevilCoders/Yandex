#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/unpackers.h>
#include <kernel/tarc/markup_zones/view_sent.h>

#include <util/string/cast.h>

void PrintDoc(IOutputStream& os, TArchiveIterator& arcIter, TArchiveHeader* curHdr)
{
    TBlob extInfo = arcIter.GetExtInfo(curHdr);
    TBlob docText = arcIter.GetDocText(curHdr);

    TDocDescr docDescr;
    docDescr.UseBlob(extInfo.Data(), (unsigned int)extInfo.Size());
    if (!docDescr.IsAvailable()) {
        return;
    }

    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &mZones);
    TArchiveZone& z = mZones.GetZone(AZ_MAIN_CONTENT);
    TString mainContent;
    if (!z.Spans.empty()) {
        TVector<int> sentNumbers;
        for (TVector<TArchiveZoneSpan>::const_iterator i = z.Spans.begin(); i != z.Spans.end(); ++i) {
            for (ui16 n = i->SentBeg; n <= i->SentEnd; ++n) {
                sentNumbers.push_back(n);
            }
        }
        TVector<TArchiveSent> outSents;
        GetSentencesByNumbers((const ui8*)docText.Data(), sentNumbers, &outSents, nullptr, false);

        TSentReader sentReader;
        for (TVector<TArchiveSent>::iterator i = outSents.begin(); i != outSents.end(); ++i) {
            const TString sent = WideToUTF8(sentReader.GetText(*i, mZones.GetSegVersion()));
            if (!sent) {
                continue;
            }
            if (mainContent.length()) {
                mainContent.append('\r');
            }
            mainContent.append(Sprintf("%hu ", i->Number));
            mainContent.append(sent);
        }
    }

    os << docDescr.get_url() << "\t" << mainContent << '\n';
}

void PrintArchive(IOutputStream& os, TArchiveIterator& arcIter)
{
    TArchiveHeader* curHdr = arcIter.NextAuto();
    while (curHdr) {
        PrintDoc(os, arcIter, curHdr);
        curHdr = arcIter.NextAuto();
    }
}

void PrintUsage()
{
    Cerr << "Usage: printmaincontent <archive-file-name>" << Endl;
}

int main_except_wrap (int argc, char* argv[])
{
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    TString arcName(argv[1]);

    TArchiveIterator arcIter(32 << 20);
    arcIter.Open(arcName.data());
    PrintArchive(Cout, arcIter);
    return 0;
}

int main(int argc, char *argv[]) {
   try {
      return main_except_wrap(argc, argv);
   } catch (std::exception& e) {
      Cerr << "Error: " << e.what() << Endl;
      return 1;
   }
}
