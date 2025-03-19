#pragma once

#include <kernel/querydata/saas/qd_saas_key.h>

#include <util/generic/string.h>

namespace NQueryData {
    class TSourceFactors;
    class TFileDescription;
}

namespace NQueryDataSaaS {
    class TQDSaasErrorEntry;
    class TQDSaaSInputMeta;
    class TQDSaaSInputRecord;
    class TQDSaaSSnapshotRecord;


    bool GetSubkeyFromInputRecord(TString& subkey, ESaaSSubkeyType st, const TQDSaaSInputRecord& in);

    bool GetSubkeyFromSnapshotRecord(TString& subkey, ESaaSSubkeyType st, const TQDSaaSSnapshotRecord& in);

    // works only if a shapshot record has debug info
    TSaaSKeyType GetSaaSTrieKeyTypeFromSnapshotRecord(const TQDSaaSSnapshotRecord& in);

    void SetSubkeyToInputRecord(TQDSaaSInputRecord& out, const TString& subkey, ESaaSSubkeyType st);

    void SetSubkeyToSnapshotRecord(TQDSaaSSnapshotRecord& out, const TString& subkey, ESaaSSubkeyType st);

    struct TSnapshotRecordFillParams {
        TString Namespace;
        ui64 TimestampMicro = 0;
        const TVector<ESaaSSubkeyType>* CustomKeyOrder = nullptr;
        EQDSaaSType SaasType = EQDSaaSType::Trie;
        bool AddDebugInfo = false;
    };

    void FillSnapshotRecordFromInputRecord(TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, const TQDSaaSInputMeta&, bool addDebugInfo, EQDSaaSType type);
    void FillSnapshotRecordFromInputRecord(TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, const TSnapshotRecordFillParams& params);

    void FillInputRecordFromQDSourceFactors(TQDSaaSInputRecord& out, const NQueryData::TSourceFactors& in);

    void FillInputMetaFromFileDescription(TQDSaaSInputMeta&, const NQueryData::TFileDescription&);

    void ValidateInputMeta(const TQDSaaSInputMeta&);
    void ValidateInputMeta(const TString& qdNamespace, ui64 timestampMicro);

    TQDSaasErrorEntry CreateSaasErrorEntry(const TQDSaaSSnapshotRecord& entry, const ui32 httpCode, const TString& errorMessage, ui32 retryNum);
}
