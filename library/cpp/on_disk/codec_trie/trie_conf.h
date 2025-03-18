#pragma once

#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/codecs/codecs.h>

namespace NCodecTrie {
    class TTrieException: public yexception {};

    struct TCodecTrieConf {
    public:
        NCodecs::TCodecPtr KeyCodec;
        NCodecs::TCodecPtr ValueCodec;

        bool Verbose = false;
        bool Sorted = false;
        bool Minimize = false;
        bool MakeFastLayout = true;

    public:
        TCodecTrieConf(bool verbose = false, bool sorted = false);

        void AddCodecName(TString& n, IBinSaver& s, NCodecs::TCodecPtr& p);

        // for mr_trie
        // called in client (write) and job (read)
        int operator&(IBinSaver& s);

        TString ReportCodecs() const;

        void SetKeyCompression(TStringBuf codecname);

        void SetValueCompression(TStringBuf codecname);

        void ResetCodecs();

        void Save(IOutputStream* out) const;

        void Load(IInputStream* in);
    };

}
