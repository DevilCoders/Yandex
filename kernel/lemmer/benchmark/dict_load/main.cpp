#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/new_engine/new_lemmer.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/testing/benchmark/bench.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>

template <ELanguage Lang>
struct TWordsHolder {
    TWordsHolder() {
        switch (Lang) {
            case LANG_RUS: {
                Word = u"есть";
                break;
            }
            case LANG_ENG: {
                Word = u"table";
                break;
            }
            case LANG_KAZ: {
                Word = u"рақмет";
                break;
            }
            default: {
                Y_FAIL("unknown language");
            }
        }
    }
    TUtf16String Word;
};

extern "C" {
extern const unsigned char RusDict[];
extern const ui32 RusDictSize;
extern const unsigned char EngDict[];
extern const ui32 EngDictSize;
extern const unsigned char KazDict[];
extern const ui32 KazDictSize;
};

template <ELanguage Lang>
struct TDictHolder {
    TDictHolder() {
        switch (Lang) {
            case LANG_RUS: {
                Blob = TBlob::NoCopy(RusDict, RusDictSize);
                break;
            }
            case LANG_ENG: {
                Blob = TBlob::NoCopy(EngDict, EngDictSize);
                break;
            }
            case LANG_KAZ: {
                Blob = TBlob::NoCopy(KazDict, KazDictSize);
                break;
            }
            default: {
                Y_FAIL("unknown language");
            }
        }
    }
    TBlob Blob;
};

#define DEFINE_BENCHMARK(Lang)                                     \
    Y_CPU_BENCHMARK(DictLoadAnalyze_##Lang, iface) {               \
        TBlob blob = TDictHolder<Lang>().Blob;                     \
        for (size_t i = 0; i < iface.Iterations(); ++i) {          \
            TNewLemmer lemmer(blob.AsCharPtr(), blob.Length());    \
            Y_UNUSED(i);                                           \
            const TUtf16String& str = TWordsHolder<Lang>().Word;   \
            TWLemmaArray out;                                      \
            NLemmer::TRecognizeOpt opt;                            \
            Y_DO_NOT_OPTIMIZE_AWAY(lemmer.Analyze(str, out, opt)); \
        }                                                          \
    }

DEFINE_BENCHMARK(LANG_RUS);
DEFINE_BENCHMARK(LANG_ENG);
DEFINE_BENCHMARK(LANG_KAZ);
