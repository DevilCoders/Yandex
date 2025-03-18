#include <kernel/search_types/search_types.h>
#include "indexconvert.h"

#include <ysite/yandex/common/prepattr.h>
#include <ysite/yandex/posfilter/formflags.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/hitlist/positerator.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/searchfile.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <library/cpp/wordpos/wordpos.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/token/nlptypes.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/containers/mh_heap/mh_heap.h>
#include <util/generic/set.h>
#include <util/stream/file.h>

#include <algorithm>
#include <string>
#include <vector>

struct TIndexEntry {
    ui64 Offset;
    ui64 Length;
    ui32 Counter;
    ui32 FormMask;
    TIndexEntry() {
        memset(this, 0, sizeof(*this));
    }
};

struct TTheEnd {
    TString End;
    TTheEnd() {
        for (size_t i = 0; i < MAXKEY_BUF; ++i)
            End.append(0xff);
    }
} theEnd;

TIndexEntry &FindEntry(TVector<TIndexEntry> &entry, ui64 offset, ui64 length, ui32 counter) {
    for (size_t i = 0; i < entry.size(); ++i) {
        if (entry[i].Offset == offset && entry[i].Length == length && entry[i].Counter == counter) {
            return entry[i];
        }
    }
    entry.emplace_back();
    entry.back().Offset = offset;
    entry.back().Length = length;
    entry.back().Counter = counter;
    return entry.back();
}

void ReadEntry(TYndex4Searching *pIndex, const TVector<TIndexEntry> &entry, TVector<SUPERLONG> &positions) {
    positions.clear();
    for (size_t i = 0; i < entry.size(); ++i) {
        TPosIterator<> it;
        it.Init(*pIndex, entry[i].Offset, entry[i].Length, entry[i].Counter, RH_DEFAULT);
        SUPERLONG take = entry[i].FormMask;
        while (it.Valid()) {
            SUPERLONG curr = it.Current();
            SUPERLONG mask = curr & 0xf;
            if (take & (1 << mask)) {
                positions.push_back(curr - mask);
            }
            it.Next();
        }
    }
    std::sort(positions.begin(), positions.end());
    positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
}

struct TLemmaIterator {
    virtual bool Valid() const = 0;
    virtual void Init(TYndex4Searching *pIndex) = 0;
    virtual size_t Size() const = 0;
    virtual TString GetCurrentPrefix()  = 0;
    virtual bool Process(TVector<SUPERLONG> &hits) = 0;
};

struct TSequentialMap : public TLemmaIterator {
    TRequestContext RC;
    TVector<ui32> Indices;
    TVector<ui32>::const_iterator Begin;
    TYndex4Searching *Index;

    void Add(ui32 index) {
        Indices.push_back(index);
    }

    bool Valid() const override {
        return Begin != Indices.end();
    }
    void Init(TYndex4Searching *pIndex) override {
        Index = pIndex;
        Begin = Indices.begin();
    }
    size_t Size() const override {
        return Indices.size();
    }
    TString GetCurrentPrefix() override {
        if (Valid()) {
            const YxRecord *rec = EntryByNumber(Index, RC, *Begin);
            return rec->TextPointer;
        }
        return theEnd.End;
    }
    bool Process(TVector<SUPERLONG> &dst) override {
        if (!Valid()) {
            return false;
        }
        const YxRecord *rec = EntryByNumber(Index, RC, *Begin);
        TPosIterator<> it;
        it.Init(*Index, rec->Offset, rec->Length, rec->Counter, RH_DEFAULT);
        while (it.Valid()) {
            dst.push_back(it.Current());
            it.Next();
        }
        ++Begin;
        return true;
    }
};

struct TLemmaMap : public TLemmaIterator {
    //map form->index_entries
    TMap<TString, TVector<TIndexEntry> > Forms;
    //map (lang + lemma)->forms
    TMap<TString, TSet<TString> > Lemmas;
    TMap<TString, TSet<TString> >::const_iterator Begin;
    TMap<TString, TVector<SUPERLONG> > PendingKishkas;
    TMap<TString, TVector<SUPERLONG> >::iterator BeginKishkas;
    TYndex4Searching *Index;

