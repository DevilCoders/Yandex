#include "trie_file.h"

bool TrieFileHdr(const char* fname, lcpd_version_data& ver) {
    TTrieFileHdr hdr;
    TFile f(fname, RdOnly);
    f.Load(&hdr, sizeof(hdr));
    return ver.parse(hdr.VersionBuf, true);
}
