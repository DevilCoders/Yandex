#include <kernel/hosts/owner/owner.h>
#include <kernel/segmentator/main_content_impl.h>
#include <kernel/segmentator/main_header_impl.h>
#include <kernel/segutils/numerator_utils.h>

#include <tools/segutils/quality_measurers/segqualitycommon/qualitycommon.h>
#include <tools/segutils/segcommon/data_utils.h>
#include <tools/segutils/segcommon/qutils.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/html/entity/htmlentity.h>

#include <util/generic/algorithm.h>
#include <util/random/random.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSegutils {
using namespace NSegm::NPrivate;
using namespace NSegm;

typedef THashMap<TString, THashMap<TString, TVector<TUtf16String> > > TGroupXPath;
struct TFeaturesProcessor : public TDocProcessor {
    TString Url;
    THashMap<TString, TVector<TUtf16String> > Groups;

    TFeaturesProcessor(const TOpts& opts, const TString& url)
        : TDocProcessor(opts, url)
        , Url(url)
    {
    }

    bool Postprocess() override {
        // Please apply segmentator.patch to get Handler.GetGroups()
        THashMap<TString, TVector<TUtf16String> > groups; // = Handler.GetGroups();
        for (THashMap<TString, TVector<TUtf16String> >::const_iterator it = groups.begin(); it != groups.end(); ++it)
        {
            if (it->second.size() == 0)
                continue;
            for (TVector<TUtf16String>::const_iterator strIt = it->second.begin(); strIt != it->second.end(); ++strIt)
            {
                if (!strIt->empty())
                    Groups[it->first].push_back(*strIt);
            }
        }
        return true;
    }
};

}

int main(int argc, const char** argv) {
    using namespace NSegutils;
    TOpts opts;
    TOwnerCanonizer canonizer(NSegutils::CreateAndInitCanonizer("../../../../yweb/urlrules/"));
    opts.ProcessOpts(argc, argv);

    TGroupXPath all_groups;
    for (TMapping::const_iterator it = opts.GetReader().Url2File.begin(); it != opts.GetReader().Url2File.end(); ++it) {
        try {
            TString url = AddSchemePrefix(it->first);
            TString host(canonizer.GetUrlOwner(url));

            TFeaturesProcessor proc(opts, it->first);
            if (!proc.ProcessLive())
                continue;
            proc.Postprocess();
            THashMap<TString, TVector<TUtf16String> > groups(proc.Groups);
            for (THashMap<TString, TVector<TUtf16String> >::const_iterator it = groups.begin(); it != groups.end(); ++it)
            {
                if (it->second.size() == 0)
                    continue;
                for (TVector<TUtf16String>::const_iterator strIt = it->second.begin(); strIt != it->second.end(); ++strIt)
                {
                    if (!strIt->empty())
                        all_groups[host][it->first].push_back(*strIt);
                }
            }
        } catch(...) {}
    }

    for (TGroupXPath::const_iterator it = all_groups.begin(); it != all_groups.end(); ++it)
    {
        const THashMap<TString, TVector<TUtf16String> >& group = it->second;
        for (THashMap<TString, TVector<TUtf16String> >::const_iterator it_g = group.begin(); it_g != group.end(); ++it_g)
        {
            TSet<TUtf16String> set_w(it_g->second.begin(), it_g->second.end());
            if (1.0 * set_w.size() / it_g->second.size() < 0.05) {
                Cout << it->first << "\t" << it_g->first << "\t" << Endl;
                // If you need to print number of xpath and unique words
                Cout << it->first << "\t" << it_g->first << "\t" << it_g->second.size() << "\t" << set_w.size() << Endl;

                // If you need to print words

                for (TSet<TUtf16String>::const_iterator tt = set_w.begin(); tt != set_w.end(); ++tt) {
                    Cout <<  *tt << Endl;
                }

            }
        }
    }
}
