#include "losswords.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/stream/buffer.h>
#include <util/stream/printf.h>
#include <util/generic/algorithm.h>

namespace NSnippets {
    template <class T>
    void Pack(IOutputStream& out, const TString& name, const T& v) {
        Save(&out, name.c_str());
        Save(&out, v);
    }

    static void SentsByType(const TSentsInfo& info, TArchiveView& view, ESentsSourceType type) {
        view.Clear();
        for (int i = 0; i < info.SentencesCount(); ++i) {
            if (info.SentVal[i].SourceType == type && info.IsSentIdFirstInArchiveSent(i)) {
                view.PushBack(&info.GetArchiveSent(i));
            }
        }
    }

    static void PullMetaDescription(const TSentsInfo& info, TArchiveView& view) {
        SentsByType(info, view, SST_META_DESCR);
    }

    class TLossWordsCallback::TImpl: public ISnippetTextDebugHandler,
                                      public ISnippetCandidateDebugHandler {
    private:
        typedef TWordStat TCurWordStat;
        const TArchiveMarkup& Markup;
        TBufferOutput Dump;
        TUnpacker* Unpacker = nullptr;
        TSentsOrder DocOrder;
        TArchiveView DocView;
        THolder<TCurWordStat> TitleStat;
        const TSnip* TitleSnip = nullptr;
        const TSnip* UnnaturalTitle = nullptr;
        const TSentsMatchInfo* SnipMatch = nullptr;
        int DocText = 0;
        int SnipText = 0;
        int AllCandCount = 0;
        int AllCandValue = 0;
        int BestCand1 = 0;
        int BestCand2 = 0;
        THolder<TCurWordStat> AllCandText;
        bool NeedTitle = false;
        bool NeedAllText = false;
        bool NeedExactMatch = false;
        TString Url;
        bool TitleReplaced = false;

        int Value(TCurWordStat& stat) const {
            if (!NeedTitle || !TitleStat)
                return (NeedExactMatch) ? stat.Data().WordSeenCount.NonstopUser : stat.Data().LikeWordSeenCount.NonstopUser;

            typedef TVector<int> Tyvi;
            const TQueryy& q = SnipMatch->Query;
            const Tyvi& t = (NeedExactMatch) ? TitleStat->Data().SeenQueryPosition : TitleStat->Data().SeenLikePos;
            const Tyvi& s = (NeedExactMatch) ? stat.Data().SeenQueryPosition : stat.Data().SeenLikePos;

            int res = 0;
            for (size_t i = 0, e = s.size(); i < e; ++i)
                if (q.Positions[i].IsUserWord && !q.Positions[i].IsStopWord)
                    if (t[i] || s[i])
                        ++res;
            return res;
        }

        int GetStatValue(const TSentsMatchInfo& info) const {
            if (info.WordsCount()) {
                TCurWordStat stat(info.Query, info);
                stat.SetSpan(0, info.WordsCount() - 1);
                return Value(stat);
            }
            return 0;
        }

        void CollectStat(TCurWordStat* stat, const TSnip& snip) {
            for (TSnip::TSnips::const_iterator i = snip.Snips.begin(); i != snip.Snips.end(); ++i) {
                if (stat->GetSpansCount() == 0)
                    stat->SetSpan(i->GetFirstWord(), i->GetLastWord());
                else
                    stat->AddSpan(i->GetFirstWord(), i->GetLastWord());
            }
        }

        int GetStatValue(const TSnip& snip) {
            if (snip.Snips.size() == 0)
                return 0;
            if (SnipMatch != snip.Snips.begin()->GetSentsMatchInfo()) {
                Dump << "[GetStatValue] Incorrect SMInfo" << Endl;
                return 0;
            }
            TCurWordStat stat(SnipMatch->Query, *SnipMatch);
            CollectStat(&stat, snip);
            return Value(stat);
        }

        void ArchiveUnpack(const TSentsMatchInfo* match) {
            SnipMatch = match;
            AllCandText.Reset(new TCurWordStat(SnipMatch->Query, *SnipMatch));

            TArchiveView metaDscr;
            PullMetaDescription(SnipMatch->SentsInfo, metaDscr);

            TSentsInfo docSentsInfo(&Markup, DocView, &metaDscr, true, false);
            TSentsMatchInfo docMatch(docSentsInfo, SnipMatch->Query, SnipMatch->Cfg);

            DocText = GetStatValue(docMatch);
            SnipText = GetStatValue(*SnipMatch);

            if (NeedAllText)
                PrintText(docSentsInfo, metaDscr);
        }
        TUtf16String QueryUserText(const TQueryy& q) const {
            TUtf16String res;
            for (const auto& position : q.Positions) {
                if (position.IsUserWord) {
                    res += position.OrigWord;
                    res += ' ';
                }
            }
            return res;
        }
        void PrintText(const TSentsInfo& docSents, const TArchiveView& meta) {
            const TQueryy& q = SnipMatch->Query;
            Dump << "--- Зарпос: " << QueryUserText(q) << " Пользовательских: " << q.NonstopUserPosCount() << Endl;
            Dump << "            Url: " << Url << Endl;
            Dump << "  Мета описание: ";
            for (size_t i = 0; i < meta.Size(); ++i)
                Dump << meta.Get(i)->Sent << "…";
            Dump << Endl;
            if (TitleSnip)
                PrintCand("  Заголовок ", *TitleSnip, Value(*TitleStat.Get()));
            Dump << "--------- Документ в архиве --- Пользовательских: " << DocText << Endl;
            Dump << docSents.Text << Endl;
            Dump << "---------- Сниппетный текст --- Пользовательских: " << SnipText << Endl;
            Dump << SnipMatch->SentsInfo.Text << Endl;
        }

        void PrintCand(const TString& name, const TSnip& snip, int snipVal) {
            Printf(Dump, "-- [%12s] --- Пользовательских: %i\t", name.c_str(), (int)snipVal);
            Dump << snip.GetRawTextWithEllipsis() << Endl;
        }

        bool CheckInit(const TSnip& snip) {
            if (snip.Snips && !SnipMatch) {
                ArchiveUnpack(snip.Snips.begin()->GetSentsMatchInfo());
            }
            return SnipMatch != nullptr;
        }

        struct TAlgo: public IAlgoTop {
            TLossWordsCallback::TImpl* Master = nullptr;
            int MaxFound = -1;
            size_t TotalCount = 0;
            size_t WithLossCount = 0;
            TString Name;
            TAlgo() {
            }
            TAlgo(const TString& name, TLossWordsCallback::TImpl* master)
                : Master(master)
                , Name(name)
            {
            }
            void Push(const TSnip& snip, const TUtf16String& /*title*/ = TUtf16String()) override {
                if (!Master->CheckInit(snip))
                    return;

                int snipVal = Master->GetStatValue(snip);
                if (MaxFound < snipVal)
                    MaxFound = snipVal;

                ++TotalCount;
                if (snipVal < Master->SnipText)
                    ++WithLossCount;

                if (Master->NeedAllText)
                    Master->PrintCand(Name, snip, snipVal);

                if (MaxFound > Master->SnipText)
                    Master->Dump << "[Warning] В кандидате: " << MaxFound << " документ: " << Master->DocText << " сниппетный текст: " << Master->SnipText << Endl;

                Master->CollectStat(Master->AllCandText.Get(), snip);
                ++Master->AllCandCount;
            }
        };

        void ResetTitleSnip(const TSnip* title) {
            if (NeedTitle && title->Snips) {
                TitleSnip = title;
                const TSentsMatchInfo* TitleMatch = TitleSnip->Snips.begin()->GetSentsMatchInfo();
                TCurWordStat* tmp = new TCurWordStat(TitleMatch->Query, *TitleMatch);
                CollectStat(tmp, *TitleSnip);
                TitleStat.Reset(tmp);
            }
        }

        void Clear() {
            SnipMatch = nullptr;
            DocView.Clear();
            DocOrder.Clear();
            Url.clear();
            DocText = SnipText = AllCandCount = AllCandValue = BestCand1 = BestCand2 = 0;
            for (TAlgoMap::iterator i = Algos.begin(); i != Algos.end(); ++i) {
                i->second.MaxFound = -1; // не было ни одного кандидата по этому алгоритму
                i->second.TotalCount = i->second.WithLossCount = 0;
            }
        }
        friend struct TAlgo;
        typedef TMap<TString, TAlgo> TAlgoMap;
        TAlgoMap Algos;

    public:
        TImpl(const TArchiveMarkup& markup, const TConfig& cfg, const TString& url)
            : Markup(markup)
            , Dump(0)
            , NeedTitle(cfg.LossWordsTitle())
            , NeedAllText(cfg.LossWordsDump())
            , NeedExactMatch(cfg.LossWordsExact())
            , Url(url)
        {
            Algos["Algo1"] = TAlgo("Algo1", this);
            Algos["Algo2"] = TAlgo("Algo2", this);
            Algos["Algo3_pairs"] = TAlgo("Algo3_pairs", this);
            Algos["AlgoPairs_single"] = TAlgo("AlgoPairs_single", this);
            Algos["AlgoTriple_single"] = TAlgo("AlgoTriple_single", this);
            Algos["Algo4_triple"] = TAlgo("Algo4_triple", this);
            Algos["Algo5_quadro"] = TAlgo("Algo5_quadro", this);
            Algos["Finals"] = TAlgo("Finals", this);
        }
        // implementation of ISnippetTextDebugHandler
        void OnUnpacker(TUnpacker* unpacker) override {
            if (SnipMatch)
                Clear();
            Unpacker = unpacker;
        }
        void OnEnd() override {
            DumpResult(DocOrder, DocView);
        }
        void OnMarkup(const TArchiveMarkupZones& zones) override {
            // узнаем границы зоны AZ_TITLE, распаковываем все кроме нее
            ui16 start = 0, end = 0;
            bool flag = false;
            const TArchiveZone& zone = zones.GetZone(AZ_TITLE);
            for (size_t i = 0; i < zone.Spans.size(); ++i) {
                const TArchiveZoneSpan& span = zone.Spans[i];
                if (!span.Empty()) {
                    ui16 s = Min(span.SentBeg, span.SentEnd);
                    ui16 e = Max(span.SentBeg, span.SentEnd);
                    if (!flag) {
                        start = s;
                        end = e;
                        flag = true;
                    } else {
                        start = Min(start, s);
                        end = Max(end, e);
                    }
                }
            }
            if (flag) {
                if (start > 0)
                    DocOrder.PushBack(0, start - 1);
                DocOrder.PushBack(end + 1, 65536);
            } else
                DocOrder.PushBack(0, 65536);
            Unpacker->AddRequest(DocOrder);
        }

        // implementation of ISnippetCandidateDebugHandler
        IAlgoTop* AddTop(const char* algo, ECandidateSource source) override {
            if (source == CS_TEXT_ARC) {
                TAlgoMap::iterator i = Algos.find(algo);
                if (i != Algos.end())
                    return &i->second;
            }
            return nullptr;
        }

        void GetExplanation(IOutputStream& output) const {
            if (!SnipMatch)
                return;

            TString str;
            Dump.Buffer().AsString(str);
            Pack(output, "dmp", str);

            TString exp = (NeedTitle ? "с учетом заголовков" : "без учета заголовков") + TString(", ") +
                         (NeedExactMatch ? "точные формы" : "точные + леммы");
            Pack(output, "<loss_words>", exp);
            Pack(output, "qw", SnipMatch->Query.NonstopUserPosCount());
            Pack(output, "dt", DocText);
            Pack(output, "st", SnipText);
            Pack(output, "ct", AllCandValue);
            Pack(output, "bst1", BestCand1);
            Pack(output, "bst2", BestCand2);
            for (TAlgoMap::const_iterator i = Algos.begin(); i != Algos.end(); ++i) {
                const TAlgo& a = i->second;
                Pack(output, i->first + "B", a.MaxFound);
                Pack(output, i->first + "L", (a.TotalCount) ? (double)a.WithLossCount / a.TotalCount : -1);
            }

            output << "</loss_words>";

            if (NeedTitle && TitleReplaced) {
                Pack<int>(output, "title_swap", 1);
            }
        }
        void OnTitleSnip(const TSnip& natural, const TSnip& unnatural) {
            ResetTitleSnip(&natural);
            UnnaturalTitle = &unnatural;
        }
        void OnBestFinal(const TSnip& snip) {
            if (SnipMatch) {
                AllCandValue = (AllCandText.Get()) ? Value(*AllCandText.Get()) : 0;
                BestCand1 = GetStatValue(snip);

                if (NeedAllText)
                    PrintCand("Лучший ", snip, BestCand1);

                ResetTitleSnip(UnnaturalTitle);
                BestCand2 = GetStatValue(snip);

                if (NeedAllText) {
                    PrintCand("Лучший'", snip, BestCand2);
                    if (TitleSnip)
                        PrintCand(" Заголовок' ", *TitleSnip, Value(*TitleStat.Get()));
                }
            }
        }
        void OnTitleReplace(bool replace) {
            TitleReplaced = replace;
        }
    };

