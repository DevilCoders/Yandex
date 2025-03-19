#include "entitycl.h"
#include "automatos.h"

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/sent.h>

#include <kernel/pure_lib/formcalcer.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/stopwords/stopwords.h>

struct TEntityClassier::TImpl {
    struct TEntCateg {
        TString name;
        ui32 startFacts;
        ui32 endFacts;

        TEntCateg(TStringBuf& nm, int start)
          : name(nm)
          , startFacts(start)
          , endFacts(start)
        {
        }
    };

    TVector<TEntCateg> EntCategs;

    struct TEntFact {
        ui32 Id;
        ui32 CategId;
        ui32 startWordPos;
        ui32 endWordPos;
        // sentence stats;

        ui32 FullWordsSents;
        ui32 WordsSents75;
        ui32 WordsSents50;
        ui32 WordsSents25;

        TEntFact(ui32 id, ui32 cId, ui32 start)
          : Id(id)
          , CategId(cId)
          , startWordPos(start)
          , endWordPos(start)
          , FullWordsSents(0)
          , WordsSents75(0)
          , WordsSents50(0)
          , WordsSents25(0)
        {
        }
    };

    TVector<TEntFact> EntFacts;
    TVector<bool> EntFinalizeFlags;
    bool WasTouched;
    TVector<ui32> EntWordsFormFactsCounters;
    struct TEntWordPos {
        ui32 FactId;
        ui32 WordPosId;

        TEntWordPos(ui32 fid, ui32 wpos)
          : FactId(fid)
          , WordPosId(wpos)
        {
        }
    };
    TMap< TUtf16String, TList<TEntWordPos> > EntSearchMod;
    TFormPureCalcer formCalcer;
    const TWordFilter& wrdFilt;

    TString Result;

public:
    TImpl(const NSc::TValue& entityData, const TWordFilter& wf);
    void AddFactToTheEnd(TStringBuf& categ, TStringBuf fact);

    bool Filled() {
        return EntFacts.size() > 0;
    }

    void FinalizeFact(TEntFact& fact) {
        float totalFound = 0.0;
        for (size_t j = fact.startWordPos; j < fact.endWordPos; ++j)
            if (EntWordsFormFactsCounters[j] > 0) {
                totalFound++;
                EntWordsFormFactsCounters[j] = 0;

            }

        if (totalFound == 0)
            return;

        float total = fact.endWordPos - fact.startWordPos;
        if (float(totalFound) > 0.95 * float(total))
            fact.FullWordsSents++;
        if (float(totalFound) > 0.7 * float(total))
            fact.WordsSents75++;
        if (float(totalFound) > 0.45 * float(total))
            fact.WordsSents50++;
        if (totalFound > 0.24 * total)
            fact.WordsSents25++;
    }
    void FinalizeSent() {
        if (!WasTouched)
            return;

        memset( &EntFinalizeFlags[0], true, EntFinalizeFlags.size());
        WasTouched = false;

    }
    void PrintModelStat() {
        Cout << "Categs: " << EntCategs.size() << Endl;
        Cout << "Facts: " << EntFacts.size() << Endl;
        Cout << "WordPoses:  " << EntWordsFormFactsCounters.size() << Endl;
        Cout << "Words: " << EntSearchMod.size() << Endl;
    }
    void FinalizeAllStat() {
        for (size_t i = 0; i < EntFinalizeFlags.size(); ++i) {
            if (EntFinalizeFlags[i])
                FinalizeFact(EntFacts[i]);
        }

        for (size_t j = 0; j < EntCategs.size(); ++j) {
            TEntCateg& ctg = EntCategs[j];

            ui32 stats[6] = {0, 0, 0, 0, 0, 0};
            for (size_t h = ctg.startFacts; h < ctg.endFacts; ++h) {
                TEntFact& fact = EntFacts[h];
                if (fact.FullWordsSents > 0) {
                    stats[0]++;
                    stats[1] += fact.FullWordsSents;
                }

                if (fact.WordsSents75 > 0) {
                    stats[2]++;
                    stats[3] += fact.WordsSents75;
                }

                if (fact.WordsSents50 > 0) {
                    stats[4]++;
                    stats[5] += fact.WordsSents50;
                }
            }

            Result += "|" + ctg.name + ":";
            for (size_t k = 0; k < 6 ;++k)
                Result += ToString(stats[k]) + ((k==5)?"":",");
        }

    }


    void PrintFactsClassify() {
        for (size_t i = 0; i < EntFacts.size(); ++i){
            TEntFact& fact = EntFacts[i];
            Cout << fact.Id << " " <<  fact.FullWordsSents   << " " << fact.WordsSents75 << " " << fact.WordsSents50 << " " << fact.WordsSents25 << " " << fact.endWordPos - fact.startWordPos <<  Endl;
        }
    }
    void ClassifySentence(const TUtf16String& rawSent);
};

