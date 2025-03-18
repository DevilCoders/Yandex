#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/generic/set.h>
#include <util/string/vector.h>
#include <util/string/split.h>

struct TWhiteListEntry {
    TString Url;
    TString Domain;
    TWhiteListEntry(const TString& url, const TString& domain)
        : Url(url)
        , Domain(domain)
    {
    }

    bool operator<(const TWhiteListEntry& entry) const {
        if (Url != entry.Url)
            return Url < entry.Url;
        return Domain < entry.Domain;
    }
};

void ReadWhiteList(const TString& whiteListFile, TSet<TWhiteListEntry>& whiteList) {
    TFileInput in(whiteListFile);
    TString line;
    while (in.ReadLine(line)) {
        TVector<TString> v;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&v);
        if (v.size() != 2) {
            Cerr << "Wrong line format in <white-list> file!" << Endl;
            continue;
        }
        const TString& url = v[0];
        const TString& domain = v[1];
        whiteList.insert(TWhiteListEntry(url, domain));
    }
}

void GetData(const TString& line, TString& url, TString& domain, TString& json) {
    url = domain = json = TString();

    TVector<TString> v;
    StringSplitter(line).Split('\t').SkipEmpty().Collect(&v);
    if (v.size() != 4) {
        Cerr << "Wrong line format in <bno> file!" << Endl;
        return;
    }
    static const TString MIDDLE_BNO_PREF = "middle_bno=";
    if (!v[2].StartsWith(MIDDLE_BNO_PREF)) {
        Cerr << "No " << MIDDLE_BNO_PREF << " attr in data!" << Endl;
        return;
    }

    url = v[0];
    domain = v[1];
    json = v[2].substr(MIDDLE_BNO_PREF.size());
}

void AddBnoAttr(const TString& bnoFile, const TString& whiteListFile) {
    TSet<TWhiteListEntry> whiteList;
    ReadWhiteList(whiteListFile, whiteList);

    TFileInput in(bnoFile);
    TString line;
    while (in.ReadLine(line)) {
        Cout << line;
        TString url, domain, json;
        GetData(line, url, domain, json);
        if (line && url && json && whiteList.contains(TWhiteListEntry(url, domain))) {
            Cout << '\t' << "bno=" << json;
        }
        Cout << Endl;
    }
}

int main(int argc, const char* argv[]) {
    TString bnoFile;
    TString whiteListFile;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('b', "bno", "data for bno trie")
        .Required()
        .StoreResult(&bnoFile);
    opts.AddLongOption('w', "white-list", "list of canonized URLs")
        .Required()
        .StoreResult(&whiteListFile);
    NLastGetopt::TOptsParseResult o(&opts, argc, argv);

    AddBnoAttr(bnoFile, whiteListFile);

    return 0;
}

