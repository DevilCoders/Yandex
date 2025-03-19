#include "cipher.h"

namespace NCS {

    bool TDBReservedKeyCipher::Store(const TString& encrypted, const TString& reserveEncrypted) const {
        TDBReserveEncrypted reserved(encrypted, !reserveEncrypted ? encrypted : reserveEncrypted);
        auto session = BuildNativeSession(false);
        return AddObjects({reserved}, "reserve_cipher", session) && session.Commit();
    }

    bool TDBReservedKeyCipher::Restore(const TString& encrypted, TString& reserveEncrypted) const {
        TDBReserveEncrypted reserved(encrypted, reserveEncrypted);
        for (auto&& i : GetByHash(reserved.GetHash())) {
            if (i.GetEncrypted() == reserved.GetEncrypted()) {
                reserveEncrypted = i.GetReserve();
                return true;
            }
        }
        return false;
    }

    bool TDBReservedKeyCipher::DoRebuildCacheUnsafe() const {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        IndexByHash.Initialize(Objects);
        IndexByReserveHash.Initialize(Objects);
        return true;
    }
    void TDBReservedKeyCipher::DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBReserveEncrypted>& ev) const {
        TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
        IndexByHash.Remove(ev);
        IndexByReserveHash.Remove(ev);
    }
    void TDBReservedKeyCipher::DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBReserveEncrypted>& ev, TDBReserveEncrypted& object) const {
        TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        IndexByHash.Upsert(ev);
        IndexByReserveHash.Upsert(ev);
    }

    TVector<TDBReserveEncrypted> TDBReservedKeyCipher::GetByHash(const TString& hash) const {
        TReadGuard rg(TBase::MutexCachedObjects);
        return IndexByHash.GetObjectsByKey(hash);
    }

    TVector<TDBReserveEncrypted> TDBReservedKeyCipher::GetByReserveHash(const TString& hash) const {
        TReadGuard rg(TBase::MutexCachedObjects);
        return IndexByReserveHash.GetObjectsByKey(hash);
    }

}