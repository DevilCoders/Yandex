#include "descriptions_to_log.h"

namespace NPersonalization {
    NProto::TUserRecordsDescription ConstructLongClicksDescription() {
        NPersonalization::NProto::TUserRecordsDescription result;
        result.SetMaxRecords(50);
        result.MutableOptions()->SetModel(NPersonalization::NProto::LogDwelltimeBigrams);
        result.MutableOptions()->SetDwelltimeThreshold(120);
        result.MutableOptions()->SetLessThanThreshold(false);
        result.MutableOptions()->SetEmbeddingType(NPersonalization::NProto::TitleEmbedding);
        result.MutableStoreOptions()->SetStoreUrl(false);
        result.MutableStoreOptions()->SetStoreTitle(false);
        result.MutableStoreOptions()->SetStoreRequestTimestamp(false);
        result.MutableStoreOptions()->SetStoreQuery(false);
        return result;
    }

    NProto::TUserRecordsDescription ConstructUserBodyRequestsDescription() {
        NPersonalization::NProto::TUserRecordsDescription result;
        result.SetMaxRecords(130);
        result.MutableOptions()->SetModel(NPersonalization::NProto::LogDtBigramsQueryPart);
        result.MutableOptions()->SetDwelltimeThreshold(0);
        result.MutableOptions()->SetLessThanThreshold(false);
        result.MutableOptions()->SetEmbeddingType(NPersonalization::NProto::QueryEmbedding);
        result.MutableStoreOptions()->SetStoreEmbedding(false);
        result.MutableStoreOptions()->SetStoreQuery(true);
        result.MutableStoreOptions()->SetStoreRegion(true);
        result.MutableStoreOptions()->SetStoreDdOsFamily(true);
        result.MutableStoreOptions()->SetStoreDevice(true);
        return result;
    }

    NProto::TUserRecordsDescription ConstructUserBodyFreshRequestsDescription() {
        NProto::TUserRecordsDescription result = ConstructUserBodyRequestsDescription();
        result.SetRecordLifetime(36 * 3600);
        return result;
    }

    NProto::TUserRecordsDescription ConstructUserBodyClicksDescription() {
        NPersonalization::NProto::TUserRecordsDescription result;
        result.SetMaxRecords(256);
        result.MutableOptions()->SetModel(NPersonalization::NProto::LogDwelltimeBigrams);
        result.MutableOptions()->SetDwelltimeThreshold(120);
        result.MutableOptions()->SetLessThanThreshold(false);
        result.MutableOptions()->SetEmbeddingType(NPersonalization::NProto::TitleEmbedding);
        result.MutableStoreOptions()->SetStoreEmbedding(false);
        result.MutableStoreOptions()->SetStoreUrl(true);
        result.MutableStoreOptions()->SetStoreTitle(true);
        result.MutableStoreOptions()->SetStorePosition(true);
        result.MutableStoreOptions()->SetStoreRequestTimestamp(true);
        return result;
    }

    NProto::TUserRecordsDescription ConstructUserBodyFreshClicksDescription() {
        NProto::TUserRecordsDescription result = ConstructUserBodyClicksDescription();
        result.SetRecordLifetime(36 * 3600);
        return result;
    }
}
