#include "richnode.h"

#include <kernel/lemmer/core/langcontext.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/morpho_lang_discr.h>

TMorphoLangDiscriminator::TResult SimpleLangDisamb(const TVector<TRichNodePtr>& children, TLangMask preferableLanguage) {
    // First pass: collect the stats about language use in the phrase
    const NLemmer::TLanguageVector& languages = NLemmer::GetLanguageList();
    TMap<ELanguage, TLangMask> langmap;

    TLangMask hasKinMask;
    TLangMask allOrNone;
    for (TVector<TRichNodePtr>::const_iterator chi = children.begin(); chi != children.end(); ++chi) {
        if (!IsWord(*chi->Get()))
            continue;

        Y_ASSERT((*chi)->WordInfo.Get());
        TLangMask wordlangmask = (*chi)->WordInfo->GetLangMask();

        TWordInstance::TLemmasVector::const_iterator i1 = (*chi)->WordInfo->GetLemmas().begin();
        TWordInstance::TLemmasVector::const_iterator e = (*chi)->WordInfo->GetLemmas().end();

        for (; i1 != e && (wordlangmask & ~hasKinMask).any(); ++i1) {
            if ((i1->GetQuality() & TYandexLemma::QFoundling) == 0)
                hasKinMask.SafeSet(i1->GetLanguage());
        }

        for (size_t i2 = 0; i2 < languages.size(); i2++) {
            ELanguage lang = languages[i2]->Id;
            if (wordlangmask.SafeTest(lang)) {
                if (langmap.find(lang) == langmap.end())
                    langmap[lang] = wordlangmask;
                else
                    langmap[lang] &= wordlangmask;
            }
        }
        if (((*chi)->WordInfo->GetLangMask() & TLangMask(LANG_RUS, LANG_UKR)) == TLangMask(LANG_RUS, LANG_UKR)) {
            static const wchar16 is[] = {'i', 'I', 0x406, 0x456, 0};
            if ((*chi)->WordInfo->GetNormalizedForm().find_first_of(is) < (*chi)->WordInfo->GetNormalizedForm().length())
                allOrNone |= TLangMask(LANG_RUS, LANG_UKR);
        }

    }

    // Disambiguation, economy version: look for languages that only occur as symbionts
    // to some other languages, and remove them from all entries - unless they're added as synonyms
    TLangMask weirdlangmask;
    TMap<ELanguage, TLangMask>::const_iterator it;

    for (it = langmap.begin(); it != langmap.end(); it++) {
        ELanguage lang = (*it).first;
        TLangMask mask = ((*it).second & ~TLangMask(lang));
        if (mask.any()) {
            if (!preferableLanguage.Test(lang) && (mask & preferableLanguage).any()
                && (!allOrNone.Test(lang) || (mask & preferableLanguage & allOrNone).none()))
            {
                weirdlangmask.Set(lang);
            } else if (!hasKinMask.Test(lang))
            {
                weirdlangmask.Set(lang);
            } else {
                // Check languages the other way round
                for (size_t i = 0; i < languages.size(); i++) {
                    ELanguage other_lang = languages[i]->Id;
                    if (mask.SafeTest(other_lang)) {
                        if (!langmap[other_lang].SafeTest(lang)) {
                            weirdlangmask.Set(lang);
                            break;
                        }
                    }
                }
            }
        }
    }
    return TMorphoLangDiscriminator::TResult(weirdlangmask, TLangMask());
}

TMorphoLangDiscriminator::TResult MorphoLangDisamb(const TVector<TRichNodePtr>& children, const TDisambiguationOptions& options, const TLangMask& addMainLang) {
    TMorphoLangDiscriminator disc(options.GetDiscrList(), options.KeepLangMask);
    for (TVector<TRichNodePtr>::const_iterator chi = children.begin(); chi != children.end(); ++chi) {
        if (!IsWord(*chi->Get()))
            continue;
        Y_ASSERT((*chi)->WordInfo.Get());
        disc.AddWord(*(*chi)->WordInfo, children.size() == 1);
    }
    return disc.ObtainResult(options.PreferredLangMask | addMainLang);
}

