#include <kernel/iznanka/rtmr_factors/indices.h>
#include <kernel/iznanka/rtmr_factors/indices.h_serialized.h>

namespace NIznanka {

TTripleIndex::TTripleIndex(ERtmrFactorIndex user, ERtmrFactorIndex reqType, ERtmrFactorIndex url) {
    Indexes.resize(GetEnumItemsCount<EFactorType>());
    Indexes[(ui8) EFactorType::User] = user;
    Indexes[(ui8) EFactorType::ReqType] = reqType;
    Indexes[(ui8) EFactorType::Url] = url;
}

ERtmrFactorIndex TTripleIndex::Get(EFactorType factorType) const {
    return Indexes[(ui8) factorType];
}

}   // namespace NIznanka