    void FillPending() {
        if (!Valid()) {
            return;
        }
        PendingKishkas.clear();
        TKeyLemmaInfo keyLemma;
        TString lemma = Begin->first;
        char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
        int nTestForms = DecodeKey(lemma.c_str(), &keyLemma, forms);
        if (nTestForms != 1) {
            ythrow yexception() << "nTestForms != 1";
        }

        TMap<TString, TVector<SUPERLONG> > kishkas;
        TVector<std::pair<double, TString> > toSort;
        for (TSet<TString>::const_iterator it = Begin->second.begin(); it != Begin->second.end(); ++it) {
            const TString &form = *it;
            TVector<SUPERLONG> &positions = kishkas[form];
            ReadEntry(Index, Forms[form], kishkas[form]);
            if (positions.size()) {
                toSort.push_back(std::pair<double, TString>(1.0 / positions.size(), form));
            }
        }
        std::sort(toSort.begin(), toSort.end());

        const char *szForms[N_MAX_FORMS_PER_KISHKA];
        const size_t N_BUF_SIZE = MAXKEY_BUF * N_MAX_FORMS_PER_KISHKA;
        char szKeyBuf[N_BUF_SIZE];
        size_t j = 0;
        while (1) {
            size_t i = 0;
            for (; i < N_MAX_FORMS_PER_KISHKA && j + i < toSort.size(); ++i) {
                szForms[i] = toSort[j + i].second.c_str();
                int nLeng = ConstructKeyWithForms(szKeyBuf, N_BUF_SIZE, keyLemma, i + 1, szForms);
                if (nLeng >= MAXKEY_BUF) {
                    if (i == 0) {
                       ythrow yexception() << "i == 0";
                    }
                    break;
                }
            }
            ConstructKeyWithForms(szKeyBuf, N_BUF_SIZE, keyLemma, i, szForms);
            TVector<SUPERLONG> dst;
            for (size_t k = 0; k < i; ++k) {
                TVector<SUPERLONG> &src = kishkas[toSort[j + k].second];
                for (size_t l = 0; l < src.size(); ++l) {
                    dst.push_back(src[l] + k);
                }
            }
            std::sort(dst.begin(), dst.end());
            if (dst.size()) {
                PendingKishkas[szKeyBuf].swap(dst);
            }
            j += i;
            if (j == toSort.size()) {
                break;
            }
        }
        BeginKishkas = PendingKishkas.begin();
    }

    void Init(TYndex4Searching *pIndex) override {
        Index = pIndex;
        Begin = Lemmas.begin();
        FillPending();
    }

    size_t Size() const override {
        return Lemmas.size();
    }

    bool Valid() const override {
        return Begin != Lemmas.end();
    }

    TString GetCurrentPrefix() override {
        while (1) {
            if (Valid()) {
                if (BeginKishkas == PendingKishkas.end()) {
                    ++Begin;
                    FillPending();
                } else {
                    return BeginKishkas->first;
                }
            } else {
                break;
            }
        }
        return theEnd.End;
    }

    bool Process(TVector<SUPERLONG> &dst) override {
        if (!Valid()) {
            return false;
        }
        dst.swap(BeginKishkas->second);
        ++BeginKishkas;
        return true;
    }

    TVector<TIndexEntry> &FindFormVec(ELanguage lang, const TString &lemma, const TString &form) {
        TVector<TIndexEntry> &formVec = Forms[form];
        TKeyLemmaInfo keyLemma;
        strcpy(keyLemma.szLemma, lemma.c_str());
        keyLemma.Lang = lang;
        const char *szForms[N_MAX_FORMS_PER_KISHKA];

        szForms[0] = lemma.c_str();
        const size_t N_BUF_SIZE = MAXKEY_BUF * N_MAX_FORMS_PER_KISHKA;
        char szKeyBuf[N_BUF_SIZE];
        int nLeng = ConstructKeyWithForms(szKeyBuf, N_BUF_SIZE, keyLemma, 1, szForms);
        if (nLeng >= MAXKEY_BUF) {
            ythrow yexception() << "nLeng >= MAXKEY_BUF";
        }
        Lemmas[szKeyBuf].insert(form);
        return formVec;
    }
};

