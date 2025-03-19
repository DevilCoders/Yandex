#include "query.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/telephone/telephones.h>

#include <kernel/qtree/richrequest/markup/markupiterator.h>
#include <kernel/qtree/richrequest/markup/synonym.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/lemmer/core/wordinstance.h>
#include <library/cpp/stopwords/stopwords.h>
#include <library/cpp/telfinder/phone.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>

namespace NSnippets
{
    struct TSynonymInfo {
        const TRichRequestNode* Tree = nullptr;
        double Relev = 0.0;
        int Type = 0;
    public:
        TSynonymInfo(const NSearchQuery::TSynonymData& synonymData)
            : Tree(synonymData.SubTree.Get())
            , Relev(synonymData.GetRelev())
            , Type(synonymData.GetType())
        {
        }
        bool IsAlmostUserWord() const {
            // translit, orfovariant or really good extension
            return Type == TE_TRANSLIT || Type == TE_ORFOVAR || Relev == 1000;
        }
    };

    struct TQueryy::TQueryCtor
    {
        const TWordFilter& WordFilter;
        TVector<THashSet<int>> Pos2Ids;
        THashMap<TUtf16String, int> Lemma2Id;
        struct TLemmaCtorData {
            TVector<int> LemmaSynIds;
            TVector<int> LemmaAlmostUserWordsIds;
        };
        TVector<TLemmaCtorData> LemmasData;
        THashMap<TUtf16String, int> UserWord2Pos;
        TVector<bool> PosIsForcedLeftNeighbor;
        TUserTelephones UserTelephones;
        size_t WordNodesHash = 0;
        TUtf16String OrigRequestText;

        TQueryCtor(const TWordFilter& wordFilter)
            : WordFilter(wordFilter) {
        }

        inline void AddForm(TQueryy& ctx, int lemmaId, const TUtf16String& lowerForm)
        {
            auto& lowerFormData = ctx.LowerForms[lowerForm];
            lowerFormData.LemmaIds.push_back(lemmaId);
        }

        int AddLemma(TQueryy& ctx, const TUtf16String& lowerLemma, bool isUserWord)
        {
            int id = ctx.Lemmas.ysize();
            auto res = Lemma2Id.insert(THashMap<TUtf16String, int>::value_type(lowerLemma, id));
            if (res.second) { // inserted
                LemmasData.emplace_back();
                ctx.Lemmas.emplace_back();
                ctx.Lemmas[id].LemmaIsPureStopWord = true;
                ctx.Lemmas[id].LemmaIsUserWord = isUserWord;
            } else {
                id = res.first->second;
            }
            if (ctx.Positions.back().IsStopWord == false)
                ctx.Lemmas[id].LemmaIsPureStopWord = false;
            Pos2Ids.back().insert(id);
            return id;
        }

        void AddWord(TQueryy& ctx, const TRichRequestNode* node, bool isUserWord, bool addLemma)
        {
            if (ctx.ParseRequestText && isUserWord) {
                if (OrigRequestText) {
                    OrigRequestText.append(' ');
                }
                OrigRequestText.append(node->GetText());
            }
            if (!node->WordInfo || !node->WordInfo->IsLemmerWord()) {
                TUtf16String lowerWord = to_lower(node->GetText());
                int lemmaId = AddLemma(ctx, lowerWord, isUserWord);
                AddForm(ctx, lemmaId, lowerWord);
                return;
            }
            for (const TLemmaForms& lemma : node->WordInfo->GetLemmas()) {
                ctx.LangMask.SafeSet(lemma.GetLanguage());
                if (isUserWord) {
                    ctx.UserLangMask.SafeSet(lemma.GetLanguage());
                }
                TUtf16String lowerLemma = to_lower(lemma.GetLemma());
                int lemmaId = AddLemma(ctx, lowerLemma, isUserWord);
                for (const auto& form : lemma.GetForms()) {
                    AddForm(ctx, lemmaId, to_lower(form.first));
                }
                if (lemma.NumForms() == 0 || addLemma) {
                    AddForm(ctx, lemmaId, lowerLemma);
                }
            }
        }

