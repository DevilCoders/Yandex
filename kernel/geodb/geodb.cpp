#include "geodb.h"

#include <kernel/search_types/search_types.h>

#include <library/cpp/streams/factory/factory.h>

#include <google/protobuf/messagext.h>

#include <util/digest/numeric.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/system/compiler.h>

namespace NGeoDB {
    static const TRegionProto INVALID_REGION_PROTO;
    static const ui32 VERSION = 2;

    void SaveRegionProto(IOutputStream* output, const NGeoDB::TRegionProto& msg) {
        NProtoBuf::io::TProtoSerializer::Save(output, msg);
    }

    void LoadRegionProto(IInputStream* input, NGeoDB::TRegionProto& msg) {
        NProtoBuf::io::TProtoSerializer::Load(input, msg);
    }

    TGeoPtr::TGeoPtr(const TRegionProto& proto, const TGeoKeeper* geoKeeper)
        : Proto_(&proto)
        , GeoKeeper_(geoKeeper)
    {
        Y_ASSERT(geoKeeper != nullptr);
    }

    TGeoPtr::TGeoPtr()
        : Proto_(&INVALID_REGION_PROTO)
        , GeoKeeper_(nullptr)
    {
    }

    // XXX may be create a cache for path parents
    bool TGeoPtr::IsIn(const TGeoPtr& probableParent) const {
        if (probableParent->GetId() == Get()->GetId())
            return true;

        for (const auto id : Get()->GetPath()) {
            if (probableParent->GetId() == id)
                return true;
        }

        return false;
    }

    bool TGeoPtr::IsIn(TCateg probableParentId) const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return false;
        }

        return IsIn(GeoKeeper_->Find(probableParentId));
    }

    TGeoPtr TGeoPtr::Parent() const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return TGeoPtr();
        }

        return GeoKeeper_->Find(Get()->GetParentId());
    }

    TGeoPtr TGeoPtr::ParentBy(TCheckRegionFunc predicate) const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return TGeoPtr();
        }

        const auto* const proto = predicate(Proto_);
        if (Y_UNLIKELY(proto)) {
            return TGeoPtr();
        }

        for (const auto pathId : Get()->GetPath()) {
            const TGeoPtr pathObj = GeoKeeper_->Find(pathId);
            if (Y_UNLIKELY(!pathObj)) {
                continue;
            }

            const auto* const obj = predicate(pathObj.Proto_);
            if (obj) {
                return TGeoPtr(*obj, GeoKeeper_);
            }
        }

        return TGeoPtr();
    }

    TGeoPtr TGeoPtr::ParentByType(EType type) const {
        if (Y_UNLIKELY(END_CATEG == Get()->GetId())) {
            return TGeoPtr();
        }

        return ParentBy(
            [type](const TRegionProto* region) -> const TRegionProto* {
                return region->GetType() == type ? region : nullptr;
            }
        );
    }

    TGeoPtr TGeoPtr::Country() const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return TGeoPtr();
        }

        return GeoKeeper_->Find(Get()->GetCountryId());
    }

    TGeoPtr TGeoPtr::CountryCapital() const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return TGeoPtr();
        }

        return GeoKeeper_->Find(Get()->GetCountryCapitalId());
    }

    TGeoPtr TGeoPtr::Chief() const {
        if (Y_UNLIKELY(!GeoKeeper_)) {
            return TGeoPtr();
        }

        return GeoKeeper_->Find(Get()->GetChiefId());
    }

    const TRegionProto::TNames* TGeoPtr::FindNames(ELanguage langId) const {
        for (const auto& names : Get()->GetNames())
            if (names.GetLang() == langId)
                return &names;

        return nullptr;
    }

    const TRegionProto::TNames* TGeoPtr::FindNames(const TStringBuf& lang) const {
        return FindNames(LanguageByName(lang));
    }

    template <typename T>
    static bool Disamb(const TGeoKeeper& geodb, const TGeoPtr& region, const TGeoPtr& userRegion, TVector<T>& disambiguated) {
        if (!userRegion || !region->AmbiguousParentsSize())
            return false;

        const size_t oldSize = disambiguated.size();
        const auto id = userRegion->GetId();
        const size_t parentSize = region->AmbiguousParentsSize();
        for (size_t i = 0; i < parentSize; ++i) {
            TGeoPtr parent = geodb.Find(region->GetAmbiguousParents(i));
            if (parent.IsIn(id))
                continue;

            disambiguated.push_back(static_cast<T>(parent));
        }

        return oldSize != disambiguated.size();
    }

    bool TGeoPtr::Disambiguate(const TGeoPtr& userRegion, TVector<TGeoPtr>& disambiguated) const {
        return Disamb<TGeoPtr>(*GeoKeeper_, *this, userRegion, disambiguated);
    }

    bool TGeoPtr::Disambiguate(const TGeoPtr& userRegion, TVector<TCateg>& disambiguated) const {
        return Disamb<TCateg>(*GeoKeeper_, *this, userRegion, disambiguated);
    }

    TGeoKeeper::TGeoKeeper()
        : Index_(END_CATEG)
        , Version_(VERSION)
    {
    }

    void TGeoKeeper::Load(IInputStream* s) {
        ::Load<ui32>(s, Version_);
        if (Version_ != VERSION) {
            ythrow TInvalidVersion() << "Expected version " << VERSION << ", got " << Version_;
        }

        ui32 size = 0;
        ui32 hash = 0;
        ::Load<ui32>(s, size);
        ::Load<ui32>(s, hash);
        if (hash != IntHash<ui32>(size)) {
            ythrow TError() << "Failed to load data: incompatible format or corrupted binary";
        }

        ::Load(s, Index_);
        if (size != Index_.Size()) {
            ythrow TError() << "Loaded size is different from the header size. Expected "
                            << size << ", got " << Index_.Size();
        }
    }

    void TGeoKeeper::Save(IOutputStream* s) const {
        ::Save<ui32>(s, VERSION);
        const size_t size = Index_.Size();
        if (static_cast<ui64>(size) > static_cast<ui64>(Max<ui32>())) {
            ythrow TError() << "Size " << size << " exceeds ui32 limits.";
        }
        ::Save<ui32>(s, size);
        ::Save<ui32>(s, IntHash<ui32>(size));
        ::Save(s, Index_);
    }

    TGeoKeeperHolder TGeoKeeper::LoadToHolder(IInputStream& input) {
        auto geodb = MakeHolder<TGeoKeeper>();
        geodb->Load(&input);
        return geodb;
    }

    TGeoPtr TGeoKeeper::Find(TCateg id) const {
        const auto* const regionProto = Index_.FindPtr(id);
        if (Y_UNLIKELY(!regionProto)) {
            return TGeoPtr{};
        }

        return TGeoPtr(*regionProto, this);
    }

    TGeoKeeperHolder TGeoKeeper::LoadToHolder(const TString& path) {
        const auto input = OpenInput(path);
        return LoadToHolder(*input);
    }
} // namespace NGeoDB

/* for debugging purposes */
template <>
void Out<NGeoDB::TGeoPtr>(IOutputStream& s, const NGeoDB::TGeoPtr& fp) {
    s << fp->AsJSON();
}
