#include <util/stream/file.h>
#include <library/cpp/containers/ext_priority_queue/ext_priority_queue.h>
#include <ysite/yandex/doppelgangers/normalize.h>
#include <kernel/yawklib/wtrutil.h>
#include <kernel/yawklib/dupes.h>
#include <queue>
#include "classify.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRephrases::AddIntents(const TClassifierScript &script)
{
    TUtf16String noIntent; noIntent += '*';
    for (TClassifierScript::const_iterator it = script.begin(); it != script.end(); ++it) {
        RephraseTypes.emplace_back();
        TRephraseData &r = RephraseTypes.back();
        r.Categ = it->first;
        r.Intent = noIntent;
        r.Props = it->second.MainIntentProps;
        const TVector<TUtf16String> &categSynonyms = it->second.Synonyms;
        for (size_t n = 0; n < categSynonyms.size(); ++n)
            r.Subphrases[categSynonyms[n]] = MF_OPTIONAL;
        for (size_t n = 0; n < it->second.Intents.size(); ++n) {
            const TIntent &i = it->second.Intents[n];
            RephraseTypes.emplace_back();
            TRephraseData &ri = RephraseTypes.back();
            ri.Categ = it->first;
            ri.Intent = i.UniqueName;
            ri.Props = i.Props;
            for (size_t m = 0; m < i.Buzzwords.size(); ++m)
                ri.Subphrases[i.Buzzwords[m]] = MF_OPTIONAL;
            for (size_t m = 0; m < categSynonyms.size(); ++m)
                ri.Subphrases[categSynonyms[m]] = MF_OPTIONAL;
            for (size_t m = 0; m < i.Wordings.size(); ++m)
                ri.Subphrases[i.Wordings[m]] = MF_INTENT;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRephrases::Fill(const TClassifierScript &script)
{
    AddIntents(script);
    FinishInitialization();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRephrases::Fill(const TClassifierScript &script, const TCategorizedReqs &reqs)
{
    AddIntents(script);
    for (TCategorizedReqs::const_iterator it = reqs.begin(); it != reqs.end(); ++it) {
        const TVector<TObjectCategory> &categs = it->second;
        for (size_t n = 0; n < categs.size(); ++n) {
            TUtf16String categoryName = categs[n].CategoryName;
            for (size_t m = 0; m < RephraseTypes.size(); ++m) {
                if (RephraseTypes[m].Categ == categoryName) {
                    TRephraseData &r = RephraseTypes[m];
                    r.Subphrases[it->first] = MF_OBJECT;
                    r.CanonicNames[it->first] = categs[n].CanonicName;
                }
            }
        }
    }
    FinishInitialization();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRephrases::FinishInitialization()
{
    for (size_t idx = 0; idx < RephraseTypes.size(); ++idx) {
        TRephraseData &r = RephraseTypes[idx];
        for (THashMap<TUtf16String, EMatchFlag>::const_iterator it = r.Subphrases.begin(); it != r.Subphrases.end(); ++it)
            PhraseStart2Indices[it->first].push_back(idx);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TQueueToken
{
    size_t Pos;
    size_t RephraseTypeIndex;
    int MatchFlags;
    bool ForwardIntent;
    TUtf16String Object;
    TQueueToken(size_t pos, size_t idx, int matchFlags, bool fwd, TUtf16String obj)
        : Pos(pos)
        , RephraseTypeIndex(idx)
        , MatchFlags(matchFlags)
        , ForwardIntent(fwd)
        , Object(obj)
    {
    }
    bool operator < (const TQueueToken &other) const {
        return Pos > other.Pos;
    }
};
void TRephrases::RecognizeExt(const TUtf16String &req, IObjectRecognizer *extObjects, TVector<TRephrase> *res) const
{
    Y_ASSERT(res->empty());
    Y_ASSERT(req == MakeUniformStyle(req) && "requests for TRephrases::Recognize must be normalized first");
    TUtf16String noIntent; noIntent += '*';
    std::priority_queue<TQueueToken> queue;
    size_t spacePos1 = req.find(' ');
    while (true) {
        TUtf16String phraseStart = req.substr(0, spacePos1);
        TPhraseStart2Indices::const_iterator it = PhraseStart2Indices.find(phraseStart);
        if (it != PhraseStart2Indices.end()) {
            for (size_t n = 0; n < it->second.size(); ++n) {
                size_t idx = it->second[n];
                EMatchFlag flag = RephraseTypes[idx].Subphrases.find(phraseStart)->second;
                queue.push(TQueueToken(spacePos1, idx, flag, flag == MF_INTENT, flag == MF_OBJECT? phraseStart : TUtf16String()));
            }
        }
        if (extObjects) {
            for (size_t idx = 0; idx < RephraseTypes.size(); ++idx) {
                TUtf16String categ = RephraseTypes[idx].Categ;
                bool add = extObjects->IsObjectFromCateg(phraseStart, categ);
                do {
                    if (add)
                        queue.push(TQueueToken(spacePos1, idx, MF_OBJECT, false, phraseStart));
                    ++idx;
                } while (idx < RephraseTypes.size() && RephraseTypes[idx].Categ == categ);
            }
        }
        if (spacePos1 == TUtf16String::npos)
            break;
        spacePos1 = req.find(' ', spacePos1 + 1);
    }
    bool reportedOverflow = false;
    while (!queue.empty()) {
        TQueueToken cur = queue.top();
        const TRephraseData &r = RephraseTypes[cur.RephraseTypeIndex];
        if (cur.Pos == TUtf16String::npos) {
            if ((cur.MatchFlags == (MF_INTENT | MF_OBJECT)) || (cur.MatchFlags == MF_OBJECT && r.Intent == noIntent)) {
                THashMap<TUtf16String, TUtf16String>::const_iterator it = r.CanonicNames.find(cur.Object);
                TUtf16String canonicName = (it == r.CanonicNames.end())? cur.Object : it->second;
                res->push_back(TRephrase(r.Categ, cur.Object, canonicName, r.Intent, cur.ForwardIntent, r.Props));
            }
        } else {
            size_t spacePos2 = req.find(' ', cur.Pos + 1);
            while (true) {
                TUtf16String subphrase = req.substr(cur.Pos + 1, spacePos2 - cur.Pos - 1);
                THashMap<TUtf16String, EMatchFlag>::const_iterator it = r.Subphrases.find(subphrase);
                if (it != r.Subphrases.end()) {
                    if (it->second != MF_OBJECT || ((cur.MatchFlags & MF_OBJECT) == 0)) {
                        if (queue.size() > 1000) {
                            if (!reportedOverflow) {
                                reportedOverflow = true;
                                fprintf(stderr, "Too huge number of parse variants for request %s\n", WideToUTF8(req).data());
                            }
                        } else {
                            TQueueToken add(cur);
                            add.Pos = spacePos2;
                            add.MatchFlags |= it->second;
                            add.ForwardIntent |= (add.MatchFlags == MF_INTENT);
                            if (it->second == MF_OBJECT)
                                add.Object = subphrase;
                            queue.push(add);
                        }
                    }
                }
                if (extObjects && ((cur.MatchFlags & MF_OBJECT) == 0)) {
                    if (extObjects->IsObjectFromCateg(subphrase, r.Categ)) {
                        TQueueToken add(cur);
                        add.Pos = spacePos2;
                        add.MatchFlags |= MF_OBJECT;
                        add.Object = subphrase;
                        queue.push(add);
                    }
                }
                if (spacePos2 == TUtf16String::npos)
                    break;
                spacePos2 = req.find(' ', spacePos2 + 1);
            }
        }
        queue.pop();
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TRephrases::Recognize(const TUtf16String &req, TVector<TRephrase> *res) const
{
    RecognizeExt(req, nullptr, res);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ParseObjectWithComment(const TString &tab, TUtf16String *obj, TString *comment)
{
    size_t commentPos = tab.find(" //");
    // commentPos == TString::npos is ok
    *obj = UTF8ToWide(tab.substr(0, commentPos));
    *comment = tab.substr(commentPos);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class TContainer>
void AddObjectsLine(const TContainer &tabs, TCategorizedReqs *res)
{
    if (tabs.empty())
        return;
    if (tabs.size() < 2) {
        TString s = tabs[0];
        fprintf(stderr, "TCategorizedReqs::Load - cannot understand this line, object name missing: %s\n", s.data());
        return;
    }
    TUtf16String reqCanonic;
    TString comment;
    ParseObjectWithComment(tabs[1], &reqCanonic, &comment);
    TUtf16String catName = UTF8ToWide(tabs[0]);
    for (size_t n = 1; n < tabs.size(); ++n) {
        TUtf16String req;
        ParseObjectWithComment(tabs[n], &req, &comment);
        if (req.empty())
            continue;
        TVector<TObjectCategory> &dst = (*res)[req];
        dst.emplace_back();
        TObjectCategory &cat = dst.back();
        cat.CanonicName = reqCanonic;
        cat.CategoryName = catName;
        if (comment.size())
            res->AddComment(catName, req, comment);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TCategorizedReqs::LoadOneObject(const TVector<TString> &tabs) {
    AddObjectsLine(tabs, this);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TCategorizedReqs::Load(const TString &filePath)
{
    TFileInput file(filePath);
    TString st;
    while (file.ReadLine(st)) {
        TVector<const char*> tabs;
        Split(st.begin(), '\t', &tabs);
        AddObjectsLine(tabs, this);
    }
    CheckCorrectness();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef THashMap<TString, TVector<TString> > TObj2Records;
typedef THashMap<TUtf16String, TObj2Records> TCat2Obj2Records;
TString JoinObjectAndComments(const TCategorizedReqs &reqs, const TUtf16String &req, const TUtf16String &categ)
{
    return WideToUTF8(req) + reqs.GetComments(categ, req);
}
void RearrangeObjects(const TCategorizedReqs &reqs, TCat2Obj2Records *res)
{
    for (TCategorizedReqs::const_iterator it = reqs.begin(); it != reqs.end(); ++it) {
        const TVector<TObjectCategory> &cats = it->second;
        for (size_t n = 0; n < cats.size(); ++n) {
            const TUtf16String &catName = cats[n].CategoryName;
            TString objName = JoinObjectAndComments(reqs, cats[n].CanonicName, catName);
            TString record = JoinObjectAndComments(reqs, it->first, catName);
            (*res)[catName][objName].push_back(record);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TCategorizedReqs::Save(const TString &filePath) const
{
    CheckCorrectness();

    TCat2Obj2Records cat2obj2records;
    RearrangeObjects(*this, &cat2obj2records);
    TFixedBufferFileOutput file(filePath);
    for (TCat2Obj2Records::const_iterator itCat = cat2obj2records.begin(); itCat != cat2obj2records.end(); ++itCat) {
        TString catName = WideToUTF8(itCat->first);
        for (TObj2Records::const_iterator itObj = itCat->second.begin(); itObj != itCat->second.end(); ++itObj) {
            file << catName;
            file << "\t" << itObj->first;
            for (size_t n = 0; n < itObj->second.size(); ++n) {
                TString synonym = itObj->second[n];
                if (synonym != itObj->first)
                    file << "\t" << synonym;
            }
            file << Endl;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TCategorizedReqs::AddComment(const TUtf16String &categ, const TUtf16String &object, const TString &comment)
{
    TString &res = Categ2Object2Comment[categ][object];
    res += " //";
    res += comment;
}
TString TCategorizedReqs::GetComments(const TUtf16String &categ, const TUtf16String &object) const
{
    TCateg2Comments::const_iterator it1 = Categ2Object2Comment.find(categ);
    if (it1 == Categ2Object2Comment.end())
        return TString();
    TObject2Comments::const_iterator it2 = it1->second.find(object);
    return it2 == it1->second.end()? TString() : it2->second;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static const TObjectCategory* GetObjectForCategory(const TCategorizedReqs &reqs, const TUtf16String &categ, const TUtf16String &object)
{
    TCategorizedReqs::const_iterator it = reqs.find(object);
    if (it == reqs.end())
        return nullptr;
    const TVector<TObjectCategory> &data = it->second;
    for (size_t n = 0; n < data.size(); ++n) {
        if (data[n].CategoryName == categ)
            return &data[n];
    }
    return nullptr;
}
void TCategorizedReqs::CheckCorrectness() const
{
    for (TCategorizedReqsData::const_iterator it = Data.begin(); it != Data.end(); ++it) {
        TString req = WideToUTF8(it->first);
        if (req.empty())
            ythrow yexception() << "an empty request is categorized";
        if (it->second.empty())
            ythrow yexception() << "an empty categs.list for " << req;
        for (size_t n = 0; n < it->second.size(); ++n) {
            const TObjectCategory &cat = it->second[n];
            if (cat.CategoryName.empty())
                ythrow yexception() << "an empty category name for " << req;
            if (cat.CategoryName.empty())
                ythrow yexception() << "an empty category name for " << req;
            for (size_t m = 0; m < n; ++m)
                if (it->second[m].CategoryName == cat.CategoryName)
                    ythrow yexception() << "request " << req << " has two records for one category " << WideToUTF8(cat.CategoryName);
            const TObjectCategory* canonicRecord = GetObjectForCategory(*this, cat.CategoryName, cat.CanonicName);
            if (!canonicRecord)
                ythrow yexception() << "an unknown canonic request " << WideToUTF8(cat.CanonicName)
                << " for object " << req << ", category " << WideToUTF8(cat.CategoryName);
            else if (canonicRecord->CanonicName != cat.CanonicName)
                ythrow yexception() << "a canonic request " << WideToUTF8(cat.CanonicName)
                << " for object " << req << ", category " << WideToUTF8(cat.CategoryName)
                << " is not canonic for itself, and is canonized as " << WideToUTF8(canonicRecord->CanonicName);
        }
    }
    for (TCateg2Comments::const_iterator it1 = Categ2Object2Comment.begin(); it1 != Categ2Object2Comment.end(); ++it1) {
        for (TObject2Comments::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
            if (!it2->second.StartsWith(" //"))
                ythrow yexception() << "comment for " << WideToUTF8(it1->first) << " " << WideToUTF8(it2->first)
                    << " doesn't start with comment modifier: " << it2->second;
            if (!GetObjectForCategory(*this, it1->first, it2->first))
                ythrow yexception() << "there is a comment for " << WideToUTF8(it1->first) << " " << WideToUTF8(it2->first)
                    << " but this request is not in that category";
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
void AddUniq(TVector<T> &dst, T what) {
    if (std::find(dst.begin(), dst.end(), what) == dst.end())
        dst.push_back(what);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TExplicitIntentsDetector::Fill(const TClassifierScript &script) {
    THashMap<TUtf16String, TVector<int> > fragment2uid;
    THashMap<TUtf16String, int> uids;
    TUtf16String noIntent; noIntent += '*';
    for (TClassifierScript::const_iterator it = script.begin(); it != script.end(); ++it) {
        for (size_t n = 0; n < it->second.Intents.size(); ++n) {
            const TIntent &i = it->second.Intents[n];
            if (i.Props & IPROP_HAS_GLOBAL_UID) {
                int uid;
                if (uids.contains(i.UniqueName))
                    uid = uids[i.UniqueName];
                else {
                    uid = uids.size();
                    uids[i.UniqueName] = uid;
                }
                for (size_t m = 0; m < i.Wordings.size(); ++m)
                    AddUniq(fragment2uid[i.Wordings[m]], uid);
            }
        }
    }
    Uids.resize(uids.size());
    for (THashMap<TUtf16String, int>::const_iterator it = uids.begin(); it != uids.end(); ++it)
        Uids[it->second] = WideToUTF8(it->first);
    TUtf16String space; space.push_back(' ');
    for (THashMap<TUtf16String, TVector<int> >::const_iterator it = fragment2uid.begin(); it != fragment2uid.end(); ++it) {
        for (size_t n = 0; n < it->second.size(); ++n)
            Fragment2Uid.AddWtroka(space + it->first + space, it->second[n]);
    }
    Fragment2Uid.Finalize();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TRecognizeExplicitIntentWorker {
    int NumMatches = 0;
    int Uid = -1;
    TRecognizeExplicitIntentWorker() = default;

    bool Do(int uid, size_t) {
        if (NumMatches == 0) {
            NumMatches = 1;
            Uid = uid;
        } else if (Uid != uid) {
            NumMatches = 2;
            return true;
        }
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TExplicitIntentsDetector::RecognizeExplicitIntent(const TStringBuf &reqUtf, TString *uid) const {
    TRecognizeExplicitIntentWorker worker;
    Fragment2Uid.Do(" ", reqUtf, " ", &worker);
    if (worker.NumMatches == 1) {
        *uid = Uids[worker.Uid];
        return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TExplicitIntentsDetector::AddToRecognizeSubset(const TStringBuf &intentName, TSubset *res) const {
    for (size_t n = 0; n < Uids.size(); ++n) {
        if (Uids[n] == intentName) {
            if (res->size() < Uids.size())
                res->resize(Uids.size(), false);
            (*res)[n] = true;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TRecognizeAllWorker {
    const TVector<TString> &Uids;
    const TVector<bool> &Subset;
    TVector<TString> *Res;
    TRecognizeAllWorker(const TVector<TString> &uids, const TVector<bool> &subset, TVector<TString> *res)
        : Uids(uids)
        , Subset(subset)
        , Res(res) {
    }

    bool Do(int uid, size_t) {
        if (Subset[uid])
            AddUniq(*Res, Uids[uid]);
        return false;
    }
};
void TExplicitIntentsDetector::RecognizeAll(const TStringBuf &reqUtf, const TSubset &subset, TVector<TString> *res) const {
    TRecognizeAllWorker worker(Uids, subset, res);
    Fragment2Uid.Do(" ", reqUtf, " ", &worker);
}