        void AddPos(TQueryy& ctx, const TUtf16String& origWord, EQueryWordType wordType, bool isUserWord,
            bool isStopWord, double idf, bool isConjunction)
        {
            ctx.Positions.emplace_back();
            auto& pos = ctx.Positions.back();
            pos.OrigWord = origWord;
            pos.WordType = wordType;
            pos.IsUserWord = isUserWord;
            pos.IsStopWord = isStopWord;
            pos.Idf = idf;
            pos.IsConjunction = isConjunction;
            PosIsForcedLeftNeighbor.emplace_back();
            Pos2Ids.emplace_back();
        }

        bool IsConjunction(const TRichRequestNode* node)
        {
            if (node->WordInfo && node->WordInfo->IsLemmerWord()) {
                for (const TLemmaForms& lemma : node->WordInfo->GetLemmas()) {
                    if (lemma.HasGram(gConjunction)) {
                        return true;
                    }
                }
            }
            return false;
        }

        bool TreatAsStopWord(const TRichRequestNode* node)
        {
            return node->IsStopWord() ||
                node->GetText().length() <= 1 ||
                WordFilter.IsStopWord(node->GetText());
        }

        double GetNodeIdf(const TRichRequestNode* node)
        {
            return node->ReverseFreq <= 0 ? 1.0 : node->ReverseFreq;
        }

        int OnWordNode(TQueryy& ctx, const TRichRequestNode* node,
            bool isUserWord, bool isAlmostUserWord, int parentPos, int leftPos)
        {
            const double idf = GetNodeIdf(node);
            const bool isStopWord = TreatAsStopWord(node);
            const bool isConjunction = IsConjunction(node);
            AddPos(ctx, node->GetText(), QWT_WORD, isUserWord, isStopWord, idf, isConjunction);
            AddWord(ctx, node, isUserWord, false);
            int pos = Pos2Ids.ysize() - 1;
            if (!isStopWord) {
                if (parentPos != -1) {
                    for (int lemmaId : Pos2Ids[pos]) {
                        for (int synId : Pos2Ids[parentPos]) {
                            LemmasData[lemmaId].LemmaSynIds.push_back(synId);
                        }
                    }
                }
                if (isUserWord) {
                    for (int lemmaId : Pos2Ids[pos]) {
                        for (int synId : Pos2Ids[pos]) {
                            LemmasData[lemmaId].LemmaSynIds.push_back(synId);
                        }
                    }
                }
            }
            if (ctx.UseAlmostUserWords) {
                if (parentPos != -1 && isAlmostUserWord) {
                    for (int lemmaId : Pos2Ids[pos]) {
                        for (int userId : Pos2Ids[parentPos]) {
                            LemmasData[lemmaId].LemmaAlmostUserWordsIds.push_back(userId);
                        }
                    }
                }
                if (isUserWord) {
                    for (int lemmaId : Pos2Ids[pos]) {
                        for (int userId : Pos2Ids[pos]) {
                            LemmasData[lemmaId].LemmaAlmostUserWordsIds.push_back(userId);
                        }
                    }
                }
            }
            if (leftPos != -1) {
                ctx.Positions[leftPos].NeighborPositions.insert(pos);
                ctx.Positions[pos].NeighborPositions.insert(leftPos);
            }
            return pos;
        }

        inline void OnDuplicateNode(const TRichRequestNode* node, TQueryy& ctx)
        {
            AddWord(ctx, node, true, true);
        }

        void OnTelephone(const TUserTelephones::TPhoneInfo& phone, TQueryy& ctx) {
            if (phone.Type == TUserTelephones::Full || phone.Type == TUserTelephones::Local) {
                const TUtf16String phoneWithCountry = ASCIIToWide(phone.Phone.ToPhoneWithCountry());
                const TUtf16String phoneWithArea = ASCIIToWide(phone.Phone.ToPhoneWithArea());
                const TUtf16String localPhone = ASCIIToWide(phone.Phone.GetLocalPhone());
                const bool isUserWord = true;
                const bool isStopWord = false;
                const bool isConjunction = false;
                AddPos(ctx, phoneWithCountry, QWT_PHONE, isUserWord, isStopWord, phone.Idf, isConjunction);
                int lemmaId = AddLemma(ctx, to_lower(phoneWithCountry), isUserWord);
                AddForm(ctx, lemmaId, to_lower(phoneWithArea));
                AddForm(ctx, lemmaId, to_lower(localPhone));
            }
        }

