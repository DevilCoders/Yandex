#include "factprocessor.h"

#include <kernel/remorph/cascade/cascade.h>
#include <kernel/remorph/matcher/matcher.h>
#include <kernel/remorph/tokenlogic/tlmatcher.h>
#include <kernel/remorph/engine/char/char_engine.h>

#include <kernel/geograph/geograph.h>

#include <util/generic/hash.h>
#include <util/folder/path.h>

namespace NFact {

void TFactProcessor::Reset() {
    TFactTypeDatabase::ResetTypes();
    Gazetteers.clear();
    GeoGraphs.clear();
}

template <class TMatcher>
void TFactProcessor::InitMatcher(const TFactType& factType, const TGazetteer* gzt, TMatcher& matcher) {
    if (!factType.GetFilter().empty()) {
        matcher.SetFilter(factType.GetFilter(), gzt);
    }
    if (!factType.GetDominants().empty()) {
        if (gzt)
            matcher.GetDominantArticles().Add(*gzt, factType.GetDominants());
        else
            throw yexception() << "The 'dominants' option requires gazetteer to be specified";
    }
    matcher.SetResolveGazetteerAmbiguity(!factType.IsAmbigGazetteer());
    matcher.SetGazetteerRankMethod(factType.GetGazetteerRankMethod());
    matcher.SetResolveCascadeAmbiguity(!factType.IsAmbigCascade());
    matcher.SetCascadeRankMethod(factType.GetCascadeRankMethod());
    matcher.SetMode(factType.GetSearchMethod());
}

void TFactProcessor::Init(const TString& path, const TGazetteer* externalGzt) {
    Reset();
    Remorphs.clear();
    TokenLogics.clear();
    CharEngines.clear();

    TFactTypeDatabase::Load(path);
    if (GetFactTypes().empty()) {
        throw yexception() << "File '" << path << "' specifies no fact definition";
    }

    for (TVector<NFact::TFactTypePtr>::const_iterator i = GetFactTypes().begin(); i != GetFactTypes().end(); ++i) {
        const NFact::TFactType& factType = **i;

        TString baseDir = TFsPath(factType.GetMatcherPath()).Parent().RealPath().c_str();
        const NGzt::TGazetteer* gzt = externalGzt;
        if (nullptr == gzt && !factType.GetGazetteerPath().empty()) {
            if (Gazetteers.contains(factType.GetGazetteerPath())) {
                gzt = Gazetteers[factType.GetGazetteerPath()].Get();
            } else {
                THolder<NGzt::TGazetteer> newGazetteer = MakeHolder<TGazetteer>(factType.GetGazetteerPath(), true);
                THolder<NGeoGraph::TGeoGraph> newGeoGraph = MakeHolder<NGeoGraph::TGeoGraph>(newGazetteer.Get());
                newGazetteer->SetGeoGraph(newGeoGraph.Get());
                gzt = newGazetteer.Get();
                Gazetteers[factType.GetGazetteerPath()] = std::move(newGazetteer);
                GeoGraphs.emplace_back(std::move(newGeoGraph));
            }
            baseDir = TFsPath(factType.GetGazetteerPath()).Parent().RealPath().c_str();
        }

        const TString& matcher = factType.GetMatcherPath();
        switch (factType.GetMatcherType()) {
        case NMatcher::MT_REMORPH:
            Remorphs.push_back(std::make_pair(gzt, new NCascade::TProcessor<NReMorph::TMatcher>(matcher, gzt, baseDir)));
            InitMatcher(factType, gzt, *Remorphs.back().second);
            break;
        case NMatcher::MT_TOKENLOGIC:
            TokenLogics.push_back(std::make_pair(gzt, new NCascade::TProcessor<NTokenLogic::TMatcher>(matcher, gzt, baseDir)));
            InitMatcher(factType, gzt, *TokenLogics.back().second);
            break;
        case NMatcher::MT_CHAR:
            CharEngines.push_back(std::make_pair(gzt, new NCascade::TProcessor<NReMorph::TCharEngine>(matcher, gzt, baseDir)));
            InitMatcher(factType, gzt, *CharEngines.back().second);
            break;
        default:
            ythrow yexception() << "Unsupported matcher type";
        }
    }
}

void TFactProcessor::AddFactType(const NProtoBuf::Descriptor& descr, IInputStream& cascade, const TGazetteer* gzt) {
    HandleDescriptor(descr);
    const NFact::TFactType& factType = *GetFactTypes().back();
    switch (factType.GetMatcherType()) {
    case NMatcher::MT_REMORPH:
        Remorphs.push_back(std::make_pair(gzt, new NCascade::TProcessor<NReMorph::TMatcher>(cascade, gzt)));
        InitMatcher(factType, gzt, *Remorphs.back().second);
        break;
    case NMatcher::MT_TOKENLOGIC:
        TokenLogics.push_back(std::make_pair(gzt, new NCascade::TProcessor<NTokenLogic::TMatcher>(cascade, gzt)));
        InitMatcher(factType, gzt, *TokenLogics.back().second);
        break;
    case NMatcher::MT_CHAR:
        CharEngines.push_back(std::make_pair(gzt, new NCascade::TProcessor<NReMorph::TCharEngine>(cascade, gzt)));
        InitMatcher(factType, gzt, *CharEngines.back().second);
        break;
    default:
        ythrow yexception() << "Unsupported matcher type";
    }
}

} // NFact
