#include <library/cpp/infected_masks/masks_comptrie.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/string/vector.h>

bool CheckValues(TVector<std::pair<TString, TStringBuf>> values, TVector<TString> canonic) {
    if (values.size() != canonic.size())
        return false;
    std::sort(canonic.begin(), canonic.end());
    std::sort(values.begin(), values.end());
    for (size_t i = 0; i < values.size(); ++i)
        if (values[i].first != canonic[i])
            return false;
    return true;
}

Y_UNIT_TEST_SUITE(CategMaskTrie) {
    Y_UNIT_TEST(Test) {
        TCategMaskComptrie masks;
        TString trieFile = GetArcadiaTestsData() + "/infected_masks/sb_masks.trie";
        TBlob trieBlob = TBlob::FromFile(trieFile);
        masks.Init(trieBlob);

        TCategMaskComptrie::TResult values;
        UNIT_ASSERT(masks.Find("example3.com http://sub.sub.example3.com/there/it/is?a=s", values) &&
                    CheckValues(values, {"example3.com/", "sub.example3.com/", "sub.sub.example3.com/there/it/is",
                                         "sub.sub.example3.com/there/", "sub.sub.example3.com/there/it/", "example3.com/there/"}));
        UNIT_ASSERT(masks.Find("example2.com http://sub.sub.example2.com/there/it", values) &&
                    CheckValues(values, {"sub.example2.com/", "sub.sub.example2.com/there/"}));
        UNIT_ASSERT(!masks.Find("flash-mx.ru http://flash-mx.ru/lessons", values));
        UNIT_ASSERT(!masks.Find("example1.com http://sub.example1.com/nosub/", values));
        UNIT_ASSERT(masks.Find("ru\texample4.com http://example4.com", values));
        UNIT_ASSERT(!masks.Find("example4.com http://example4.com", values));
        UNIT_ASSERT(!masks.Find("ru\texample3.com http://example3.com", values));

        UNIT_ASSERT(masks.FindByUrl("http://sub.sub.example3.com/there/it/is", values));
        UNIT_ASSERT(masks.FindByUrl("sub.sub.example2.com/there/it/is", values));
        UNIT_ASSERT(!masks.FindByUrl("http://example4.com", values));
    }

    Y_UNIT_TEST(TestExact) {
        TCategMaskComptrie masks;
        TString trieFile = GetArcadiaTestsData() + "/infected_masks/sb_masks.trie";
        TBlob trieBlob = TBlob::FromFile(trieFile);
        masks.Init(trieBlob);

        TStringBuf exactResult;
        UNIT_ASSERT(masks.FindExact("example2.com\tcom.example2.sub.sub/there/it/is*", exactResult));
        UNIT_ASSERT(!masks.FindExact("example2.com\tcom.example2.sub.sub/there/it/is/*", exactResult));
    }

    Y_UNIT_TEST(TestToInfectedMask) {
        TCategMaskComptrie masks;
        TString trieFile = GetArcadiaTestsData() + "/infected_masks/sb_masks.trie";
        TBlob trieBlob = TBlob::FromFile(trieFile);
        masks.Init(trieBlob);

        using TMaskKeys = TVector<std::pair<TString, TString>>;
        TMaskKeys maskKeys;
        for (auto it = masks.Begin(); it != masks.End(); ++it) {
            maskKeys.emplace_back(it.GetKey(), TCategMaskComptrie::GetInfectedMaskFromInternalKey(it.GetKey()));
        }

        UNIT_ASSERT_VALUES_EQUAL(maskKeys, (TMaskKeys{
                                               {"example0.com\tcom.example0.sub/1/*", "sub.example0.com/1/"},
                                               {"example0.com\tcom.example0.sub/nosub*", "sub.example0.com/nosub"},
                                               {"example1.com\tcom.example1.sub/1/*", "sub.example1.com/1/"},
                                               {"example1.com\tcom.example1.sub/10/*", "sub.example1.com/10/"},
                                               {"example1.com\tcom.example1.sub/2/*", "sub.example1.com/2/"},
                                               {"example1.com\tcom.example1.sub/3/*", "sub.example1.com/3/"},
                                               {"example1.com\tcom.example1.sub/4/*", "sub.example1.com/4/"},
                                               {"example1.com\tcom.example1.sub/5/*", "sub.example1.com/5/"},
                                               {"example1.com\tcom.example1.sub/6/*", "sub.example1.com/6/"},
                                               {"example1.com\tcom.example1.sub/7/*", "sub.example1.com/7/"},
                                               {"example1.com\tcom.example1.sub/8/*", "sub.example1.com/8/"},
                                               {"example1.com\tcom.example1.sub/9/*", "sub.example1.com/9/"},
                                               {"example1.com\tcom.example1.sub/nosub*", "sub.example1.com/nosub"},
                                               {"example2.com\tcom.example2.sub.sub/there/*", "sub.sub.example2.com/there/"},
                                               {"example2.com\tcom.example2.sub.sub/there/it/is*", "sub.sub.example2.com/there/it/is"},
                                               {"example2.com\tcom.example2.sub.sup/there/*", "sup.sub.example2.com/there/"},
                                               {"example2.com\tcom.example2.sub/*", "sub.example2.com/"},
                                               {"example3.com\tcom.example3.sub.sub/there/*", "sub.sub.example3.com/there/"},
                                               {"example3.com\tcom.example3.sub.sub/there/if/is*", "sub.sub.example3.com/there/if/is"},
                                               {"example3.com\tcom.example3.sub.sub/there/it/*", "sub.sub.example3.com/there/it/"},
                                               {"example3.com\tcom.example3.sub.sub/there/it/is*", "sub.sub.example3.com/there/it/is"},
                                               {"example3.com\tcom.example3.sub.sub/threre/it/is*", "sub.sub.example3.com/threre/it/is"},
                                               {"example3.com\tcom.example3.sub.sup/there/it/is*", "sup.sub.example3.com/there/it/is"},
                                               {"example3.com\tcom.example3.sub.sup/threre/it/is*", "sup.sub.example3.com/threre/it/is"},
                                               {"example3.com\tcom.example3.sub/*", "sub.example3.com/"},
                                               {"example3.com\tcom.example3/*", "example3.com/"},
                                               {"example3.com\tcom.example3/there/*", "example3.com/there/"},
                                               {"flash-mx.ru\tru.flash-mx/lessons/*", "flash-mx.ru/lessons/"},
                                               {"ru\texample4.com\tcom.example4/*", "example4.com/"}}));
    }

    Y_UNIT_TEST(TestTrieQueryMatch) {
        TStringStream s;
        {
            TCategMaskComptrie::TTrie::TBuilder builder;
            builder.Add("youtube.com\tcom.youtube/watch?v=a0MVtbB8hRM*", "xxx");
            builder.Add("youtube.com\tcom.youtube/watch?v=a0&t=10*", "xxx/");
            builder.SaveAndDestroy(s);
        }
        TCategMaskComptrie trie;
        trie.Init(TBlob::FromString(s.Str()));
        TCategMaskComptrie::TResult res;
        UNIT_ASSERT(trie.Find("youtube.com https://www.youtube.com/watch?v=a0MVtbB8hRM&t=150", res) &&
                    CheckValues(res, {"youtube.com/watch?v=a0MVtbB8hRM"}));
        UNIT_ASSERT(trie.Find("youtube.com https://www.youtube.com/watch?v=a0&t=10", res) &&
                    CheckValues(res, {"youtube.com/watch?v=a0&t=10"}));
        UNIT_ASSERT(!trie.Find("youtube.com https://www.youtube.com/watch?v=a0", res));
    }
}
