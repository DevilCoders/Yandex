#include <library/cpp/deprecated/solartrie/solartrie.h>
#include <library/cpp/deprecated/solartrie/triebuilder.h>

#include <library/cpp/on_disk/codec_trie/codectrie.h>
#include <library/cpp/on_disk/codec_trie/triebuilder.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/digest/fnv.h>
#include <util/digest/murmur.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <util/random/shuffle.h>

#include <util/system/hp_timer.h>
#include <util/string/cast.h>

//#include <google/profiler.h>

enum ETestMode {
    TM_Solar = 1,
    TM_Codec = 2,
    TM_VAL_IS_HASH = 256,
    TM_VAL_IS_STRING = 512
};

void TestHashVal(ETestMode mode, ui32 keyLength, const TVector<TBuffer>& d) {
    using namespace NSolarTrie;
    using namespace NCodecTrie;

    TSolarTrieConf solarconf;
    TCodecTrieConf codecconf;

    solarconf.Verbose = true;
    codecconf.Verbose = true;

    if (mode & TM_Solar) {
        solarconf.ResetCodecs();
        solarconf.SetSuffixCompression("huffman");
        solarconf.SetValueCompression("huffman");

        TSolarTrieBuilder solarbuilder(solarconf);

        {
            TBuffer b;
            for (auto it = d.begin(); it != d.end(); ++it) {
                TStringBuf l(it->Begin(), it->End());
                ui64 h = FnvHash<ui64>(l.begin(), l.size());
                TStringBuf key = l.SubStr(0, keyLength);
                solarbuilder.AddValue(key, h);
            }
        }

        TSolarTrie solartrie;

        {
            TBlob b = solarbuilder.Compact();
            solartrie.Init(b);
            Clog << "solar size: " << b.Size() << Endl;
        }

        {
            TBuffer sbuff0, sbuff1, cbuff;
            for (auto it = d.begin(); it != d.end(); ++it) {
                sbuff0.Clear();
                sbuff1.Clear();

                TStringBuf l(it->Begin(), it->End());
                ui64 h = FnvHash<ui64>(l.begin(), l.size());
                TStringBuf key = l.SubStr(0, keyLength);

                ui64 sval = 0;
                Y_VERIFY(solartrie.GetValue(key, sval, sbuff0, sbuff1), "%u: '%s'", (ui32)(it - d.begin()), ToString(key).data());
                Y_VERIFY(h == sval, "%u: '%s' != '%s'", (ui32)(it - d.begin()), ToString(key).data(), ToString(sval).data());
            }
        }
    }

    if (mode & TM_Codec) {
        codecconf.SetKeyCompression("huffman");
        codecconf.SetValueCompression("huffman");

        TCodecTrieBuilder<ui64> codecbuilder(codecconf);

        {
            TBuffer b;
            for (auto it = d.begin(); it != d.end(); ++it) {
                TStringBuf l(it->Begin(), it->End());
                ui64 h = FnvHash<ui64>(l.begin(), l.size());
                TStringBuf key = l.SubStr(0, keyLength);
                codecbuilder.Add(key, h);
            }
        }

        TCodecTrie<ui64> codectrie;

        {
            TBlob b = codecbuilder.Compact();
            codectrie.Init(b);
            Clog << "codec size: " << b.Size() << Endl;
        }

        {
            TBuffer sbuff0, sbuff1, cbuff;
            for (auto it = d.begin(); it != d.end(); ++it) {
                sbuff0.Clear();
                sbuff1.Clear();

                TStringBuf l(it->Begin(), it->End());
                ui64 h = FnvHash<ui64>(l.begin(), l.size());
                TStringBuf key = l.SubStr(0, keyLength);

                ui64 cval = 0;
                Y_VERIFY(codectrie.Get(key, cval, cbuff), "%u: '%s'", (ui32)(it - d.begin()), ToString(key).data());
                Y_VERIFY(h == cval, "%u: '%s' != '%s'", (ui32)(it - d.begin()), ToString(key).data(), ToString(cval).data());
            }
        }
    }
}