    TLossWordsCallback::TLossWordsCallback(const TArchiveMarkup& markup, const TConfig& cfg, const TString& url)
        : Impl(new TImpl(markup, cfg, url))
    {
    }
    TLossWordsCallback::~TLossWordsCallback() {
    }
    ISnippetTextDebugHandler* TLossWordsCallback::GetTextHandler(bool isBylink) {
        return isBylink ? nullptr : Impl.Get();
    }
    ISnippetCandidateDebugHandler* TLossWordsCallback::GetCandidateHandler() {
        return Impl.Get();
    }
    void TLossWordsCallback::OnTitleSnip(const TSnip& natural, const TSnip& unnatural, bool isByLink) {
        if (!isByLink)
            Impl->OnTitleSnip(natural, unnatural);
    }
    void TLossWordsCallback::OnBestFinal(const TSnip& snip, bool isByLink) {
        if (!isByLink)
            Impl->OnBestFinal(snip);
    }
    void TLossWordsCallback::OnTitleReplace(bool replace) {
        Impl->OnTitleReplace(replace);
    }
    void TLossWordsCallback::GetExplanation(IOutputStream& output) const {
        Impl->GetExplanation(output);
    }

    template <class T>
    T Unpack(const TString& str, const char* name) {
        size_t b = str.find(name);
        if (b == TString::npos)
            return T();
        b += strlen(name);
        TMemoryInput input(str.c_str() + b, str.size() - b);
        T res;
        Load(&input, res);
        return res;
    }

