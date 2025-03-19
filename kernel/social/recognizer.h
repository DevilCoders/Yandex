#pragma once

#include <kernel/social/protos/socnets.pb.h>

#include <util/generic/string.h>

namespace NSocial {

enum EPageParser {
    PP_NONE,
    PP_FACEBOOK,
    PP_FOURSQUARE,
    PP_FREELANCE,
    PP_FRIENDFEED,
    PP_GPLUS,
    PP_INSTAGRAM,
    PP_LINKEDIN,
    PP_LIVEJOURNAL,
    PP_MIRTESEN,
    PP_MOIKRUG,
    PP_MOIMIR,
    PP_ODNOKLASSNIKI,
    PP_TWITTER,
    PP_VK,
    PP_VK_FOAF,
    PP_YARU_FOAF,
};

struct TIdentityPair {
    ESocialNetwork Network;
    TString Id;
    EPageParser Parser;

    TIdentityPair(ESocialNetwork network,
                  TString id,
                  EPageParser parser = PP_NONE)
        : Network(network)
        , Id(id)
        , Parser(parser)
    {
    }

    TIdentityPair()
        : Network(UnknownNetwork)
        , Id()
        , Parser(PP_NONE)
    {
    }

    bool IsNarrow() {
        return Parser != PP_NONE;
    }

    bool ValidateProfileId() const;
};

bool ValidateProfileId(const TString& id, NSocial::ESocialNetwork network);
bool ExtractIdent(TStringBuf url, TIdentityPair& result);
bool IsSocialProfileUrl(TStringBuf url);

}