static TLangMask DisambiguatePhrase(TNodeSequence& children, const TDisambiguationOptions& options, const TLangMask& addMainLang) {
    if (!options.FilterLanguages)
        return TLangMask();
    TMorphoLangDiscriminator::TResult langs = MorphoLangDisamb(children, options, addMainLang);
    TLangMask langsToKill = langs.Loosers;

    for (size_t chi = 0; chi < children.size(); ++chi) {
        TRichRequestNode* child = children[chi].Get();
        if (!IsWord(*child))
            continue;

        Y_ASSERT(child->WordInfo.Get());
        TWordNode* winfo = child->WordInfo.Get();

        if ((winfo->GetLangMask() & langsToKill).any() &&
             (winfo->GetLangMask() & ~langsToKill).any())
        {
            TWordInstanceUpdate(*winfo).FilterLemmas(langsToKill);
        }
    }
    return langs.Winners;
}

static TLangMask DisambiguatePhrases(TRichRequestNode* root, const TDisambiguationOptions& options, const TLangMask& addMainLang) {
    TLangMask allWinners;
    for (size_t i = 0; i < root->Children.size(); ++i)
        allWinners |= DisambiguatePhrases(root->Children[i].Get(), options, addMainLang);
    for (size_t i = 0; i < root->MiscOps.size(); ++i)
        allWinners |= DisambiguatePhrases(root->MiscOps[i].Get(), options, addMainLang);
    switch (root->Op()) {
        case oAnd:
        case oOr:
        case oWeakOr:
            allWinners |= DisambiguatePhrase(root->Children, options, addMainLang);
            break;
        default:
            break;
    }
    return allWinners;
}

static TLangMask DisambiguateOneWord(TRichRequestNode* root, const TDisambiguationOptions& options, const TLangMask& addMainLang) {
    Y_ASSERT(root);
    Y_ASSERT(root->WordInfo.Get());
    TLangMask langsToKill;
    TLangMask winners;
    // For single-word requests only, remove lemmas but the best ones
    if (root->WordInfo->GetFormType() == fGeneral)
        langsToKill |= TWordInstanceUpdate(*root->WordInfo).SelectBest(options.FilterOneWordLemmas, options.PreferredLangMask | addMainLang);
    if (options.FilterLanguages) {
        TMorphoLangDiscriminator disc(options.GetDiscrList(), options.KeepLangMask);
        Y_ASSERT(root->WordInfo.Get());
        disc.AddWord(*root->WordInfo, true);
        TMorphoLangDiscriminator::TResult langs = disc.ObtainResult(options.PreferredLangMask | addMainLang);
        langsToKill |= langs.Loosers;
        winners = langs.Winners;
        if (langsToKill.any())
            TWordInstanceUpdate(*root->WordInfo).FilterLemmas(langsToKill);
    }
    return winners;
}

static void DisambiguateSynonyms(TRichRequestNode* node, const TDisambiguationOptions& options, const TLangMask& treeLang) {
    for (size_t i = 0; i < node->Children.size(); ++i)
        DisambiguateSynonyms(node->Children[i].Get(), options, treeLang);

    TDisambiguationOptions myOptions(options);
    myOptions.PreferredLangMask = treeLang;

    using NSearchQuery::TCheckMarkupIterator;
    using NSearchQuery::TForwardMarkupIterator;
    using NSearchQuery::TSynonymTypeCheck;

    typedef TCheckMarkupIterator<TForwardMarkupIterator<TSynonym, false>, TSynonymTypeCheck> TIt;
    for (TIt i(node->MutableMarkup(), TSynonymTypeCheck(TE_WEAKOR)); !i.AtEnd(); ++i)
        DisambiguateLanguagesTree(i.GetData().SubTree.Get(), myOptions, TLangMask());
}

TLangMask DisambiguateLanguagesTree(TRichRequestNode* root, const TDisambiguationOptions& options, const TLangMask& addMainLang) {
    Y_ASSERT(root);
    TLangMask winners;
    if (root->Children.empty() && root->WordInfo.Get())
        winners = DisambiguateOneWord(root, options, addMainLang);
    else
        winners = DisambiguatePhrases(root, options, addMainLang);
    DisambiguateSynonyms(root, options, winners);
    return winners;
}
