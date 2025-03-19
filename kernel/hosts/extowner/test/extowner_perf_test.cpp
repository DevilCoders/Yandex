#include <kernel/hosts/extowner/ext_owner.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/string/vector.h>
#include <util/stream/file.h>

TVector<TString> GetAreasList(const TString& urlsFile) {
    TFileInput input(urlsFile);
    TString line;
    TVector<TString> areas;

    while (input.ReadLine(line)) {
        areas.push_back(line);
    }

    return areas;
}

int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    TString areasFile;
    TString urlsFile;
    size_t repeatNumber;

    opts
        .AddLongOption('a', "areas", "Areas file")
        .StoreResult(&areasFile)
        .RequiredArgument("Areas file");

    opts
        .AddLongOption('u', "urls", "Urls file")
        .StoreResult(&urlsFile)
        .RequiredArgument("Urls file");

    opts
        .AddLongOption('r', "repeat", "Repeat number")
        .StoreResult(&repeatNumber)
        .RequiredArgument("Repeat number");

    NLastGetopt::TOptsParseResult parsing{&opts, argc, argv};

    TOwnerExtractor ownerExtractor(areasFile);
    const TVector<TString> urls = GetAreasList(urlsFile);

    for (size_t i = 0; i < repeatNumber; ++i) {
        for (const TString& url : urls) {
            ownerExtractor.GetOwner(url);
        }
    }

    return 0;
}
