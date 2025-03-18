#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <util/stream/input.h>
#include <util/random/mersenne.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/split.h>

static const size_t GROUPS_TO_CONSIDER = 1200;
static const size_t GROUPS_TO_CHOOSE = 100;
static const int URLS_PER_GROUP = 600;

class TUrlList
{

public:
    int UniqueDocs;
    int GroupShows;
    TString Group;
    TVector<TString> Sample;

    TUrlList(const TString& groupName)
        : UniqueDocs(0)
        , GroupShows(0)
        , Group(groupName)
    {
        Sample.reserve(URLS_PER_GROUP);
    }

    void InsertSample(const TString& url, TMersenne<ui32>& random, int count)
    {
        GroupShows += count;
        UniqueDocs++;
        if (Sample.size() < URLS_PER_GROUP) {
            Sample.push_back(url);
            return;
        }

        ui32 k = random.Uniform(static_cast<ui32>(UniqueDocs));

        if (k < URLS_PER_GROUP) {
            Sample[k] = url;
        }
    }
};

using TUrlListPtr = TSimpleSharedPtr<TUrlList>;
using TUrlGroups = THashMap<TString, TUrlListPtr>;


int main(void)
{
    TMersenne<ui32> random(1414515);

    TUrlGroups groups;
    TVector<TUrlListPtr> knownGroups;
    TVector<std::pair<TString, int>> urlWithCounts;

    NHtmlStats::TUrlGrouper grouper;
    grouper.LoadGroups("mined_groups.txt");

    TSetDelimiter<const char> delim("\t ");
    TString line;
    while (Cin.ReadLine(line)) {
        if (!line) {
            continue;
        }
        TStringBuf lineBuf(line);
        TVector<TStringBuf> fields;

        if (Split(lineBuf, delim, fields) != 2) {
            ythrow yexception() << "Bad input format (expecting 2 fields: count url, found " << fields.size() <<  ")";
        }

        int count = FromString<int>(fields.at(0));
        TString url = TString{fields.at(1)};
        urlWithCounts.push_back(std::make_pair(url, count));
    }

    for (const auto& urlWithCount : urlWithCounts)
    {
        const TString& url = urlWithCount.first;
        int count = urlWithCount.second;

        if (count <= 0) {
            ythrow yexception() << "Count must be > 0";
        }

        TString group = grouper.GetGroupName(url);
        if (!group) {
            continue;
        }
        TUrlListPtr& urlList = groups[group];
        if (!urlList) {
            urlList.Reset(new TUrlList(group));
            knownGroups.push_back(urlList);
        }
        urlList->InsertSample(url, random, count);
    }

    decltype(knownGroups) filteredGroups;
    for (const auto& groupPtr : knownGroups) {
        if (groupPtr->UniqueDocs >= URLS_PER_GROUP) {
            filteredGroups.push_back(groupPtr);
        }
    }
    knownGroups.swap(filteredGroups);
    filteredGroups.clear();

    Sort(knownGroups.begin(), knownGroups.end(), [](const TUrlListPtr& left, const TUrlListPtr& right) {
        return left->GroupShows > right->GroupShows;
    });

    if (knownGroups.size() > GROUPS_TO_CONSIDER) {
        knownGroups.resize(GROUPS_TO_CONSIDER);
    }

    if (knownGroups.size() > GROUPS_TO_CHOOSE) {
        Shuffle(knownGroups.begin(), knownGroups.end(), random);
        knownGroups.resize(GROUPS_TO_CHOOSE);
    }

    for (const TUrlListPtr& urlList : knownGroups) {
        for (const TString& url : urlList->Sample) {
            TString urlWithScheme = AddSchemePrefix(url, "http");
            Cout << urlList->Group << "\t" << urlWithScheme << Endl;
        }
    }
}