void TestStringVal(ETestMode mode, ui32 keyLength, const TVector<TBuffer>& d) {
    using namespace NSolarTrie;
    using namespace NCodecTrie;

    TSolarTrieConf solarconf;
    TCodecTrieConf codecconf;

    if (mode & TM_Solar) {
        solarconf.ResetCodecs();
        solarconf.SetSuffixCompression("huffman");
        solarconf.SetValueCompression("huffman");

        TSolarTrieBuilder solarbuilder(solarconf);

        {
            TBuffer b;
            for (auto it = d.begin(); it != d.end(); ++it) {
                TStringBuf l(it->Begin(), it->End());
                TStringBuf key = l.SubStr(0, keyLength);
                solarbuilder.Add(key, l);
            }
        }

        TSolarTrie solartrie;

        {
            TBlob b = solarbuilder.Compact();
            solartrie.Init(b);
            Clog << "solar size: " << b.Size() << Endl;
        }

        {
            TBuffer sbuff0, sbuff1, cbuff;
            for (auto it = d.begin(); it != d.end(); ++it) {
                sbuff0.Clear();
                sbuff1.Clear();

                TStringBuf l(it->Begin(), it->End());
                TStringBuf key = l.SubStr(0, keyLength);

                Y_VERIFY(solartrie.Get(key, sbuff0, sbuff1), "%u: '%s'", (ui32)(it - d.begin()), ToString(key).data());
                Y_VERIFY(TStringBuf(sbuff0.data(), sbuff0.size()) == l, "%u: '%s' != '%s'", (ui32)(it - d.begin()), ToString(key).data(), ToString(key).data());
            }
        }
    }

    if (mode & TM_Codec) {
        codecconf.SetKeyCompression("huffman");
        codecconf.SetValueCompression("huffman");

        TCodecTrieBuilder<TStringBuf> codecbuilder(codecconf);

        {
            TBuffer b;
            for (auto it = d.begin(); it != d.end(); ++it) {
                TStringBuf l(it->Begin(), it->End());
                TStringBuf key = l.SubStr(0, keyLength);
                codecbuilder.Add(key, l);
            }
        }

        TCodecTrie<TStringBuf> codectrie;

        {
            TBlob b = codecbuilder.Compact();
            codectrie.Init(b);
            Clog << "codec size: " << b.Size() << Endl;
        }

        {
            TBuffer sbuff0, sbuff1, cbuff;
            for (auto it = d.begin(); it != d.end(); ++it) {
                sbuff0.Clear();
                sbuff1.Clear();

                TStringBuf l(it->Begin(), it->End());
                TStringBuf key = l.SubStr(0, keyLength);

                TStringBuf cval;
                Y_VERIFY(codectrie.Get(key, cval, sbuff0, sbuff1), "%u: '%s'", (ui32)(it - d.begin()), ToString(key).data());
                Y_VERIFY(l == cval, "%u: '%s' != '%s'", (ui32)(it - d.begin()), ToString(key).data(), ToString(cval).data());
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc > 1 && TStringBuf(argv[1]) == "--help") {
        Cerr << "Usage: " << argv[0] << " [data_mode] [key_length] <data.gz" << Endl;
        exit(0);
    }

    enum EDataMode {
        DM_PLAIN = 1,
        DM_BASE64 = 2
    };

    unsigned dataMode = (argc > 1 ? FromString<int>(argv[1]) : DM_PLAIN | TM_VAL_IS_HASH | TM_VAL_IS_STRING);

    ui32 keyLength = argc > 2 ? (ui32)FromString<int>(argv[2]) : -1;

    ETestMode mode = (ETestMode)(TM_Solar | TM_Codec);

    TZLibDecompress gz(&Cin, ZLib::GZip);

    TVector<TBuffer> d;
    {
        TString line;
        while (gz.ReadLine(line)) {
            d.emplace_back();
            if (dataMode & DM_BASE64) {
                line = Base64Decode(line);
            }
            d.back().Append(line.begin(), line.end());
        }
    }

    if (dataMode & TM_VAL_IS_HASH) {
        TestHashVal(mode, keyLength, d);
    }

    if (dataMode & TM_VAL_IS_STRING) {
        TestHashVal(mode, keyLength, d);
    }

    Cout << "OK!" << Endl;
}
