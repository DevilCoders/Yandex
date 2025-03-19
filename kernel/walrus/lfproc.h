#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>

typedef int TNumFormArray[N_MAX_FORMS_PER_KISHKA];

namespace NIndexerCore {


struct TFormAndCount {
    char Form[MAXKEY_BUF];
    int FormCount;
    TFormAndCount()
        : FormCount(0)
    {}
    explicit TFormAndCount(const char *c)
        : FormCount(0)
    {
        strcpy(Form, c);
    }
};

struct TOutputKey {
    TNumFormArray FormIndexes;
    char Key[MAXKEY_BUF];
    int KeyFormCount;
};

////
// Merging indexes with lemma+forms - packed keys
// Reading:
// R1. Call ProcessNextKey() for each key from heap top.
//     When it returns true, run saving procedure, then Flush().
// R2. Call FindFormIndexes()
// R3. Remember all hits for this key, assign current form indexes to them.
//     Read each hit, counting the amount of positions that are passed to
//     the new index for each form. This is Pass 1 of input reading.
// Writing:
// WA. When no forms detected, we can write using old simple procedure
// W1. Call ConstructOutKeys()
// W2. For each key constructed, prepare form remapping array, read hits heap
//     and write hits with remapped form nums. This is pass 2+
//     (one pass for each outKey, usually just one).

class TLemmaAndFormsProcessor {
private:
    class TKey {
    public:
        ui32 Len;
        char Key[MAXKEY_BUF];

    public:
        TKey() {
        }

        TKey(const char* key) {
            Len = strlen(key);
            memcpy(Key, key, Len + 1);
        }

        TKey(const TKey& key) {
            Len = key.Len;
            memcpy(Key, key.Key, Len + 1);
        }

        TKey& operator=(const char* key) {
            Len = strlen(key);
            memcpy(Key, key, Len + 1);
            return *this;
        }

        TKey& operator=(const TKey& key) {
            Len = key.Len;
            memcpy(Key, key.Key, Len + 1);
            return *this;
        }
    };

    struct key_equal_to {
        inline bool operator()(const TKey& x, const char* y) const {
            ui32 len = strlen(y);
            return (len == x.Len) && (memcmp(x.Key, y, len) == 0);
        }

        inline bool operator()(const TKey& x, const TKey& y) const {
            return (x.Len == y.Len) && (memcmp(x.Key, y.Key, x.Len) == 0);
        }

    };

    struct key_hash {
        inline size_t operator()(const char* s) const {
            return ComputeHash(TStringBuf(s, strlen(s)));
        }
        inline size_t operator()(const TKey& s) const {
            return ComputeHash(TStringBuf(s.Key, s.Len));
        }
    };

private:
    TKeyLemmaInfo CurLemma;
    TVector<TFormAndCount> FormCounts;
    THashMap<TKey, ui32, key_hash, key_equal_to> FormHash;
    TKeyLemmaInfo NextLemma;
    ui64 Req;
    ui64 Reqs;
    char CurForms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    int CurFormCount;
    const bool RawKeys;
    const bool UseHash;
public:
    /**
     * \param rawKeys                   Treat input keys as raw keys.
     * \param stripKeys                 Convert input keys into stripped format when decoding.
     *                                  Note that in case of raw keys there is nothing to convert.
     *
     * \see ConvertKeyToStrippedFormat
     */
    explicit TLemmaAndFormsProcessor(bool rawKeys = false, bool stripKeys = true, bool useHash = false)
        : Req(0)
        , Reqs(0)
        , RawKeys(rawKeys)
        , UseHash(useHash)
    {
        Y_UNUSED(stripKeys);
    }
    ~TLemmaAndFormsProcessor() {
    }
    size_t GetTotalFormCount() const {
        return FormCounts.size();
    }
    int GetCurFormCount() const {
        return CurFormCount;
    }
    const TKeyLemmaInfo& GetCurLemma() const {
        return CurLemma;
    }
    void IncreaseFormCount(int formIndex) {
        Y_ASSERT(!RawKeys);
        FormCounts[formIndex].FormCount++;
    }
    void IncreaseFormCount(int formIndex, ui32 inc) {
        Y_ASSERT(!RawKeys);
        FormCounts[formIndex].FormCount += inc;
    }
    void SetFormCount(int formIndex, int count) {
        Y_ASSERT(!RawKeys);
        FormCounts[formIndex].FormCount = count;
    }
    int FormCount(int formIndex) {
        return FormCounts[formIndex].FormCount;
    }

    // input lemma processing (raw_key => no processing)
    bool ProcessNextKey(const char* key) {
        if (RawKeys) {
            CurFormCount = DecodeRawKey(key, &NextLemma); // always return 0
        } else {
            int lengths[N_MAX_FORMS_PER_KISHKA];
            CurFormCount = DecodeKey(key, &NextLemma, CurForms, lengths);
            ConvertKeyToStrippedFormat(&NextLemma, CurForms, lengths, CurFormCount);
        }
        return CurLemma != NextLemma;
    }
    void PrepareForNextLemma() {
        static THashMap<TKey, ui32, key_hash, key_equal_to> EmptyHash(17);
        CurLemma = NextLemma;
        FormCounts.clear();
        if (UseHash)
            FormHash = EmptyHash;
    }
    // create forms mapping FormIndexes from single file to a global array
    void FindFormIndexes(TNumFormArray& indexes);
    int FindFormIndex(int form);
    // output lemma and forms processing
    void ConstructOutKeys(TVector<TOutputKey>& outKeys, bool need_rearrange);
};

}