    template <class T>
    double CalcMedian(TVector<T>& v) {
        std::sort(v.begin(), v.end());
        size_t i = v.size() >> 1;
        return (v.size() % 2 == 0) ? 0.5 * (v[i] + v[i - 1]) : v[i];
    }

    template <class T>
    double CalcAverage(const TVector<T>& v) {
        T sum = 0;
        for (size_t i = 0; i < v.size(); ++i)
            sum += v[i];
        return sum / v.size();
    }

    const int MaxWordsCount = 5;
    class TLossWordsExplain::TImpl {
    private:
        struct TLossStat {
            size_t DocCount = 0;
            size_t DocWithLoss = 0;
            size_t DocWithLossInCand = 0;
            size_t DocWithLossInBest = 0;
            size_t BestLessDocCount = 0;
            TVector<double> Doc2Text, Text2Cand2, Cand2Best, Doc2Best;
            struct TPerAlgo {
                TString Name;
                size_t CandWithLoss = 0;
                TVector<double> Text2Cand;
                TVector<double> AlgoLoss;
                TPerAlgo(const TString& algoName)
                    : Name(algoName)
                {
                }
                int Process(int InSnipText, const TString& exp) {
                    int MaxInCand = Unpack<int>(exp, (Name + "B").c_str());
                    if (MaxInCand >= 0 && MaxInCand < InSnipText) {
                        ++CandWithLoss;
                        Text2Cand.push_back((InSnipText) ? 1.0 - (double)MaxInCand / InSnipText : 0);
                    }
                    double l = Unpack<double>(exp, (Name + "L").c_str());
                    if (l >= 0)
                        AlgoLoss.push_back(l);
                    return MaxInCand;
                }
            };
            TVector<TPerAlgo> Algos;

