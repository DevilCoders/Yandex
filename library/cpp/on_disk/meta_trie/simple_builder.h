#pragma once

#include "metatrie.h"

namespace NMetatrie {
    class TMetatrieBuilderSimple : TNonCopyable {
    public:
        TMetatrieBuilderSimple(const TMetatrieConf& conf = TMetatrieConf()) {
            Init(conf);
        }

        void Init(const TMetatrieConf& conf) {
            Conf = conf;
            SubtrieBuilder = CreateSubtrieBuilder(conf);
        }

        void Add(TStringBuf key, TStringBuf val) {
            SubtrieBuilder->Add(key, val);
        }

        void Finish(IOutputStream& out) {
            TMetatrieBuilder b(out, CreateIndexBuilder(Conf, 1));
            b.AddSubtrie(*SubtrieBuilder);
            b.CommitAll();
        }

    private:
        TMetatrieConf Conf;
        TSubtrieBuilderPtr SubtrieBuilder;
    };

}
