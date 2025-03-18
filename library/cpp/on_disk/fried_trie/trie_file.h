#pragma once

#include "trie.h"
#include <library/cpp/deprecated/datafile/datafile.h>

// for TDataFile, etc.
struct Y_PACKED TTrieFileHdr {
    ui8 VersionBuf[LCPD_VERSION_BUF_SZ];
    td_sz_t TrieOffset;
    td_sz_t AllocationOffset;
    ui32 WordMaxLength; // just for reference, may fail where Y_PACKED is undefined
    size_t FileSize() const {
        return LCPD_VERSION_BUF_SZ + 2 * sizeof(td_sz_t) + sizeof(ui32) + LCPD_SAVE_ALIGN(AllocationOffset);
    }
};

template <typename N, typename PL>
class TTrieFile: public TTrie<N, PL>, public TDataFile<TTrieFileHdr> {
public:
    TTrieFile(const char* fname = nullptr, EDataLoadMode loadMode = DLM_DEFAULT) {
        if (fname)
            Load(fname, loadMode);
    }

    void Load(const char* fname, EDataLoadMode loadMode) {
        TDataFile<TTrieFileHdr>::Load(fname, loadMode);
        ui8* const begin = (ui8*)TDataFile<TTrieFileHdr>::Start;
        ui8* pos = TTrie<N, PL>::FromMemoryMap(begin);

        if (!pos || pos - begin != (ssize_t)TDataFile<TTrieFileHdr>::Length)
            ythrow yexception() << fname << ": bad trie size";
    }

    bool Has(const TStringBuf& s) const {
        return TTrie<N, PL>::Find((const td_chr*)s.data(), s.size()) != nullptr;
    }
};

// return false if given file is not a TTrie image
bool TrieFileHdr(const char* fname, lcpd_version_data& ver);