            TLossStat() {
                Algos.push_back(TPerAlgo("Algo1"));
                Algos.push_back(TPerAlgo("Algo2"));
                Algos.push_back(TPerAlgo("Algo3_pairs"));
                Algos.push_back(TPerAlgo("AlgoPairs_single"));
                Algos.push_back(TPerAlgo("AlgoTriple_single"));
                Algos.push_back(TPerAlgo("Algo4_triple"));
                Algos.push_back(TPerAlgo("Algo5_quadro"));
            }
        };
        IOutputStream& Out;
        TVector<TLossStat> Stats;
        size_t TotalDocCount = 0;
        size_t LossDocCount = 0;
        size_t LossBestCount = 0;
        size_t EmptyDocCount = 0;
        size_t Text2Cand1 = 0;
        size_t GlobIndex = 0;
        size_t SwapTitleCount = 0;
        size_t BestLessDocCount = 0;
        TString Explain;

    public:
        TImpl(IOutputStream& out)
            : Out(out)
            , Stats(MaxWordsCount)
        {
        }

        void Parse(const TString& exp) {
            TString dmp = Unpack<TString>(exp, "dmp");
            if (dmp.size())
                Out << Endl << dmp;

            size_t begin = exp.find("<loss_words>");
            size_t end = exp.find("</loss_words>");
            TString str = exp.substr(begin, end - begin);
            ++GlobIndex;
            if (str.size() == 0)
                return;
            Explain = Unpack<TString>(str, "<loss_words>");
            int words = Unpack<int>(str, "qw");  // кол-во слов пользователя в запросе
            int doc = Unpack<int>(str, "dt");    // в документе
            int snip = Unpack<int>(str, "st");   // в сниппетном тексте
            int cand = Unpack<int>(str, "ct");   // в склейке всех кандидатов
            int best = Unpack<int>(str, "bst1"); // в лучшем кандидате

            if (Unpack<int>(exp, "title_swap")) {
                int tmp = best;
                best = Unpack<int>(str, "bst2");
                if (dmp.size())
                    Out << "Подстановка альтернативного заголовка (в лучшем кандидате): " << tmp << " -> " << best << Endl;
                ++SwapTitleCount;
            }

            if (Unpack<int>(str, "good"))
                Out << GlobIndex << " ";

            if (words == 0 || snip > doc)
                return;
            if (doc == 0 && snip == 0) {
                ++EmptyDocCount;
                return;
            }
            if (words > MaxWordsCount)
                words = MaxWordsCount;
            --words;

            TLossStat& s = Stats[words];
            ++TotalDocCount;
            ++s.DocCount;
            if (snip < doc) {
                ++LossDocCount;
                ++s.DocWithLoss;
                s.Doc2Text.push_back(1.0 - (double)snip / doc);
            }
            if (cand < snip)
                ++Text2Cand1;
            if (best < doc) {
                ++BestLessDocCount;
                ++s.BestLessDocCount;
                s.Doc2Best.push_back(1.0 - (double)best / doc);
            }
            s.Text2Cand2.push_back(snip ? 1.0 - (double)cand / snip : 0);
            bool fin1Flag = false;
            int maxAllCand = 0;
            for (TVector<TLossStat::TPerAlgo>::iterator i = s.Algos.begin(); i != s.Algos.end(); ++i) {
                int maxCand = i->Process(snip, str);
                if (best < maxCand)
                    fin1Flag = true;
                maxAllCand = Max(maxCand, maxAllCand);
            }
            if (fin1Flag) {
                ++LossBestCount;
                ++s.DocWithLossInBest;
            }
            if (maxAllCand < snip)
                ++s.DocWithLossInCand;
            s.Cand2Best.push_back(cand ? 1.0 - (double)best / cand : 0);
        }

