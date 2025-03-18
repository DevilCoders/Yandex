#pragma once

#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/codecs/codecs.h>

#include <util/string/printf.h>

namespace NSolarTrie {
    class TTrieException: public yexception {};

    struct TSolarTrieConf {
    public:
        ui32 ForkValuesBlockSize;
        ui32 BucketMaxSize;
        ui32 BucketMaxPrefixSize;
        ui32 RawValuesBlockSize;

        NCodecs::TCodecPtr KeyCodec;

        NCodecs::TCodecPtr SuffixCodec;
        NCodecs::TCodecPtr SuffixLengthCodec;
        NCodecs::TCodecPtr SuffixBlockCodec;

        NCodecs::TCodecPtr ValueCodec;
        NCodecs::TCodecPtr ValueLengthCodec;
        NCodecs::TCodecPtr ValueBlockCodec;

        NCodecs::TCodecPtr RawValuesLengthCodec;
        NCodecs::TCodecPtr RawValuesBlockCodec;

        bool Verbose;
        bool UnsortedKeys;

    public:
        TSolarTrieConf(bool verbose = false);

        void SetUnsortedKeys(bool unsorted);

        TString ReportCodecs() const;

        void SetKeyCompression(TStringBuf codecname);

        void SetSuffixCompression(TStringBuf codecname);

        void SetSuffixLengthCompression(TStringBuf codecname);

        void SetSuffixBlockCompression(TStringBuf codecname);

        void SetValueCompression(TStringBuf codecname);

        void SetValueLengthCompression(TStringBuf codecname);

        void SetValueBlockCompression(TStringBuf codecname);

        void SetRawValuesLengthCompression(TStringBuf codecname);

        void SetRawValuesBlockCompression(TStringBuf codecname);

        void ResetCodecs();

        void ClearDataCodecs();

        void Save(IOutputStream* out) const;

        void Load(IInputStream* in);

    public:
        void AddCodecName(TString& n, IBinSaver& s, NCodecs::TCodecPtr& p);

        // for mr_trie
        // called in client (write) and job (read)
        int operator&(IBinSaver& s);
    };

}
