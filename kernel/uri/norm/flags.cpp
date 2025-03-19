/*
 *  Created on: Dec 6, 2011
 *      Author: albert@
 *
 * $Id$
 */



#include "flags.h"

namespace Nydx {
namespace NUriNorm {

void TFlags::ParseFlagsImpl() const
{
    ParseAllow_ = 0;
    ParseExtra_ = 0;

    if (!GetUseRFC())
        ParseAllow_ |= ::NUri::TUri::FeatureNoRelPath;

    if (GetAllowRootless())
        ParseAllow_ |= ::NUri::TUri::FeatureAllowOpaque;

    if (GetParseRobot())
        ParseAllow_ |= ::NUri::TUri::FeaturesRobot;
    else if (GetParseBare())
        ParseAllow_ |= ::NUri::TUri::FeaturesBare;
    else
        ParseAllow_ |= ::NUri::TUri::FeaturesAll;

    if (!GetNormalizeURLs())
        return;

    if (GetLowercaseURL()) {
        ParseAllow_ |= ::NUri::TUri::FeatureToLower;
        ParseExtra_ |= ::NUri::TUri::FeatureToLower;
    }
    else if (GetLowercaseHost())
        ParseAllow_ |= ::NUri::TUri::FeatureToLower;
    else
        ParseAllow_ &= ~::NUri::TUri::FeatureToLower;

    if (GetEncodeSpcAsPlus()) {
        ParseAllow_ |= ::NUri::TUri::FeatureEncodeSpaceAsPlus;
        ParseExtra_ |= ::NUri::TUri::FeatureEncodeSpaceAsPlus;
    }

    if (GetEncodeExtended())
        ParseAllow_ |= ::NUri::TUri::FeaturesEncodeExtended;

    if (GetHostAllowIDN())
        ParseAllow_ |= ::NUri::TUri::FeatureConvertHostIDN;

    if (GetQryEscFrag())
        ParseAllow_ |= ::NUri::TUri::FeatureHashBangToEscapedFragment;
    else if (GetQryUnescFrag())
        ParseAllow_ |= ::NUri::TUri::FeatureEscapedToHashBangFragment;
}

}
}
