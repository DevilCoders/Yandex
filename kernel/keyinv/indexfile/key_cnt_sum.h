#pragma once

#include <kernel/search_types/search_types.h>
#include <algorithm>
#include <kernel/keyinv/hitlist/record.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include "indexfile.h"
#include "indexreader.h"

struct YxFilePlusRec {
    NIndexerCore::TInputIndexFile IndexFile;
    THolder<NIndexerCore::TInvKeyReader> IndexReader;
    TKeyLemmaInfo lemma;

    YxFilePlusRec(const char *fname = nullptr)
        : IndexFile(IYndexStorage::FINAL_FORMAT)
    {
        if (fname)
            init(fname);
    }

    void init(const char *fname) {
        IndexFile.OpenKeyFile(fname);
        IndexReader.Reset(new NIndexerCore::TInvKeyReader(IndexFile));
    }
    ~YxFilePlusRec() {}

    int next_k() {
        if (!IndexReader->ReadNext())
            return false;
        char dummy[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
        DecodeKey(IndexReader->GetKeyText(), &lemma, dummy);
        if (*lemma.szPrefix) {
            int llem = strlen(lemma.szLemma), lpref = strlen(lemma.szPrefix);
            memmove(lemma.szLemma + lpref, lemma.szLemma, llem + 1);
            memcpy(lemma.szLemma, lemma.szPrefix, lpref);
        }
        return true;
    }
};

// bad :) This is closely a copy-paste of heap_of_buf_hit_iters
struct Key_Cnt_Sum {

    struct yfk_ptr_less {
        bool operator() (const YxFilePlusRec *a, const YxFilePlusRec *b) const {
            return strcmp(a->lemma.szLemma, b->lemma.szLemma) > 0;
        }
    };

    TVector<YxFilePlusRec*> theheap;
    size_t cur_heap_size;
    YxRecord Rec;
    char const* str;

    Key_Cnt_Sum(YxFilePlusRec *arr, size_t numel) : theheap(numel), str(Rec.TextPointer) {
        cur_heap_size = 0;
        for(size_t n = 0; n < numel; n++) if (arr[n].next_k()) {
            theheap[cur_heap_size++] = arr + n;
        }
        std::make_heap(&theheap[0], &theheap[0] + cur_heap_size, yfk_ptr_less());
        *Rec.TextPointer = 0;
        Rec.Offset = 0;
        Rec.Length = 0;
    }

    size_t NextKey() {
        if (!cur_heap_size) {
            return 0;
        }
        YxFilePlusRec *r = theheap[0];
        i64 Sum = r->IndexReader->GetCount();
        std::pop_heap(&theheap[0], &theheap[0] + cur_heap_size, yfk_ptr_less());
        strcpy(Rec.TextPointer, r->lemma.szLemma);
        if (r->next_k())
            std::push_heap(&theheap[0], &theheap[0] + cur_heap_size, yfk_ptr_less());
        else
            cur_heap_size--;
        while(cur_heap_size && !strcmp(Rec.TextPointer, (r = theheap[0])->lemma.szLemma)) {
            std::pop_heap(&theheap[0], &theheap[0] + cur_heap_size, yfk_ptr_less());
            Sum += r->IndexReader->GetCount();
            if (r->next_k())
                std::push_heap(&theheap[0], &theheap[0] + cur_heap_size, yfk_ptr_less());
            else
                cur_heap_size--;
        }
        if (Sum >= 0x0fffffff) {
            warnx("Sum overflow for key %s (%" PRId64 " = 0x%" PRIx64 " > max signed int-)", Rec.TextPointer, Sum, Sum);
            Sum = 0x0fffffff;
        }
        Rec.Counter = (int)Sum;
        //printf("%s: %u / %u [%i]\n", Rec.TextPointer, cur_size, cur_heap_size, Rec.Counter);
        return 1;
    }
};
