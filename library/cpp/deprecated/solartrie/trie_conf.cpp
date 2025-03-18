#include "trie_conf.h"

#include <util/generic/yexception.h>
#include <util/string/printf.h>

namespace NSolarTrie {
    static const char MAGIC[] = "SOLAR_TR";
    static const ui32 TRIE_VERSION = 1;

    TSolarTrieConf::TSolarTrieConf(bool verbose)
        : ForkValuesBlockSize(32)
        , BucketMaxSize(32)
        , BucketMaxPrefixSize(4)
        , RawValuesBlockSize(64 * 1024)
        , Verbose(verbose)
        , UnsortedKeys()
    {
        SetKeyCompression("");

        SetSuffixCompression("huffman");
        SetSuffixLengthCompression("pfor-delta64-sorted");
        SetSuffixBlockCompression("");

        SetValueCompression("solar-16k:huffman");
        SetValueLengthCompression("pfor-delta64-sorted");
        SetValueBlockCompression("");

        SetRawValuesLengthCompression("pfor-delta32-sorted");
        SetRawValuesBlockCompression("lz4fast");
    }

    TString TSolarTrieConf::ReportCodecs() const {
        return Sprintf("keys %s, values %s, suffixes %s,"
                       " lengths (suffix %s, value %s),"
                       " blocks (suffix %s, value %s)",
                       NCodecs::ICodec::GetNameSafe(KeyCodec).data(),
                       NCodecs::ICodec::GetNameSafe(ValueCodec).data(),
                       NCodecs::ICodec::GetNameSafe(SuffixCodec).data(),
                       NCodecs::ICodec::GetNameSafe(SuffixLengthCodec).data(),
                       NCodecs::ICodec::GetNameSafe(ValueLengthCodec).data(),
                       NCodecs::ICodec::GetNameSafe(SuffixBlockCodec).data(),
                       NCodecs::ICodec::GetNameSafe(ValueBlockCodec).data());
    }

    void TSolarTrieConf::SetKeyCompression(TStringBuf codecname) {
        KeyCodec = NCodecs::ICodec::GetInstance(codecname);
        if (!!KeyCodec)
            UnsortedKeys = true;
    }

    void TSolarTrieConf::SetSuffixCompression(TStringBuf codecname) {
        SuffixCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetSuffixLengthCompression(TStringBuf codecname) {
        SuffixLengthCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetSuffixBlockCompression(TStringBuf codecname) {
        SuffixBlockCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetValueCompression(TStringBuf codecname) {
        ValueCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetValueLengthCompression(TStringBuf codecname) {
        ValueLengthCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetValueBlockCompression(TStringBuf codecname) {
        ValueBlockCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::SetRawValuesLengthCompression(TStringBuf codecname) {
        RawValuesLengthCodec = NCodecs::ICodec::GetInstance(codecname);
        Y_VERIFY(!RawValuesLengthCodec || !RawValuesLengthCodec->Traits().NeedsTraining, " ");
    }

    void TSolarTrieConf::SetRawValuesBlockCompression(TStringBuf codecname) {
        RawValuesBlockCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TSolarTrieConf::ResetCodecs() {
        using namespace NCodecs;
        KeyCodec = ICodec::GetInstance(ICodec::GetNameSafe(KeyCodec));

        SuffixCodec = ICodec::GetInstance(ICodec::GetNameSafe(SuffixCodec));
        SuffixLengthCodec = ICodec::GetInstance(ICodec::GetNameSafe(SuffixLengthCodec));
        SuffixBlockCodec = ICodec::GetInstance(ICodec::GetNameSafe(SuffixBlockCodec));

        ValueCodec = ICodec::GetInstance(ICodec::GetNameSafe(ValueCodec));
        ValueLengthCodec = ICodec::GetInstance(ICodec::GetNameSafe(ValueLengthCodec));
        ValueBlockCodec = ICodec::GetInstance(ICodec::GetNameSafe(ValueBlockCodec));

        RawValuesLengthCodec = ICodec::GetInstance(ICodec::GetNameSafe(RawValuesLengthCodec));
        RawValuesBlockCodec = ICodec::GetInstance(ICodec::GetNameSafe(RawValuesBlockCodec));
    }

    void TSolarTrieConf::ClearDataCodecs() {
        KeyCodec = nullptr;
        SuffixCodec = nullptr;
        SuffixBlockCodec = nullptr;
        ValueCodec = nullptr;
        ValueBlockCodec = nullptr;
    }

    void TSolarTrieConf::Save(IOutputStream* out) const {
        out->Write(MAGIC);
        ::Save(out, TRIE_VERSION);

        ::Save(out, ForkValuesBlockSize);
        ::Save(out, BucketMaxSize);
        ::Save(out, BucketMaxPrefixSize);

        NCodecs::ICodec::Store(out, KeyCodec);
        NCodecs::ICodec::Store(out, SuffixCodec);
        NCodecs::ICodec::Store(out, SuffixLengthCodec);
        NCodecs::ICodec::Store(out, SuffixBlockCodec);
        NCodecs::ICodec::Store(out, ValueCodec);
        NCodecs::ICodec::Store(out, ValueLengthCodec);
        NCodecs::ICodec::Store(out, ValueBlockCodec);
    }

    void TSolarTrieConf::Load(IInputStream* in) {
        char m[9];
        in->Load(m, 8);
        m[8] = 0;

        if (strcmp(m, MAGIC))
            ythrow TTrieException();

        ui32 ver = 0;
        ::Load(in, ver);

        if (ver != TRIE_VERSION)
            ythrow TTrieException();

        ::Load(in, ForkValuesBlockSize);
        ::Load(in, BucketMaxSize);
        ::Load(in, BucketMaxPrefixSize);

        KeyCodec = NCodecs::ICodec::Restore(in);

        SuffixCodec = NCodecs::ICodec::Restore(in);
        SuffixLengthCodec = NCodecs::ICodec::Restore(in);
        SuffixBlockCodec = NCodecs::ICodec::Restore(in);

        ValueCodec = NCodecs::ICodec::Restore(in);
        ValueLengthCodec = NCodecs::ICodec::Restore(in);
        ValueBlockCodec = NCodecs::ICodec::Restore(in);
    }

    void TSolarTrieConf::AddCodecName(TString& n, IBinSaver& s, NCodecs::TCodecPtr& p) {
        if (!s.IsReading())
            n = NCodecs::ICodec::GetNameSafe(p);
        s.Add(0, &n);
        if (s.IsReading())
            p = NCodecs::ICodec::GetInstance(n);
    }

    // for mr_trie
    // called in client (write) and job (read)
    int TSolarTrieConf::operator&(IBinSaver& s) {
        s.Add(0, &ForkValuesBlockSize);
        s.Add(0, &BucketMaxSize);
        s.Add(0, &BucketMaxPrefixSize);
        s.Add(0, &RawValuesBlockSize);

        TString n;
        AddCodecName(n, s, KeyCodec);

        AddCodecName(n, s, SuffixCodec);
        AddCodecName(n, s, SuffixLengthCodec);
        AddCodecName(n, s, SuffixBlockCodec);

        AddCodecName(n, s, ValueCodec);
        AddCodecName(n, s, ValueLengthCodec);
        AddCodecName(n, s, ValueBlockCodec);

        AddCodecName(n, s, RawValuesLengthCodec);
        AddCodecName(n, s, RawValuesBlockCodec);

        s.Add(0, &Verbose);

        return 0;
    }

}
