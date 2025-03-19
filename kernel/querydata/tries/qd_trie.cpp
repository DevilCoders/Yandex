#include "qd_trie.h"
#include "qd_coded_blob_trie.h"
#include "qd_comptrie.h"
#include "qd_solartrie.h"
#include "qd_codectrie.h"
#include "qd_metatrie.h"
#include "qd_categ_mask_comptrie.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>

namespace NQueryData {

    void TQDTrie::TValue::Clear() {
        Value.Clear();
        Buffer1.Clear();
        Buffer2.Clear();
        StrVal.clear();
    }

    TQDTrie::TPtr SelectTrieImpl(const TFileDescription& d) {
        switch (d.GetTrieVariant()) {
        default:
            return nullptr;
        case TV_CODED_BLOB_TRIE:
            return new TQDCodedBlobTrie();
        case TV_CODEC_TRIE:
            return new TQDCodecTrie();
        case TV_SOLAR_TRIE:
            return new TQDSolarTrie();
        case TV_COMP_TRIE:
            return new TQDCompTrie();
        case TV_METATRIE:
            return new TQDMetatrie();
        case TV_CATEG_MASK_COMP_TRIE:
            return new TQDCategMaskCompTrie();
        }
    }

    bool TrieSupportsFastMMap(const TFileDescription& d) {
        return d.GetTrieVariant() == TV_CODED_BLOB_TRIE;
    }

    ui64 PredictTrieLoadSize(const TFileDescription& d, const TBlob& data, ELoadMode lm) {
        TQDTrie::TPtr trie = SelectTrieImpl(d);
        if (!trie)
            return 0;

        return trie->PredictLoadSize(data, lm);
    }

}
