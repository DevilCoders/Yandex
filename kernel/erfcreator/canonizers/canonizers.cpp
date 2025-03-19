#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/folder/dirut.h>
#include "canonizers.h"


void TCanonizers::InitCanonizers(const THostCanonizerPtr mirrorCanonizer, const THostCanonizerPtr ownerCanonizer) {
    THostCanonizerPtr reverseDomainCanonizer = new TReverseDomainCanonizer();
    THostCanonizerPtr punycodeCanonizer = new TPunycodeHostCanonizer();
    THostCanonizerPtr lowerCaseCanonizer = new TLowerCaseHostCanonizer();
    THostCanonizerPtr stripWWWCanonizer = new TStripWWWHostCanonizer();

    TSimpleSharedPtr<TCombinedHostCanonizer> canonizer(new TCombinedHostCanonizer());

    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    if (!!mirrorCanonizer)                          // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(mirrorCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    MirrorCanonizer = canonizer;
    MirrorCachedCanonizer = new TLastCachedHostCanonizer(MirrorCanonizer.Get());

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    if (!!mirrorCanonizer)                          // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(mirrorCanonizer);
    canonizer->AddCanonizer(stripWWWCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    MirrorWWWCanonizer = canonizer;
    MirrorWWWCachedCanonizer = new TLastCachedHostCanonizer(MirrorWWWCanonizer.Get());

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    canonizer->AddCanonizer(ownerCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    OwnerCanonizer = canonizer;
    OwnerCachedCanonizer = new TLastCachedHostCanonizer(OwnerCanonizer.Get());

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    if (!!mirrorCanonizer)                          // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(mirrorCanonizer);
    canonizer->AddCanonizer(ownerCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    MirrorOwnerCanonizer = canonizer;
    MirrorOwnerCachedCanonizer = new TLastCachedHostCanonizer(MirrorOwnerCanonizer.Get());

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    canonizer->AddCanonizer(ownerCanonizer);
    if (!!mirrorCanonizer)                          // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(mirrorCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    OwnerMirrorCanonizer = canonizer;
    OwnerMirrorCachedCanonizer = new TLastCachedHostCanonizer(OwnerMirrorCanonizer.Get());

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(stripWWWCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    StripWWWCanonizer = canonizer;

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    if (!!mirrorCanonizer)                          // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(mirrorCanonizer);
    canonizer->AddCanonizer(ownerCanonizer);
    canonizer->AddCanonizer(stripWWWCanonizer);
    canonizer->AddCanonizer(reverseDomainCanonizer);
    MirrorOwnerWWWCanonizer = canonizer;

    canonizer = new TCombinedHostCanonizer();
    canonizer->AddCanonizer(lowerCaseCanonizer);
    canonizer->AddCanonizer(punycodeCanonizer);
    canonizer->AddCanonizer(new TLastCachedHostCanonizer(ownerCanonizer.Get()));
    if (!!mirrorCanonizer) {
        // do not use mirror canonizer in some cases
        canonizer->AddCanonizer(new TLastCachedHostCanonizer(mirrorCanonizer.Get()));
        canonizer->AddCanonizer(new TLastCachedHostCanonizer(ownerCanonizer.Get()));
    }
    MainPageCanonizer = canonizer;

    ReverseDomainCanonizer = new TCombinedHostCanonizer(reverseDomainCanonizer);
    TrivialCanonizer = new TCombinedHostCanonizer(new TTrivialHostCanonizer());
}

THostCanonizerPtr GetMirrorResolver(const TErfCreateConfigCommon& config, EPrechargeMode prechargeMode) {
    if (config.UseMappedMirrors) {
        return new TMirrorsMappedTrieCanonizer(config.Mirrors + "/mirrors.trie", prechargeMode);
    } else if (config.UseLocalMirrors) {
        return new TLocalMirrorsCanonizer(config.Mirrors + "/localmirrorsfull.txt");
    } else {
        return new TMirrorHostCanonizer(config.Mirrors + "/mirrors.res");
    }
}

THostCanonizerPtr GetOwnerResolver(const TErfCreateConfigCommon& config) {
    TString areas = config.Catalog + "/j-owners-spm.lst";
    if (!NFs::Exists(areas))
        areas = config.Areas;
    return new TOwnerHostCanonizer(areas.data());
}