TEntityClassier::TImpl::TImpl(const NSc::TValue& entityData, const TWordFilter& wf)
  : WasTouched(false)
  , formCalcer(false, false, false, false)
  , wrdFilt(wf)
  , Result("")
{
     if (!entityData.IsDict())
         return;

     NSc::TStringBufs categs = entityData.DictKeys();
     for (size_t i = 0; i < categs.size(); ++i) {
          TStringBuf categ = categs[i];
          const NSc::TValue& facts = entityData.Get(categ);

          if (facts.IsDict()) {
              NSc::TStringBufs fctsNames = facts.DictKeys();
              for (size_t j = 0; j < fctsNames.size(); ++j) {
                  AddFactToTheEnd(categ, facts.Get(fctsNames[j]).GetString());
              }
          }

          if (facts.IsArray()) {
              const NSc::TArray& arr = facts.GetArray();
              for (size_t j = 0; j < arr.size(); ++j) {
                 AddFactToTheEnd(categ, arr[j].GetString());
              }
          }
     }
};

TEntityClassier::TEntityClassier(const NSc::TValue& entityData, const TWordFilter& wf)
  : Impl(new TImpl(entityData, wf))
{
}

TEntityClassier::~TEntityClassier() {
}

void TEntityClassier::TImpl::AddFactToTheEnd(TStringBuf& categ, TStringBuf fact) {
    TUtf16String str = UTF8ToWide(fact);

    if (str.back() == ')') {
        size_t num = str.find('(');
        if (num != size_t(-1))
            str = str.substr(0, num);
    }

    formCalcer.Fill(str);
    if (formCalcer.Sources.size() == 0)
        return;

    TString defaultS = "";
    TString& curCat = EntCategs.size() > 0 ? EntCategs.back().name : defaultS;

    if (curCat != categ.data())
        EntCategs.push_back(TEntCateg(categ, EntFacts.size()));

    ui32 fid = EntFacts.size();
    EntFacts.push_back(TEntFact(fid, EntCategs.size()-1, EntWordsFormFactsCounters.size()));
    EntFinalizeFlags.push_back(true);
    EntCategs.back().endFacts++;

    for (size_t i = 0; i < formCalcer.Sources.size(); ++i) {
        TUtf16String word = UTF8ToWide(formCalcer.Sources[i]);
        if (word.size() <= 1 || wrdFilt.IsStopWord(word))
            continue;
        word = PorterStemm(word);
        if (EntSearchMod.find(word) == EntSearchMod.end())
            EntSearchMod[word] = TList<TEntWordPos>();

        EntSearchMod[word].push_back(TEntWordPos(fid, EntWordsFormFactsCounters.size()));
        EntWordsFormFactsCounters.push_back(0);
        EntFacts.back().endWordPos++;

    }
}

void TEntityClassier::TImpl::ClassifySentence(const TUtf16String& rawSent) {
    formCalcer.Fill(rawSent);
    for (size_t i = 0; i < formCalcer.Sources.size(); ++i) {
        TUtf16String word = UTF8ToWide(formCalcer.Sources[i]);
        word = PorterStemm(word);
        TMap<TUtf16String, TList<TEntWordPos> >::iterator it = EntSearchMod.find(word);
        if (it == EntSearchMod.end())
            continue;

        for (TList<TEntWordPos>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            TEntFact& fact = EntFacts[it2->FactId];

            if (EntFinalizeFlags[it2->FactId]) {
                EntFinalizeFlags[it2->FactId] = false;
                WasTouched = true;
                FinalizeFact(fact);
            }
            EntWordsFormFactsCounters[it2->WordPosId]++;
        }
    }
}

void TEntityClassier::ClassifySentence(const TUtf16String& rawSent) {
    Impl->ClassifySentence(rawSent);
}
void TEntityClassier::FinalizeSent() {
    Impl->FinalizeSent();
}
void TEntityClassier::FinalizeAllStat() {
    Impl->FinalizeAllStat();
}
TString TEntityClassier::GetResult() const {
    return Impl->Result;
}

TString EntityClassify(const NSnippets::TConfig& cfg, const NSnippets::TArchiveView& view) {
    TEntityClassier classify(cfg.GetEntityData(), cfg.GetStopWordsFilter());
    for (size_t i = 0; i < view.Size(); ++i) {
         const NSnippets::TArchiveSent* ptr = view.Get(i);

         classify.ClassifySentence(ptr->RawSent);
         if (i > 0) {
              const NSnippets::TArchiveSent* ptr1 = view.Get(i - 1);
              if (ptr1->RawSent.find(' ') == TUtf16String::npos)
                   classify.ClassifySentence(ptr1->RawSent);
         }

         if (i + 1 < view.Size()) {
              const NSnippets::TArchiveSent* ptr1 = view.Get(i + 1);
              if (ptr1->RawSent.find(' ') == TUtf16String::npos)
                   classify.ClassifySentence(ptr1->RawSent);
         }

         classify.FinalizeSent();
    }
    classify.FinalizeAllStat();
    return classify.GetResult();
}
