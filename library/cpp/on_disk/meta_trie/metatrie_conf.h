#pragma once

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/on_disk/codec_trie/trie_conf.h>
#include <library/cpp/deprecated/solartrie/trie_conf.h>

#include <library/cpp/binsaver/bin_saver.h>

namespace NMetatrie {
    enum EIndexType {
        IT_NONE = -1,
        IT_DIGEST,
        IT_COUNT,
    };

    enum ESubtrieType {
        ST_NONE = -1,
        ST_COMPTRIE = 0,
        ST_SOLARTRIE,
        ST_CODECTRIE,
        ST_COUNT,
    };

    struct TMetatrieConf {
        ECompactTrieBuilderFlags CompTrieConf;
        NCodecTrie::TCodecTrieConf CodecTrieConf;
        NSolarTrie::TSolarTrieConf SolarTrieConf;

        ESubtrieType SubtrieType;
        EIndexType IndexType;

        TMetatrieConf(ESubtrieType st = ST_CODECTRIE, EIndexType it = IT_DIGEST)
            : CompTrieConf(CTBF_PREFIX_GROUPED)
            , CodecTrieConf(false, true)
            , SubtrieType(st)
            , IndexType(it)
        {
        }

        void SetKeyCompression(TStringBuf codec) {
            SolarTrieConf.SetKeyCompression(codec);
            CodecTrieConf.SetKeyCompression(codec);
        }

        void SetValueCompression(TStringBuf codec) {
            SolarTrieConf.SetValueCompression(codec);
            CodecTrieConf.SetValueCompression(codec);
        }

        int operator&(IBinSaver& s) {
            s.Add(0, &CompTrieConf);
            s.Add(0, &SolarTrieConf);
            s.Add(0, &CodecTrieConf);
            s.Add(0, &SubtrieType);
            s.Add(0, &IndexType);
            return 0;
        }
    };

}
