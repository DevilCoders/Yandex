#include "serp_features.h"
#include "query_features.h"
#include <library/cpp/dot_product/dot_product.h>
#include <quality/trailer/trailer_common/normalize.h>
#include <library/cpp/string_utils/url/url.h>

namespace NUnstructuredFeatures
{
    void TSerpCalculator::TSerpStrings::Add(const TString &element) {
        Vec.push_back(element);
        if (!Positions.contains(element))
            Positions.insert(std::make_pair(element, Vec.ysize()));
    }

    TSerpCalculator::TSerpCalculator(const TCalculatorData& data, const TVector<TString>& serp, bool normalizeUrls)
        : Data(data)
    {
        size_t size = serp.size();
        for (size_t n = 0; n < size; ++n) {
            TString url = normalizeUrls?  PrepareUrl2(serp[n]) : serp[n];
            Urls.Add(url);
            Hosts.Add(TString{GetHost(url)});
        }
    }

    TSerpCalculator::TSerpCalculator(const TCalculatorData& data, const TUnstructuredSerp& serp)
        : Data(data)
    {
        size_t size = serp.GetResults().size();
        for (size_t n = 0; n < size; ++n) {
            TString url = serp.GetResults(n).GetUrl();
            // we assume that URLs are already normalized inside TUnstructuredSerp, one way or another
            Urls.Add(url);
            Hosts.Add(TString{GetHost(url)});
        }
    }

    static float DcgIntersection(const THashMap<TString, int> &v1, const THashMap<TString, int> &v2) {
        float res = 0;
        for (const auto &it1 : v1) {
            auto it2 = v2.find(it1.first);
            if (it2 != v2.end()) {
                res += 1 / it1.second;
                res += 1 / it2->second;
            }
        }
        return res;
    }

    void TSerpCalculator::BuildIntersectionFeatures(const TSerpCalculator& rhs, TVector<float>& features) const {
        features.push_back(DcgIntersection(Urls.Positions, rhs.Urls.Positions));
        features.push_back(DcgIntersection(Hosts.Positions, rhs.Hosts.Positions));
    }

    void TSerpCalculator::BuildNeocortexFeatures(const TSerpCalculator& rhs, TVector<float>& features) const {
        EnsureTtHEmbed();
        rhs.EnsureTtHEmbed();
        features.push_back(Dot(TextToHostsNeocortexEmbed, rhs.TextToHostsNeocortexEmbed));
        features.push_back(Cosine(TextToHostsNeocortexEmbed, rhs.TextToHostsNeocortexEmbed));
    }

    void TSerpCalculator::BuildNeocortexFeatures(const TQueryCalculator& rhs, TVector<float>& features) const {
        EnsureTtHEmbed();
        features.push_back(Dot(TextToHostsNeocortexEmbed, rhs.GetTextToHostsEmbed()));
    }

    void TSerpCalculator::EnsureTtHEmbed() const {
        ENSURE_DATA(Data, TextToHostsNeocortexModel);
        TextToHostsNeocortexEmbed.Assign(NNeocortex::PrepareEmbedding(*Data.TextToHostsNeocortexModel, Hosts.Vec, false, true));
    }
}
