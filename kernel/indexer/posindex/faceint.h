#pragma once

#include "memindex.h"

#include <cstddef>
#include <cstring>
#include <util/system/defaults.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <utility>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

typedef TListType<ui32> TPostings; // список позиций

// Ключу индекса TWordKey соответствует список TDocPostingsList,
// каждый элемент (TDocPostings) которого представляет один документ
// и в свою очередь является списком позиций.

struct TDocPostings {
    size_t     DocOffset;  // номер по-порядку документа в порции
    TPostings  Postings;   // список позиции в рамках одного документа
};

typedef TListType<TDocPostings> TDocPostingsList;

struct TWordKey {
    const char* LemmaText;
    const char* FormaText;
    ui8 Lang;
    ui8 Flags;
    ui8 Joins;

    TWordKey()
        : LemmaText(nullptr)
        , FormaText(nullptr)
        , Lang(0) // it's LANG_UNK
        , Flags(0)
        , Joins(0)
    {}

    bool operator ==(const TWordKey& other) const {
        return strcmp(LemmaText, other.LemmaText) == 0 && strcmp(FormaText, other.FormaText) == 0
            && Lang == other.Lang && Flags == other.Flags && Joins == other.Joins;
    }


    bool operator <(const TWordKey& other) const {
        int tmp = strcmp(LemmaText, other.LemmaText);
        if (tmp != 0) {
            return tmp < 0;
        }

        if (Lang != other.Lang) {
            return Lang < other.Lang;
        }

        tmp = strcmp(FormaText, other.FormaText);
        if (tmp != 0) {
            return tmp < 0;
        }

        if (Flags != other.Flags) {
            return Flags < other.Flags;
        }

        return Joins < other.Joins;
    }

    struct THash {
        size_t operator()(const TWordKey& ke) const {
            return ComputeHash(TStringBuf{ke.LemmaText, strlen(ke.LemmaText)}) +
                   ComputeHash(TStringBuf{ke.FormaText, strlen(ke.FormaText)}) +
                   ke.Lang + ke.Flags + ke.Joins;
        }
    };
};

struct TAttrKey {
    const char* Key; // текст ключа атрибута

    TAttrKey()
        : Key(nullptr)
    {}

    bool operator ==(const TAttrKey& other) const {
        return strcmp(Key, other.Key) == 0;
    }

    bool operator <(const TAttrKey& other) const {
        return strcmp(Key, other.Key) < 0;
    }

    struct THash {
        size_t operator()(const TAttrKey& ke) const {
            return ComputeHash(TStringBuf{ke.Key, strlen(ke.Key)});
        }
    };
};

typedef std::pair<const TWordKey, TDocPostingsList> TWordEntry;
typedef std::pair<const TAttrKey, TDocPostingsList> TAttrEntry;

struct TWordEntryWithFeedId {
    ui64 FeedId;
    const TWordEntry* WordEntry;
    TWordEntryWithFeedId(ui64 id, const TWordEntry* ent)
        : FeedId(id)
        , WordEntry(ent)
    {}
};

struct TAttrEntryWithFeedId {
    ui64 FeedId;
    const TAttrEntry* AttrEntry;
    TAttrEntryWithFeedId(ui64 id, const TAttrEntry* ent)
        : FeedId(id)
        , AttrEntry(ent)
    {}
};

typedef TVector<const TWordEntry*> TWordEntries;
typedef TVector<TWordEntryWithFeedId> TWordEntryWithFeedIds;
typedef TVector<const TAttrEntry*> TAttrEntries;
typedef TVector<TAttrEntryWithFeedId> TAttrEntryWithFeedIds;

#define MAXATTRNAME_BUF 32

}}