        inline void OnAttrNode(const TRichRequestNode* node)
        {
            if (TUserTelephones::IsPhoneAttribute(node->GetTextName())) {
                UserTelephones.ProcessAttribute(node->GetTextName(), node->GetText(), GetNodeIdf(node));
            }
        }

        inline void VisitTree(TQueryy& ctx, const TRichRequestNode* root)
        {
            VisitNode(ctx, root, true, -1, nullptr, -1, false, nullptr, nullptr);
        }

        //left-to-right traverse query words, fill info
        void VisitNode(
            TQueryy& ctx,
            const TRichRequestNode* root,
            bool isUserNode,
            int leftPos,
            int* rightPos,
            int parentPos,
            bool isAlmostUserWord,
            const TVector<TSynonymInfo>* synonyms,
            TQueryy::TBag* bag)
        {
            if (!root)
                return;
            if (IsAttribute(*root)) {
                OnAttrNode(root);
                return;
            }

            if (IsWord(*root)) {
                if (root->GetSnippetType() == SNIPPET_NONE) {
                    return;
                }

                WordNodesHash = CombineHashes(WordNodesHash, ComputeHash(root->GetText()));
                if (!isUserNode) {
                    if ((int)WordNodesHash % 100 >= ctx.ExtRatio) {
                        return;
                    }
                }

                if (ctx.IgnoreDuplicateExtensions && !isUserNode) {
                    if (UserWord2Pos.contains(root->GetText())) {
                        OnDuplicateNode(root, ctx);
                        return;
                    }
                }

                const int pos = OnWordNode(ctx, root, isUserNode, isAlmostUserWord, parentPos, leftPos);
                if (ctx.IgnoreDuplicateExtensions && isUserNode) {
                    UserWord2Pos[root->GetText()] = pos;
                }
                if (bag) {
                    bag->Set(pos);
                }
                if (rightPos) {
                    *rightPos = pos;
                }
                for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> j(root->Markup()); !j.AtEnd(); ++j) {
                    TSynonymInfo synonym(j.GetData());
                    TQueryy::TBag mbag;
                    int synParentPos = -1;
                    if (isUserNode && IsWord(*synonym.Tree)) {
                        synParentPos = pos;
                    }
                    VisitNode(ctx, synonym.Tree, false, leftPos, nullptr, synParentPos, false, nullptr, bag ? bag : &mbag);
                    if (!bag && !mbag.Empty()) {
                        ctx.Positions[pos].Bags.push_back(std::make_pair(mbag, synonym.Relev));
                    }
                }
                if (synonyms) {
                    for (const auto& synonym : *synonyms) {
                        TQueryy::TBag mbag;
                        int synParentPos = -1;
                        bool synIsAlmostUserWord = false;
                        if (isUserNode && IsWord(*synonym.Tree)) {
                            synParentPos = pos;
                            if (ctx.UseAlmostUserWords && synonym.IsAlmostUserWord()) {
                                synIsAlmostUserWord = true;
                            }
                        }
                        VisitNode(ctx, synonym.Tree, false, leftPos, nullptr, synParentPos, synIsAlmostUserWord, nullptr, bag ? bag : &mbag);
                        if (!bag && !mbag.Empty()) {
                            ctx.Positions[pos].Bags.push_back(std::make_pair(mbag, synonym.Relev));
                        }
                    }
                }
                return;
            }

