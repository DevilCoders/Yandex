#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <util/stream/input.h>
#include <util/stream/buffered.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

static const int MIN_URLS_PER_GROUP = 2;

int main(void)
{
    THashMap<TString, int> naiveGroups;
    NHtmlStats::TUrlGrouper grouper;
    TBufferedInput inf(&Cin, 1<<24);

    TString rec;
    while (inf.ReadLine(rec)) {
        if (!rec) {
            continue;
        }
        TString url;
        TString shard;
        Split(rec, '\t', url, shard);

        TVector<TStringBuf> groups = grouper.GetAllGroupNames(url);
        for (const auto& groupName : groups) {
            TString key = TString{groupName};
            naiveGroups[key]++;
        }
    }

    for (const auto& group : naiveGroups)
    {
        const TString& url = group.first;
        int count = group.second;
        if (count >= MIN_URLS_PER_GROUP) {
            Cout << url << '\t' << count << Endl;
        }
    }
}
