#include "url_wanderer.h"

#include "matcher.h"
#include "consts.h"

#include <kernel/snippets/simple_textproc/deyo/deyo.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/system/guard.h>
#include <util/system/spinlock.h>

namespace NUrlCutter {
    class TRTreeWandHelper : private TNonCopyable {
    private:
        TRichTreeConstPtr Rtree;
        TMaybe<TMatcherSearcher> MatcherSearcher;
        ELanguage Lang;
        THashSet<TUtf16String> DynamicStopWords;
        bool volatile Inited;
        TAdaptiveLock Lock;

    private:
        inline void AddFormsFromTree(TMatcherBuilder& matcherBuilder, const TRichRequestNode* node,
            bool isMisc = false)
        {
            if (!node) {
                return;
            }
            if (IsWord(*node)) {
                if (node->GetHiliteType()) {
                    AddWordNode(matcherBuilder, node);
                }
                for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> j(node->Markup()); !j.AtEnd(); ++j) {
                    AddFormsFromTree(matcherBuilder, j.GetData().SubTree.Get(), false);
                }
                return;
            }

            for (size_t i = 0; i < node->MiscOps.size(); ++i)
                AddFormsFromTree(matcherBuilder, node->MiscOps[i].Get(), true);
            if (isMisc) {
                return;
            }

            for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it(node->Markup()); !it.AtEnd(); ++it)
                AddFormsFromTree(matcherBuilder, it.GetData().SubTree.Get(), false);
            for (size_t i = 0; i < node->Children.size(); ++i)
                AddFormsFromTree(matcherBuilder, node->Children[i].Get(), false);
        }

        inline void AddWordNode(TMatcherBuilder& matcherBuilder, const TRichRequestNode* node) {
            const TWordNode* word = node->WordInfo.Get();
            if (!word) {
                return;
            }

            if (!word->IsLemmerWord()) {
                AddExactForm(matcherBuilder, node->GetText(), node->GetText(), node->ReverseFreq, false,
                    word->IsStopWord());
                return;
            }

            bool needTr = word->GetLangMask().SafeTest(Lang);

            for (const TLemmaForms& lemma : word->GetLemmas()) {
                for (const auto& form : lemma.GetForms()) {
                    AddExactForm(matcherBuilder, node->GetText(), form.first, node->ReverseFreq, needTr,
                        word->IsStopWord());
                    needTr = false;
                }
            }
        }

        inline static void RemoveAll(TUtf16String& in, const wchar16 ch) {
            size_t n = in.find(ch);
            while (n != TUtf16String::npos) {
                in.remove(n, 1);
                n = in.find(ch, n);
            }
        }

        inline void AddExactForm(TMatcherBuilder& matcherBuilder, const TUtf16String& word, TUtf16String form,
            i32 revFreq, bool needTr, bool isStop)
        {
            if (needTr) {
                const TLanguage* language = NLemmer::GetLanguageById(Lang);
                if (language) {
                    THolder<TUntransliter> transliter = language->GetTransliter();
                    if (transliter) {
                        int max = int(word.size());
                        TUtf16String tmp = word;
                        tmp.to_lower();
                        RemoveAll(tmp, SOFT_SIGN);
                        RemoveAll(tmp, HARD_SIGN);
                        transliter->Init(tmp);
                        TUntransliter::WordPart pt = transliter->GetNextAnswer();
                        ui32 quality = 0;
                        for (int i = 0; !pt.Empty() && (i < max || quality == pt.Quality()); ++i) {
                            quality = pt.Quality();
                            const TUtf16String& trWord = pt.GetWord();
                            matcherBuilder.AddString(trWord, TQueryWord(word, (revFreq < 0 ? 20 : revFreq), trWord.size(), isStop || StopWords.contains(trWord)));
                            pt = transliter->GetNextAnswer();
                        }
                    }
                }
            }

            form.to_lower();
            NSnippets::DeSmallYo(form);
            matcherBuilder.AddString(form, TQueryWord(word, (revFreq < 0 ? 20 : revFreq), form.size(), isStop || StopWords.contains(form)));

            // вычеркиваем только оригинальные слова из запроса (без транслитерации)
            DynamicStopWords.erase(form);
        }

        void Init() {
            if (!Inited) {
                TGuard guard(Lock);
                if (!Inited) {
                    DynamicStopWords = StopWords;
                    TMatcherBuilder matcherBuilder;
                    if (Rtree.Get()) {
                        AddFormsFromTree(matcherBuilder, Rtree->Root.Get());
                    }
                    MatcherSearcher.ConstructInPlace(matcherBuilder.Save());
                    Inited = true;
                }
            }
        }

    public:
        explicit TRTreeWandHelper(TRichTreeConstPtr rtree, ELanguage lang)
            : Rtree(rtree)
            , Lang(lang)
            , Inited(false)
        {
        }

        const TMatcherSearcher& GetMatcherSearcher() {
            Init();
            return MatcherSearcher.GetRef();
        }

        const THashSet<TUtf16String>& GetDynamicStopWords() {
            Init();
            return DynamicStopWords;
        }
    };

    void TRichTreeWanderer::AddLang(ELanguage lang) {
        if (TLang2WandererMap::insert_ctx ctx; Lang2Wanderer.find(lang, ctx) == Lang2Wanderer.end()) {
            Lang2Wanderer.emplace_direct(ctx, lang, THolder<TRTreeWandHelper>(new TRTreeWandHelper(Richtree, lang)));
        }
    }

    TRichTreeWanderer::TRichTreeWanderer(TRichTreeConstPtr rtree)
        : Richtree(rtree)
    {
    }

    TRichTreeWanderer::~TRichTreeWanderer() {
    }

    const TMatcherSearcher& TRichTreeWanderer::GetMatcherSearcher(ELanguage lang) {
        TGuard guard(Lock);
        AddLang(lang);
        return Lang2Wanderer[lang]->GetMatcherSearcher();
    }

    const THashSet<TUtf16String>& TRichTreeWanderer::GetDynamicStopWords(ELanguage lang) {
        TGuard guard(Lock);
        AddLang(lang);
        return Lang2Wanderer[lang]->GetDynamicStopWords();
    }
}