        void Print() {
            float med = 0, avg = 0;
            Printf(Out, "\n\nПОТЕРИ СЛОВ ЗАПРОСА (%s)\n", Explain.data());
            Printf(Out, " Не содержащих слов запроса: %8i\n", (int)EmptyDocCount);
            Printf(Out, " Содержащих слова запроса:   %8i\n", (int)TotalDocCount);
            if (TotalDocCount == 0)
                return;
            Printf(Out, " Альтернативный заголовок:   %8i\t(%7.4f%%)\n", (int)SwapTitleCount, 100.0f * SwapTitleCount / TotalDocCount);

            TLossStat* s = nullptr;

            Printf(Out, "\n1 ЭТАП: Формирование сниппетного текста\n");
            Printf(Out, "1.1 С потерями в сниппетном тексте: %i (%7.4f%%)\n", (int)LossDocCount, 100.0f * LossDocCount / TotalDocCount);
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                Printf(Out, " Словность [%i] %8i\t(%7.4f%%)", (int)i + 1, (int)s->DocCount, 100.0f * s->DocCount / TotalDocCount);
                if (s->DocCount)
                    Printf(Out, "\t %8i (%7.4f%%)", (int)s->DocWithLoss, 100.0f * s->DocWithLoss / s->DocCount);
                Out << Endl;
            }
            Printf(Out, "1.2 Медиана значений потерь:                 среднее:\n");
            for (size_t i = 0; i < Stats.size(); ++i) {
                Out << " Словность [" << i + 1 << "]";
                s = &Stats[i];
                if (s->Doc2Text.size()) {
                    med = 100.0f * CalcMedian(s->Doc2Text);
                    avg = 100.0f * CalcAverage(s->Doc2Text);
                } else
                    med = avg = 0;
                Printf(Out, "\t\t %7.4f%%\t         \t%7.4f%%\n", med, avg);
            }