            TVector<TVector<TSynonymInfo>> childSynonyms(root->Children.size());
            for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> j(root->Markup()); !j.AtEnd(); ++j) {
                const size_t child = j->Range.End;
                if (child >= childSynonyms.size()) {
                    continue;
                }
                childSynonyms[child].push_back(TSynonymInfo(j.GetData()));
            }
            TVector<int> childrenPositions;
            childrenPositions.push_back(ctx.PosCount());
            int right = -1;
            for (size_t i = 0; i < root->Children.size(); ++i) {
                VisitNode(ctx, root->Children[i].Get(), isUserNode, i == 0 ? leftPos : right, &right, -1, false, &childSynonyms[i], bag);
                childrenPositions.push_back(ctx.PosCount());
            }
            if (rightPos && right != -1) {
                *rightPos = right;
            }
            for (size_t i = 1; i < root->Children.size(); ++i) {
                if (childrenPositions[i - 1] + 1 == childrenPositions[i] &&
                    childrenPositions[i] + 1 == childrenPositions[i + 1])
                {
                    const TProximity& prx = root->Children.ProxBefore(i);
                    if (prx.Level == BREAK_LEVEL && prx.Beg >= -1 && prx.End <= 1 && (prx.Beg || prx.End)) {
                        PosIsForcedLeftNeighbor[childrenPositions[i - 1]] = true;
                    }
                }
            }
            if (synonyms) {
                for (const auto& synonym : *synonyms) {
                    VisitNode(ctx, synonym.Tree, false, leftPos, nullptr, -1, false, nullptr, bag);
                }
            }
        }

        void FillPosCounts(TQueryy& ctx)
        {
            ctx.MUserPosCount = 0;
            ctx.MNonstopUserPosCount = 0;
            ctx.MNonstopPosCount = 0;
            ctx.MWizardPosCount = 0;
            ctx.MNonstopWizardPosCount = 0;
            for (const auto& p : ctx.Positions) {
                ctx.MUserPosCount += p.IsUserWord;
                ctx.MNonstopUserPosCount += p.IsUserWord && !p.IsStopWord;
                ctx.MNonstopPosCount += !p.IsStopWord;
                ctx.MWizardPosCount += !p.IsUserWord;
                ctx.MNonstopWizardPosCount += !p.IsUserWord && !p.IsStopWord;
            }
        }

        void FillForcedNeighborPos(TQueryy& ctx)
        {
            ctx.MaxForcedNeighborPosCount = 0;
            for (int i = 0; i < ctx.PosCount(); ++i) {
                bool canBeForced = false;
                if (PosIsForcedLeftNeighbor[i]) {
                    for (int a : Pos2Ids[i]) {
                        for (int b : Pos2Ids[i + 1]) {
                            std::pair<int, int> key = {Min(a, b), Max(a, b)};
                            auto res = ctx.LemmaPairToForcedNeighborPos.insert(THashMap<std::pair<int, int>, int>::value_type(key, i));
                            if (res.second) { // inserted
                                if (!canBeForced) {
                                    ++ctx.MaxForcedNeighborPosCount;
                                    canBeForced = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        void FillIdf(TQueryy& ctx)
        {
            double sumIdf = 0.0;
            double sumUserIdf = 0.0;
            double sumWizardIdf = 0.0;
            double sumIdfLog = 0.0;
            double sumUserIdfLog = 0.0;
            for (auto& p : ctx.Positions) {
                p.IdfLog = log(p.Idf);
                p.UserIdfLog = p.IsUserWord ? p.IdfLog : 0.0;
                sumIdfLog += p.IdfLog;
                sumUserIdfLog += p.UserIdfLog;
                sumUserIdf += p.IsUserWord ? p.Idf : 0.0;
                sumWizardIdf += p.IsUserWord ? 0.0 : p.Idf;
                sumIdf += p.Idf;
            }
            for (auto& p : ctx.Positions) {
                p.IdfNorm = p.Idf / (sumIdf + 1E-11);
                p.UserIdfNorm = p.IsUserWord ? p.Idf / (sumUserIdf + 1E-11) : 0.0;
                p.WizardIdfNorm = p.IsUserWord ? 0.0 : p.Idf / (sumWizardIdf + 1E-11);
            }
            ctx.SumIdfLog = sumIdfLog;
            ctx.SumUserIdfLog = sumUserIdfLog;
        }

        void FillSynonyms(TQueryy& ctx)
        {
            for (auto& lemmaData : LemmasData) {
                SortUnique(lemmaData.LemmaSynIds);
            }
            for (auto& entry : ctx.LowerForms) {
                auto& lowerFormData = entry.second;
                for (int id : lowerFormData.LemmaIds) {
                    for (int synId : LemmasData[id].LemmaSynIds) {
                        lowerFormData.SynIds.push_back(synId);
                    }
                }
                SortUnique(lowerFormData.SynIds);
            }
        }

        void FillAlmostUserWords(TQueryy& ctx) {
            for (auto& lemmaData : LemmasData) {
                SortUnique(lemmaData.LemmaAlmostUserWordsIds);
            }
            for (auto& entry : ctx.LowerForms) {
                auto& lowerFormData = entry.second;
                for (int id : lowerFormData.LemmaIds) {
                    for (int userId : LemmasData[id].LemmaAlmostUserWordsIds) {
                        lowerFormData.AlmostUserWordsIds.push_back(userId);
                        for (int pos : ctx.Id2Poss[id]) {
                            for (int userPos : ctx.Id2Poss[userId]) {
                                ctx.Positions[pos].AlmostUserWordsIdfNorm = Max(ctx.Positions[pos].AlmostUserWordsIdfNorm, ctx.Positions[userPos].UserIdfNorm);
                            }
                        }
                    }
                }
                SortUnique(lowerFormData.AlmostUserWordsIds);
            }
        }
    };

    TQueryy::TQueryy(const TRichRequestNode* origTree, const TConfig& cfg, const TRichRequestNode* regionPhraseTree)
        : UseAlmostUserWords(cfg.UseAlmostUserWords())
        , ExtRatio(cfg.GetExtRatio())
        , IgnoreDuplicateExtensions(cfg.IgnoreDuplicateExtensions())
        , ParseRequestText(!cfg.ExpFlagOff("get_query"))
        , OrigTree(origTree)
        , RegionQuery(regionPhraseTree ? new TRegionQuery(regionPhraseTree) : nullptr)
    {
        TQueryCtor queryCtor(cfg.GetStopWordsFilter());
        if (OrigTree) {
            queryCtor.VisitTree(*this, OrigTree);
        }

        CyrillicQuery = UserLangMask.SafeTest(LANG_RUS) || UserLangMask.SafeTest(LANG_UKR) || UserLangMask.SafeTest(LANG_BEL) || UserLangMask.SafeTest(LANG_KAZ);

        for (const auto& phone : queryCtor.UserTelephones.GetPhones()) {
            queryCtor.OnTelephone(phone, *this);
        }

        for (int i = 0; i < PosCount(); ++i) {
            TUtf16String lowerWord = to_lower(Positions[i].OrigWord);
            auto& lowerWordData = LowerWords[lowerWord];
            lowerWordData.WordPositions.push_back(i);
        }

        for (auto& entry : LowerForms) {
            SortUnique(entry.second.LemmaIds);
        }

        Id2Poss.resize(IdsCount());
        for (int j = 0; j < PosCount(); ++j) {
            for (int id : queryCtor.Pos2Ids[j]) {
                Id2Poss[id].push_back(j);
            }
        }

        queryCtor.FillPosCounts(*this);
        queryCtor.FillForcedNeighborPos(*this);
        queryCtor.FillIdf(*this);
        PosSqueezer.Reset(new TQueryPosSqueezer(*this, cfg.Algo3TopLen()));
        queryCtor.FillSynonyms(*this);
        if (UseAlmostUserWords) {
            queryCtor.FillAlmostUserWords(*this);
        }
        OrigRequestText = std::move(queryCtor.OrigRequestText);
    }

    int TQueryy::PosCount() const {
        return Positions.ysize();
    }
    int TQueryy::IdsCount() const {
        return Lemmas.ysize();
    }
    int TQueryy::NonstopPosCount() const {
        return MNonstopPosCount;
    }
    int TQueryy::UserPosCount() const {
        return MUserPosCount;
    }
    int TQueryy::NonstopUserPosCount() const {
        return MNonstopUserPosCount;
    }
    int TQueryy::WizardPosCount() const {
        return MWizardPosCount;
    }
    int TQueryy::NonstopWizardPosCount() const {
        return MNonstopWizardPosCount;
    }
    bool TQueryy::PositionsHasPhone(const TVector<int>& positions) const {
        for (int pos : positions) {
            if (Positions[pos].WordType == QWT_PHONE) {
                return true;
            }
        }
        return false;
    }
    bool TQueryy::IdsHasPhone(const TVector<int>& lemmaIds) const {
        for (int lemmaId : lemmaIds) {
            if (PositionsHasPhone(Id2Poss[lemmaId])) {
                return true;
            }
        }
        return false;
    }
}

