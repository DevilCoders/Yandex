#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <util/stream/input.h>
#include <util/stream/buffered.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

static const i64 MIN_URLS_PER_GROUP = 200;

int main(void)
{
    TBufferedInput inf(&Cin, 1<<24);

    TString rec;
    TString currGroup;
    i64 currCount = 0;
    while (inf.ReadLine(rec)) {
        if (!rec) {
            continue;
        }
        TString group;
        i64 count;
        Split(rec, '\t', group, count);
        if (group != currGroup) {
            if (currCount >= MIN_URLS_PER_GROUP) {
                Cout << currGroup << "\t" << currCount << Endl;
            }
            currCount = 0;
            currGroup = group;
        }
        currCount += count;
    }

    if (currCount >= MIN_URLS_PER_GROUP) {
        Cout << currGroup << "\t" << currCount << Endl;
    }
}

