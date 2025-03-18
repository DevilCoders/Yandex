#include "trie_conf.h"

#include <util/string/builder.h>

namespace NCodecTrie {
    static const char MAGIC[] = "CODEC_TR";
    static const ui32 TRIE_VERSION = 1;

    TCodecTrieConf::TCodecTrieConf(bool verbose, bool sorted)
        : Verbose(verbose)
        , Sorted(sorted)
    {
        SetKeyCompression("huffman");
        SetValueCompression("solar-16k:huffman");
    }

    void TCodecTrieConf::AddCodecName(TString& n, IBinSaver& s, NCodecs::TCodecPtr& p) {
        if (!s.IsReading())
            n = NCodecs::ICodec::GetNameSafe(p);
        s.Add(0, &n);
        if (s.IsReading())
            p = NCodecs::ICodec::GetInstance(n);
    }

    // for mr_trie
    // called in client (write) and job (read)
    int TCodecTrieConf::operator&(IBinSaver& s) {
        TString n;
        AddCodecName(n, s, KeyCodec);
        AddCodecName(n, s, ValueCodec);

        s.Add(0, &Verbose);
        s.Add(0, &Sorted);
        s.Add(0, &Minimize);

        return 0;
    }

    TString TCodecTrieConf::ReportCodecs() const {
        using namespace NCodecs;
        return TStringBuilder() << "keys " << ICodec::GetNameSafe(KeyCodec) << ", values " << ICodec::GetNameSafe(ValueCodec);
    }

    void TCodecTrieConf::SetKeyCompression(TStringBuf codecname) {
        KeyCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TCodecTrieConf::SetValueCompression(TStringBuf codecname) {
        ValueCodec = NCodecs::ICodec::GetInstance(codecname);
    }

    void TCodecTrieConf::ResetCodecs() {
        using namespace NCodecs;
        KeyCodec = ICodec::GetInstance(ICodec::GetNameSafe(KeyCodec));
        ValueCodec = ICodec::GetInstance(ICodec::GetNameSafe(ValueCodec));
    }

    void TCodecTrieConf::Save(IOutputStream* out) const {
        out->Write(MAGIC);
        ::Save(out, TRIE_VERSION);

        NCodecs::ICodec::Store(out, KeyCodec);
        NCodecs::ICodec::Store(out, ValueCodec);
    }

    void TCodecTrieConf::Load(IInputStream* in) {
        char m[9];
        size_t read = in->Load(m, 8);
        m[8] = 0;

        if (read != 8 || strcmp(m, MAGIC))
            ythrow TTrieException();

        ui32 ver = 0;
        ::Load(in, ver);

        if (ver != TRIE_VERSION)
            ythrow TTrieException();

        KeyCodec = NCodecs::ICodec::Restore(in);
        ValueCodec = NCodecs::ICodec::Restore(in);
    }

}
