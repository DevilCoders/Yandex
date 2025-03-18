#include <kernel/hosts/owner/owner.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/cast.h>
#include <util/stream/output.h>
#include <tools/segutils/segcommon/qutils.h>

int main(int argc, const char** argv) {
    if (argc > 1 && !strcmp(argv[1], "--help") || argc < 4) {
        Clog << "usage: " << argv[0] << " <url rules dir> <urls column in input> <owners column in output>" << Endl;
        exit(0);
    }

    TOwnerCanonizer canonizer = NSegutils::CreateAndInitCanonizer(argv[1]);

    const ui32 urlscolumn = FromString<ui32>(argv[2]) - 1;
    const ui32 hostscolumn = FromString<ui32>(argv[3]) - 1;
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

    TString line;
    TString url;

    while (Cin.ReadLine(line)) {
        TStringBuf l(line);
        url.clear();

        for (ui32 i = 0; i < urlscolumn; ++i)
            l.NextTok('\t');

        url.assign(l.NextTok('\t'));

        if (!url)
            continue;

        if (!url.StartsWith("http://") && !url.StartsWith("https://")) {
            url.prepend("http://");
        }

        TStringBuf host = canonizer.GetUrlOwner(url);

        TStringBuf ll(line);
        for (ui32 i = 0; i < hostscolumn; ++i)
            Cout << ll.NextTok('\t') << '\t';

        Cout << host;

        if (!ll)
            Cout << '\n';
        else
            Cout << '\t' << ll << '\n';
    }
}
