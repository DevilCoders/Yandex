#include "trie.h"
#include "trie_common.h"

#include <util/stream/output.h>
#include <util/stream/format.h>

bool NTriePrivate::Debug =
#ifndef NDEBUG
    true;
#else
    false;
#endif

lcpd_version_data::lcpd_version_data(ui32 eSig, ui32 dSig) {
#ifdef _little_endian_
    is_little_endian = true;
#else
    is_little_endian = false;
#endif

#ifdef _must_align8_
    alignment_step = 8;
#elif defined _must_align4_
    alignment_step = 4;
#elif defined _must_align2_
    alignment_step = 2;
#else
    alignment_step = 1;
#endif

    offset_type_size = sizeof(td_offs_t);
    character_type_size = sizeof(td_chr);

    version_major = LCPD_VERSION_MAJOR;
    version_minor = LCPD_VERSION_MINOR;

    enum_type_sig = eSig;
    value_type_sig = dSig;
    /*
    #ifdef PERVERT_WORD
        pervert_word_version = PERVERT_WORD_VERSION;
        pervert_word_head = prv_head;
        pervert_word_tail = prv_tail;
    #else
        pervert_word_version = 0;
        pervert_word_head = SHORT_DICTWRD_LIM;
        pervert_word_tail = 0;
    #endif
*/
}

bool lcpd_version_data::parse(const ui8* buf, bool quiet) {
    if (memcmp(buf, "lcpd_tree_tr", 12)) {
        if (!quiet)
            Cerr << "Wrong version signature - dictionary file probably corrupt" << Endl;
        return false;
    }
    buf += 12;

    is_little_endian = !acr<ui8>(buf);
    acq<ui8>(alignment_step, buf);

    acq<ui8>(offset_type_size, buf);
    acq<ui8>(character_type_size, buf);
    acq<ui32>(version_major, buf);
    acq<ui32>(version_minor, buf);
    acq<ui32>(enum_type_sig, buf);
    acq<ui32>(value_type_sig, buf);
    return true;
}

void lcpd_version_data::save(ui8* buf) const {
    memset(buf, 0, LCPD_VERSION_BUF_SZ);
    memcpy(buf, "lcpd_tree_tr", 12);
    buf += 12;

    scr<ui8>(is_little_endian ? 0 : 1, buf);
    scr<ui8>(alignment_step, buf);

    scr<ui8>(offset_type_size, buf);
    scr<ui8>(character_type_size, buf);
    scr<ui32>(version_major, buf);
    scr<ui32>(version_minor, buf);
    scr<ui32>(enum_type_sig, buf);
    scr<ui32>(value_type_sig, buf);
    // 32 free so far
}

bool lcpd_version_data::check_version(const lcpd_version_data& other) const {
    using namespace NTriePrivate;
    if (is_little_endian == other.is_little_endian && alignment_step == other.alignment_step && offset_type_size == other.offset_type_size && character_type_size == other.character_type_size && version_major >= other.version_major && (enum_type_sig == other.enum_type_sig || enum_type_sig == TVoid::RecordSig) && (value_type_sig == other.value_type_sig || value_type_sig == TVoid::RecordSig && enum_type_sig == TVoid::RecordSig))
        return true;

    Cerr << "Failed to initialize lcpd_tree because of version mismatch:" << Endl;

    if (version_major < other.version_major)
        Cerr << "\t- unsupported version " << other.version_major << "." << other.version_minor
             << ": last supported is " << version_major << "." << version_minor << Endl;

    if (is_little_endian != other.is_little_endian)
        Cerr << "\t- wrong platform endianness" << Endl;

    if (alignment_step != other.alignment_step)
        Cerr << "\t- wrong data alignment boundary" << Endl;

    if (offset_type_size != other.offset_type_size)
        Cerr << "\t- wrong offset type size" << Endl;

    if (character_type_size != other.character_type_size)
        Cerr << "\t- wrong character type size" << Endl;

    if (enum_type_sig != other.enum_type_sig)
        Cerr << "\t- wrong node enumerator type (" << Hex(other.enum_type_sig) << ", must be " << Hex(enum_type_sig) << ")" << Endl;

    if (value_type_sig != other.value_type_sig)
        Cerr << "\t- wrong terminal value type (" << Hex(other.value_type_sig) << ", must be " << Hex(value_type_sig) << ")" << Endl;

    return false;
}
