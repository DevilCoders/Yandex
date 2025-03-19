#include "shuffle_words.h"

#include "pseudo_rand.h"
#include "words_messer.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/passagereply.h>

namespace NSnippets {
    void ShuffleWords(const TConfig& cfg, const TString& url, const THitsInfoPtr& hitsInfo, TPassageReply& reply) {
        TWordShuffler f(PseudoRand(url, hitsInfo));
        if (cfg.BrkTit()) {
            TUtf16String t = reply.GetTitle();
            f.ShuffleWords(cfg, t);
            reply.SetTitle(t);
        }
        TUtf16String h = reply.GetHeadline();
        f.ShuffleWords(cfg, h);
        reply.SetHeadline(h);
        TVector<TUtf16String> p = reply.GetPassages();
        for (size_t i = 0; i < p.size(); ++i) {
            f.ShuffleWords(cfg, p[i]);
        }
        reply.SetPassages(p);
    }
}
