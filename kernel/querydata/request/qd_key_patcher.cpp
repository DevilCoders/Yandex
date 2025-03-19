#include "qd_key_patcher.h"

#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/urlid/url2docid.h>

#include <util/generic/strbuf.h>

namespace NQueryData {

    bool PatchKey(TString& key, int& keyType) {
        if (KT_CATEG_URL == keyType || KT_SNIPCATEG_URL == keyType) {
            key = Url2DocIdSimple(TStringBuf(key).After(' '));
            keyType = KT_CATEG_URL == keyType ? KT_DOCID : KT_SNIPDOCID;
            return true;
        } else {
            return false;
        }
    }

}
