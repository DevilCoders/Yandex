#pragma once

#include <kernel/url/url_canonizer.h>
#include <kernel/erfcreator/common_config/common_config.h>


class TCanonizers {
public:
    //reverse canonizers (for trie)
    THostCanonizerPtr MirrorCachedCanonizer;
    TCombinedHostCanonizerPtr MirrorCanonizer;
    THostCanonizerPtr MirrorWWWCachedCanonizer;
    TCombinedHostCanonizerPtr MirrorWWWCanonizer;
    THostCanonizerPtr OwnerCachedCanonizer;
    TCombinedHostCanonizerPtr OwnerCanonizer;
    THostCanonizerPtr MirrorOwnerCachedCanonizer;
    TCombinedHostCanonizerPtr MirrorOwnerCanonizer;
    TCombinedHostCanonizerPtr OwnerMirrorCanonizer; // SEARCHSPAM-3938
    THostCanonizerPtr OwnerMirrorCachedCanonizer; // SEARCHSPAM-3938
    TCombinedHostCanonizerPtr StripWWWCanonizer;
    TCombinedHostCanonizerPtr MirrorOwnerWWWCanonizer;

    //normal canonizers (for other usages)
    TCombinedHostCanonizerPtr MainPageCanonizer;

    // no-op canonizer
    TCombinedHostCanonizerPtr TrivialCanonizer;

    // for realtime geo retrieving
    TCombinedHostCanonizerPtr ReverseDomainCanonizer;

    void InitCanonizers(THostCanonizerPtr mirrorCanonizer, THostCanonizerPtr ownerCanonizer);
};

THostCanonizerPtr GetMirrorResolver(const TErfCreateConfigCommon& config, EPrechargeMode = PCHM_Disable);
THostCanonizerPtr GetOwnerResolver(const TErfCreateConfigCommon& config);