static void ConvertLemmatize(TYndex4Searching *pIndex, NIndexerCore::TOldIndexFile* dstIndex, const TLemmatizer &lemmatizer, const bool verbose) {
    TRequestContext rc;
    ui32 nKey = 0;
    TVector<TWordLanguage> res;
    TWLemmaArray buffer;
    TKeyLemmaInfo keyLemma;
    char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    TUtf16String::TChar wbuff[MAXWORD_BUF];
    char lemmabuf[MAXKEY_BUF];
    TLemmaMap lemmas;
    TSequentialMap seq;
    int proc = -1;
    int mpos = -1;

    double positionIn = 0.0;
    double positionOut = 0.0;
    while (nKey < pIndex->KeyCount()) {
        int nproc = (int)(100.0 * (nKey + 1.0) / Max((double)pIndex->KeyCount(), 1.0));
        int npos = (int)(positionIn / 1000.0 / 100.0);
        if (verbose && (proc != nproc || npos != mpos)) {
            Cout << "\r";
            Cout << "First run " << (proc = nproc) << "%" << " Mposition in " << (int)(positionIn / 1000.0 / 1000.0);
            Cout.Flush();
            mpos = npos;
        }
        const YxRecord *rec = EntryByNumber(pIndex, rc, nKey);
        int nTestForms = DecodeKey(rec->TextPointer, &keyLemma, forms);
        positionIn += rec->Counter;
        if (lemmatizer.NeedLemmatizing(keyLemma, forms, nTestForms)) {
            if (verbose && keyLemma.szPrefix[0] != 0)
                Cout << (int)keyLemma.szPrefix[0] << "\n";
            for (int i = 0; i < nTestForms; ++i) {
                size_t in = 0, out = 0;
                TString form(forms[i]);
                int len = form.size();
                ui8 flags = 0, joins = 0, lang = LANG_UNK;
                RemoveFormFlags(forms[i], &len, &flags, &joins, &lang);
                if (keyLemma.Lang != LANG_UNK) // old language format
                    lang = keyLemma.Lang;
                if (keyLemma.Lang != LANG_UNK || lemmatizer.NeedLemmatizing(keyLemma, form, (ELanguage)lang)) {
                    bool isutf = UTF8_FIRST_CHAR == *forms[i];
                    RecodeToUnicode(isutf ? CODES_UTF8 : CODES_YANDEX, forms[i] + isutf, wbuff, len - isutf, MAXWORD_BUF, in, out);
                    lemmatizer.LemmatizeIndexWord(TWtringBuf(wbuff, wbuff + out), (ELanguage)lang, res, buffer);
                    for (size_t j = 0; j < res.size(); ++j) {
                        TFormToKeyConvertor lemmaConv(lemmabuf, MAXKEY_BUF);
                        lemmaConv.Convert(res[j].LemmaText, res[j].LemmaLen);
                        TVector<TIndexEntry> &records = lemmas.FindFormVec(res[j].Language, TString(lemmabuf), form);
                        TIndexEntry &entry = FindEntry(records, rec->Offset, rec->Length, rec->Counter);
                        entry.FormMask |= (1 << i);
                    }
                } else { // no relemmatization, keep old lemma
                    TVector<TIndexEntry> &records = lemmas.FindFormVec((ELanguage)lang, TString(keyLemma.szLemma), form);
                    TIndexEntry &entry = FindEntry(records, rec->Offset, rec->Length, rec->Counter);
                    entry.FormMask |= (1 << i);
                }
            }
        } else {
            seq.Add(nKey);
        }
        ++nKey;
    }

    if (verbose)
        Cout << "\nFirst run completed " << " Mposition in " << positionIn / 1000.0 / 1000.0 << "\n";

    lemmas.Init(pIndex);
    seq.Init(pIndex);
    TLemmaIterator &t0 = lemmas;
    TLemmaIterator &t1 = seq;
    proc = -1;
    mpos = -1;
    double count = t0.Size() + t1.Size();
    nKey = 0;
    TString start;
    while (t0.Valid() || t1.Valid()) {
        TString s0 = t0.GetCurrentPrefix();
        TString s1 = t1.GetCurrentPrefix();
        if (!t0.Valid() && !t1.Valid()) {
            break;
        }
        int nproc = (int)(100.0 * (nKey + 1.0) / count);
        int npos = (int)(positionIn / 1000.0 / 100.0);
        if (verbose && (proc != nproc || npos != mpos)) {
            Cout << "\r";
            Cout << "Second run " << (proc = nproc) << "%" << " Mposition out " << (int)(positionOut / 1000.0 / 1000.0);
            Cout.Flush();
            mpos = npos;
        }
        if (s0 < s1) {
            TVector<SUPERLONG> dst;
            t0.Process(dst);
            dstIndex->StorePositions(s0.c_str(), &dst[0], dst.size());
            if (s0 <= start) {
                ythrow yexception() << "s0 <= start\n" << nKey << "\n" << s0 <<  "\n" << start << "\n";
            }
            positionOut += dst.size();
            start = s0;
            ++nKey;
        } else if (s0 > s1) {
            TVector<SUPERLONG> dst;
            t1.Process(dst);
            dstIndex->StorePositions(s1.c_str(), &dst[0], dst.size());
            if (s1 <= start) {
                ythrow yexception() << "s1 <= start\n" << nKey << "\n"<< s1 << "\n" << start << "\n";
            }
            positionOut += dst.size();
            start = s1;
            ++nKey;
        } else {
            TVector<SUPERLONG> dst1;
            t1.Process(dst1);
            TVector<SUPERLONG> dst0;
            t0.Process(dst0);
            for (size_t i = 0; i < dst1.size(); ++i) {
                dst0.push_back(dst1[i]);
            }
            if (s0 <= start) {
                ythrow yexception() << "s0, s1 <= start";
            }
            start = s0;
            std::sort(dst0.begin(), dst0.end());
            dst0.erase(std::unique(dst0.begin(), dst0.end()), dst0.end());
            dstIndex->StorePositions(s0.c_str(), &dst0[0], dst0.size());
            positionOut += dst0.size();
            ++nKey;
            ++nKey;
        }
    }
    if (verbose)
        Cout << "\nSecond run completed " << " Mposition out " << positionOut / 1000.0 / 1000.0 << "\n";
}


int IndexConvert(const char *from, const char *to, const TLemmatizer &lemmatizer, const bool verbose) {
    TYndex4Searching yndex;
    yndex.InitSearch(from);
    ui32 targetVersion =
        YNDEX_VERSION_BLK8 +
        YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY +
        YNDEX_VERSION_FLAG_KEY_2K;

    NIndexerCore::TOldIndexFile dstIndex(IYndexStorage::FINAL_FORMAT, targetVersion);
    dstIndex.Open(to);

    ConvertLemmatize(&yndex, &dstIndex, lemmatizer, verbose);
    dstIndex.CloseEx();

    return 0;
}
