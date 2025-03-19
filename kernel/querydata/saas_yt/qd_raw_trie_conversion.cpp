#include "qd_raw_trie_conversion.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/server/qd_printer.h>
#include <kernel/querydata/tries/qd_categ_mask_comptrie.h>
#include <ysite/yandex/reqanalysis/normalize.h>
#include <library/cpp/scheme/scheme.h>

namespace NQueryDataSaaS {

    TString TrieMask2SaaSUrlMask(TString rawMask) {
        return NQueryData::TQDCategMaskCompTrie::GetInfectedMaskFromInternalKey(rawMask);
    }

    static NQueryData::EKeyType FixQDSubkey(TString& subkey, NQueryData::EKeyType kt) {
        if (IsIn({NQueryData::KT_CATEG_URL, NQueryData::KT_SNIPCATEG_URL}, kt)) {
            subkey.assign(TrieMask2SaaSUrlMask(subkey));
            return kt;
        } else if (IsIn({NQueryData::KT_QUERY_EXACT, NQueryData::KT_QUERY_LOWERCASE, NQueryData::KT_QUERY_SIMPLE}, kt)) {
            subkey.assign(NQueryNorm::TSimpleNormalizer()(subkey));
            return NQueryData::KT_QUERY_STRONG;
        } else {
            return kt;
        }
    }

    static void FixQDSourceFactors(NQueryData::TSourceFactors& sf) {
        if (sf.HasSourceKey()) {
            sf.SetSourceKeyType(FixQDSubkey(*sf.MutableSourceKey(), sf.GetSourceKeyType()));
        }

        if (sf.SourceSubkeysSize()) {
            for (auto& sk : *sf.MutableSourceSubkeys()) {
                sk.SetType(FixQDSubkey(*sk.MutableKey(), sk.GetType()));
            }
        }

        if (!NJson::ValidateJson(sf.GetJson())) {
            sf.SetJson(NSc::TValue::FromJsonThrow(sf.GetJson()).ToJson());
        }
    }

    void ProcessQDTrie(TBlob trie, TOnQDTrieEntry onEntry, TOnQDTrieMeta onMeta) {
        onMeta(NQueryData::IterSourceFactors(trie, [onEntry](const NQueryData::TSourceFactors& sf) {
            NQueryData::TSourceFactors tmp = sf;
            FixQDSourceFactors(tmp);
            onEntry(tmp);
        })->GetDescr());
    }

    void ProcessQDTrieMeta(TBlob trie, TOnQDTrieMeta onMeta) {
        onMeta(NQueryData::GetSource(trie)->GetDescr());
    }

}