            Printf(Out, "\n2 ЭТАП: Построение кандидатов\n");
            Printf(Out, "2.1 Доля документов с потерями в склеенном тексте кандидатов: %7.4f%%\n", 100.0f * Text2Cand1 / TotalDocCount);
            for (size_t i = 0; i < Stats.size(); ++i) {
                Out << " Словность [" << i + 1 << "]" << Endl;
                s = &Stats[i];
                if (s->DocCount == 0)
                    continue;
                if (s->Text2Cand2.size())
                    Printf(Out, "\t2.2 Медиана потерь по склеенному тексту всех кандидатов:  %7.4f%%\n", 100.0f * CalcMedian(s->Text2Cand2));
                Printf(Out, "\t2.3 Доля документов              \t2.4 Медиана доли\n");
                Printf(Out, "\t    с потерями в кандидатах:         \tпотерь в кандидатах:\n");
                for (TVector<TLossStat::TPerAlgo>::iterator a = s->Algos.begin(); a != s->Algos.end(); ++a) {
                    med = (a->Text2Cand.size()) ? 100.0f * CalcMedian(a->Text2Cand) : 0;
                    Printf(Out, "\t %17s   %7.4f%%         \t%7.4f%%\n", a->Name.c_str(), 100.0f * a->CandWithLoss / s->DocCount, med);
                }
            }
            Printf(Out, "2.5 Доля документов с потерями в кандидатах\n   (максимальных по кол-ву слов запроса):\n");
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                Printf(Out, " Словность [%i] %8i", (int)i + 1, (int)s->DocWithLossInCand);
                if (s->DocCount)
                    Printf(Out, "\t(%7.4f%%)", 100.0f * s->DocWithLossInCand / s->DocCount);
                Out << Endl;
            }

            Printf(Out, "2.6 Доля кандидатов с потерями (медиана  /  среднее )\n");
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                Printf(Out, " Словность [%i]\n", (int)i + 1);
                for (TVector<TLossStat::TPerAlgo>::iterator a = s->Algos.begin(); a != s->Algos.end(); ++a) {
                    if (a->AlgoLoss.size()) {
                        med = 100.0f * CalcMedian(a->AlgoLoss);
                        avg = 100.0f * CalcAverage(a->AlgoLoss);
                        Printf(Out, "\t %17s   %7.4f%%         \t%7.4f%%\n", a->Name.c_str(), med, avg);
                    }
                }
            }

            Printf(Out, "\n3 ЭТАП: Ранжирование кандидатов\n");
            Printf(Out, "3.1 Доля документов с потерями в финальном:\t%i (%7.4f%%)\n", (int)LossBestCount, 100.0f * LossBestCount / TotalDocCount);
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                Printf(Out, " Словность [%i] %8i", (int)i + 1, (int)s->DocWithLossInBest);
                if (s->DocCount)
                    Printf(Out, "\t(%7.4f%%)", 100.0f * s->DocWithLossInBest / s->DocCount);
                Out << Endl;
            }
            Printf(Out, "3.2 Медиана доли потери в финальном:         среднее:\n");
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                if (s->Cand2Best.size())
                    Printf(Out, " Словность [%i] \t\t %7.4f%%             \t%7.4f%%\n", (int)i + 1, 100.0f * CalcMedian(s->Cand2Best), 100.0f * CalcAverage(s->Cand2Best));
            }

            Printf(Out, "\n4 ЭТАП: Суммарный показатель (относительно документа)\n");
            Printf(Out, "4.1 Доля документов с потерями в финальном:\t%i (%7.4f%%)\n", (int)BestLessDocCount, 100.0f * BestLessDocCount / TotalDocCount);
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                Printf(Out, " Словность [%i] %8i", (int)i + 1, (int)s->BestLessDocCount);
                if (s->DocCount)
                    Printf(Out, "\t(%7.4f%%)", 100.0f * s->BestLessDocCount / s->DocCount);
                Out << Endl;
            }
            Printf(Out, "4.2 Медиана доли потери в финальном:         среднее:\n");
            for (size_t i = 0; i < Stats.size(); ++i) {
                s = &Stats[i];
                if (s->Doc2Best.size())
                    Printf(Out, " Словность [%i] \t\t %7.4f%%             \t%7.4f%%\n", (int)i + 1, 100.0f * CalcMedian(s->Doc2Best), 100.0f * CalcAverage(s->Doc2Best));
            }
        }
    };

    TLossWordsExplain::TLossWordsExplain(IOutputStream& out)
        : Impl(new TImpl(out))
    {
    }

    TLossWordsExplain::~TLossWordsExplain() {
        delete Impl;
    }

    void TLossWordsExplain::Parse(const TString& explanation) {
        Impl->Parse(explanation);
    }

    void TLossWordsExplain::Print() {
        Impl->Print();
    }
}
