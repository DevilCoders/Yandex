#pragma once

#include <util/generic/strbuf.h>

#include <kernel/keyinv/invkeypos/keychars.h>

namespace NDoom {


/**
 * Key class as is relevant for panther index.
 */
enum EKeyClass {
    /**
     * Normal key without any prefixes.
     */
    NormalKey,

    /**
     * Phone-related key that should be used in panther filtration.
     *
     * We have the following phone prefixes in index:
     * - #tel_code_area - used in snippets.
     * - #tel_code_country - used in snippets.
     * - #tel_full - full phone number, can appear in qtree.
     * - #tel_local - phone number w/o area code, can appear in qtree.
     *
     * The last two are marked as PhoneKey.
     */
    PhoneKey,

    /**
     * Everything else goes here.
     */
    OtherKey
};


inline EKeyClass ClassifyKey(const TStringBuf& lemma) {
    if (lemma.size() <= 1)
        return NormalKey;

    char c = lemma[0];
    if (c == ATTR_PREFIX) {
        if (lemma.StartsWith("#tel_full") || lemma.StartsWith("#tel_local")) {
            return PhoneKey;
        } else {
            return OtherKey;
        }
    } else if (c == OPEN_ZONE_PREFIX || c == CLOSE_ZONE_PREFIX) {
        return OtherKey;
    } else {
        return NormalKey;
    }
}


} // namespace NDoom
