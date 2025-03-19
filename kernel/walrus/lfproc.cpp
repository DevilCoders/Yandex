#include <kernel/search_types/search_types.h>
#include <util/system/yassert.h>
#include <util/generic/algorithm.h>

#include "lfproc.h"

namespace NIndexerCore {

struct SCmpStrings {
    const TFormAndCount *pArr;
    SCmpStrings(const TVector<TFormAndCount> *_pArr)
        : pArr(&(*_pArr)[0])
    {}
    bool operator()(int a, int b) const {
        return strcmp(pArr[a].Form, pArr[b].Form) < 0;
    }
};

struct SCmpCounts {
    const TFormAndCount *pArr;
    SCmpCounts(const TVector<TFormAndCount> *_pArr)
        : pArr(&(*_pArr)[0])
    {}
    bool operator()(int a, int b) const {
        return pArr[a].FormCount > pArr[b].FormCount ||
               (pArr[a].FormCount == pArr[b].FormCount && strcmp(pArr[a].Form, pArr[b].Form) < 0);
    }
};

struct SCmpConstChars {
    bool operator()(const char *pszA, const char *pszB) const { return strcmp(pszA, pszB) < 0; }
};

static void RearrangeFormIdx(const TVector<TFormAndCount>& formCounts, const TKeyLemmaInfo& curLemma, TVector<int>& formIdx) {
    const int N_KEY_BUF_SIZE = MAXKEY_BUF * 2;
    int nDst = 0;
    for (size_t i = 0; i < formIdx.size(); ++i) {
        int nForm = formIdx[i];
        if (formCounts[nForm].FormCount == 0)
            continue;
        formIdx[nDst++] = nForm;
    }
    formIdx.resize(nDst);

    // select most probable N for the first key
    if (formIdx.size() > N_MAX_FORMS_PER_KISHKA) {
        TVector<int> mostProbable = formIdx;
        Sort(mostProbable.begin(), mostProbable.end(), SCmpCounts(&formCounts));

        // determine word form count (limited by max key size)
        int nMost = Min((int)N_MAX_FORMS_PER_KISHKA, (int)mostProbable.size());
        for (; nMost >= 1; --nMost) {
            const char *szForms[N_MAX_FORMS_PER_KISHKA];
            for (int n = 0; n < nMost; ++n)
                szForms[n] = formCounts[mostProbable[n]].Form;
            Sort(szForms, szForms + nMost, SCmpConstChars());
            char szBuf[N_KEY_BUF_SIZE];
            int nLength = ConstructKeyWithForms(szBuf, N_KEY_BUF_SIZE, curLemma, nMost, szForms);
            if (nLength <= MAXKEY_BUF)
                break;
        }

        // sort forms for the first key with most probable word forms
        mostProbable.resize(nMost);
        Sort(mostProbable.begin(), mostProbable.end(), SCmpStrings(&formCounts));

        // add rest forms
        TVector<char> takenForms(formCounts.size(), 0);
        for (int k = 0; k < (int)mostProbable.size(); ++k)
            takenForms[mostProbable[k]] = 1;
        for (int k = 0; k < (int)formIdx.size(); ++k) {
            int nForm = formIdx[k];
            if (!takenForms[nForm])
                mostProbable.push_back(nForm);
        }
        formIdx.swap(mostProbable);
    }
}

struct SCmpOutputKeys {
    bool operator()(const TOutputKey &a, const TOutputKey &b) const { return strcmp(a.Key, b.Key) < 0; }
};

void TLemmaAndFormsProcessor::ConstructOutKeys(TVector<TOutputKey>& outKeys, bool need_rearrange) {
    size_t nTotalForms = FormCounts.size();

    if (!nTotalForms) {
        outKeys.resize(1);
        outKeys[0].KeyFormCount = 0;
        strcpy(outKeys[0].Key, CurLemma.szPrefix);
        strcat(outKeys[0].Key, CurLemma.szLemma);
        return;
    }

    TVector<int> formIdx;
    formIdx.resize(nTotalForms);
    for (size_t i = 0; i < nTotalForms; ++i)
        formIdx[i] = i;
    Sort(formIdx.begin(), formIdx.end(), SCmpStrings(&FormCounts));

    // filter not used word forms and put most frequent into first key
    if (need_rearrange) {
        RearrangeFormIdx(FormCounts, CurLemma, formIdx);
    }

    // prepare key list
    const int N_KEY_BUF_SIZE = MAXKEY_BUF * N_MAX_FORMS_PER_KISHKA;
    unsigned nResFormsCount = formIdx.size();
    outKeys.clear();
    outKeys.reserve(nResFormsCount * 2 / N_MAX_FORMS_PER_KISHKA);
    TTempBuf tempBuf(N_KEY_BUF_SIZE);
    char* szBuf = tempBuf.Data();
    for (unsigned nStart = 0; nStart < nResFormsCount;)
    {
        int nNumForms;
        int nMaxNumForms = Min(nResFormsCount - nStart, N_MAX_FORMS_PER_KISHKA);
        for (nNumForms = nMaxNumForms; nNumForms >= 0; --nNumForms) {
            const char *szForms[N_MAX_FORMS_PER_KISHKA];
            for (int n = 0; n < nNumForms; ++n)
                szForms[n] = FormCounts[formIdx[nStart + n]].Form;
            int nLength = ConstructKeyWithForms(szBuf, N_KEY_BUF_SIZE, CurLemma, nNumForms, szForms);
            if (nLength <= MAXKEY_BUF)
                break;
        }
        Y_ASSERT(nNumForms > 0);

        outKeys.resize(outKeys.size() + 1);
        TOutputKey& key = outKeys.back();
        for (int n = 0; n < nNumForms; ++n)
            key.FormIndexes[n] = formIdx[nStart + n];
        strcpy(key.Key, szBuf);
        key.KeyFormCount = nNumForms;

        nStart += nNumForms;
    }

    Sort(outKeys.begin(), outKeys.end(), SCmpOutputKeys());
}

void TLemmaAndFormsProcessor::FindFormIndexes(TNumFormArray& indexes) {
    static bool called = false;
    if (!called) {
        called = true;
    }
    memset(&indexes, 0, sizeof(indexes));
    for (int i = 0; i < CurFormCount; ++i) {
        int nFormIdx = -1;
        const char* pszForm = CurForms[i];
        Req++;
        if (UseHash) {
            THashMap<TKey, ui32, key_hash, key_equal_to>::iterator it = FormHash.find(pszForm);
            if (it != FormHash.end())
                nFormIdx = it->second;
        }
        else {
            for (size_t k = 0; k < FormCounts.size(); ++k) {
                if (strcmp(FormCounts[k].Form, pszForm) == 0) {
                    nFormIdx = k;
                    break;
                }
            }
        }
        if (nFormIdx == -1) {
            nFormIdx = FormCounts.size();
            FormCounts.push_back(TFormAndCount(pszForm));
            if (UseHash)
                FormHash[pszForm] = nFormIdx;
        }
//        if (strcmp(FormCounts[nFormIdx].Form, pszForm))
//            fprintf(stderr, "Wrong form found index %u size %u %s %s\n", nFormIdx, (ui32)FormCounts.size(), pszForm, FormCounts[nFormIdx].Form);
        Reqs += nFormIdx + 1;
        indexes[i] = nFormIdx;
    }
}

int TLemmaAndFormsProcessor::FindFormIndex(int form) {
    int nFormIdx = -1;
    const char* pszForm = CurForms[form];
    Req++;
    if (UseHash) {
        ui32& idx = FormHash[pszForm];
        if (idx == 0) {
            idx = FormCounts.size() + 1;
            FormCounts.push_back(TFormAndCount(pszForm));
        }
        Reqs += idx;
        return idx - 1;
//        THashMap<TKey, ui32, key_hash, key_equal_to>::iterator it = FormHash.find(pszForm);
//        if (it != FormHash.end())
//            nFormIdx = it->second;
    }
    else {
        for (size_t k = 0; k < FormCounts.size(); ++k) {
            if (strcmp(FormCounts[k].Form, pszForm) == 0) {
                nFormIdx = k;
                break;
            }
        }
    }
    if (nFormIdx == -1) {
        nFormIdx = FormCounts.size();
        FormCounts.push_back(TFormAndCount(pszForm));
//        if (UseHash)
//            FormHash[pszForm] = nFormIdx;
    }
    Reqs += nFormIdx + 1;
    return nFormIdx;
}

}
