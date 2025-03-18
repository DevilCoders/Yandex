#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/snippets/urlcut/urlcut.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <util/generic/strbuf.h>
#include <util/stream/zlib.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/cast.h>
#include <util/string/strip.h>

int main(int /*argc*/, char** /*argv[]*/) {
    TString line;

    TZLibDecompress source(&Cin, ZLib::GZip, 1024 * 1024);

    while (source.ReadLine(line)) {
        StripInPlace(line);

        if (line.empty())
            continue;

        TStringBuf url, query;
        TStringBuf(line).Split('\t', url, query);
        TCgiParameters pars;
        pars.Scan(query.data());

        if (!pars.NumOfValues("qtree") || !pars.NumOfValues("text"))
            continue;

        TRichTreePtr rTreePtr = DeserializeRichTree(DecodeRichTreeBase64(pars.Get("qtree")));
        NUrlCutter::TRichTreeWanderer rTreeWanderer(rTreePtr);
        NUrlCutter::THilitedString hilitedUrl =
            NUrlCutter::HiliteAndCutUrl(ToString(url), 50, 40, rTreeWanderer);
        TUtf16String result = hilitedUrl.Merge(u"\007[", u"\007]");
        Cout << pars.Get("text") << "\t" << result << Endl;
    }

    return 0;
}

