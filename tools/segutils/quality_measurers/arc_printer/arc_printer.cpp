#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/hosts/owner/owner.h>
#include <util/folder/path.h>
#include <library/cpp/string_utils/url/url.h>

int main(int argc, const char** argv) {
    if (argc > 1 && !strcmp(argv[1], "--help") || argc < 3) {
        Clog << "usage: " << argv[0] << " <textarc> <urlrulesdir>" << Endl;
        exit(0);
    }

    TArchiveIterator arcIter;
    arcIter.Open(argv[1]);

    TFsPath urlrules(argv[2]);

    TOwnerCanonizer canonizer;
    /*
    b2ld.list - list of free hostings (-: d=0, etc.; +: sometimes different owners for PR/antispam)

    d2ld.list - list of non-free hostings or geo-hostings

    g2ld.list - list of geographical or generic domains (+: different owners for PR/antispam)
             Previous location is /hol/pagerank (aka yweb/webutil/pagerank/scripts)
             Temporary location is /Berkanavt/catalog

    r2ld.list - list of non-free geographical or generic domains


    reg-free.txt - free regional domains (other, russian). usually manual registration via e-mail
    relcom-free.txt - free regional domains from Relcom. auto registration via e-mail
    ripn-free.txt - free domain zones from RIPN. auto registration via e-mail

    freezones.lst - free ns delegation for subdomains

    nsareas.lst - domains whose subdomains are delegated and thus subject of ns scan;
             file is bigger than freezones.lst
     */
    canonizer.LoadDom2(urlrules / "2ld.list");
    canonizer.UnloadDom2(urlrules / "f2ld.list");

    for (const TArchiveHeader* curHdr = arcIter.NextAuto();
                    curHdr;
                    curHdr = arcIter.NextAuto()) {
        TBlob extInfo = arcIter.GetExtInfo(curHdr);
        TDocDescr docDescr;
        docDescr.UseBlob(extInfo.Data(), (unsigned int) extInfo.Size());

        TString url = AddSchemePrefix(docDescr.get_url());

        Cout << url << "\t" << canonizer.GetUrlOwner(url) <<
                       "\t" << arcIter.GetDocText(curHdr).Size() << Endl;
    }
}
