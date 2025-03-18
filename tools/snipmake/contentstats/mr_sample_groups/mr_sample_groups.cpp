#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <util/stream/input.h>
#include <util/stream/buffered.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

static const i64 MAX_JOB_GROUP_SIZE = 200;
static const i64 MAX_GROUP_SIZE = 600;

void DoMap()
{
    NHtmlStats::TUrlGrouper grouper;
    THashMap<TString, i64> groupSizes;
    THashSet<size_t> seenUrls;
    grouper.LoadGroups("groups.txt");

    TBufferedInput inf(&Cin, 1<<24);

    TString rec;
    while (inf.ReadLine(rec)) {
        if (!rec) {
            continue;
        }

        TString url;
        TString subkey;
        TString shard;
        Split(rec, '\t', url, subkey, shard);

        TString groupName = grouper.GetGroupName(url, true);
        if (!groupName) {
            continue;
        }

        i64 gs = groupSizes[groupName];
        if (gs >= MAX_JOB_GROUP_SIZE) {
            continue;
        }

        size_t hsh = ComputeHash(url);
        if (seenUrls.contains(hsh)) {
            continue;
        }
        seenUrls.insert(hsh);

        groupSizes[groupName]++;

        Cout << groupName << "\t" << ComputeHash(url) << "\t" << url << Endl;
    }
}

void DoReduce()
{
    TString prevKey;
    THashSet<TString> seenUrls;
    i64 count = 0;

    TString rec;
    TBufferedInput inf(&Cin, 1<<24);
    while (inf.ReadLine(rec)) {
        if (!rec) {
            continue;
        }

        TString group;
        TString url;
        Split(rec, '\t', group, url);

        if (group != prevKey) {
            prevKey = group;
            seenUrls.clear();
            count = 0;
        }

        if (seenUrls.contains(url) || count > MAX_GROUP_SIZE) {
            continue;
        }
        seenUrls.insert(url);
        ++count;
        Cout << group << "\t" << url << Endl;
    }
}

int main(int argc, const char* argv[])
{
    if (argc != 2) {
        return 1;
    }
    else if (!strcmp(argv[1], "map")) {
        DoMap();
    }
    else if (!strcmp(argv[1], "reduce")) {
        DoReduce();
    }
}

