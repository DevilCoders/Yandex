#pragma once

#include "dains.h"
#include <library/cpp/charset/ci_string.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <kernel/keyinv/invkeypos/keychars.h>

class TDocAttrInserter : public NIndexerCore::TInserterToDocAttrs {
private:
    struct TAttrKey {
        TCiString Name;
        ui32 Type;
        TAttrKey() : Type(0) {}
        TAttrKey(ui32 type, const char* name) : Name(name), Type(type) {}
        struct THash {
            size_t operator()(const TAttrKey& k) const {
                return ComputeHash(k.Name) + k.Type;
            }
        };
        bool operator ==(const TAttrKey& y) const {
            return Name == y.Name && Type == y.Type;
        }
    };
    typedef THashMap< TAttrKey, TVector<ui8>, TAttrKey::THash, TEqualTo<TAttrKey> > TAttrs;
    TAttrs Attrs;
private:
    template <class T>
    void StoreData(ui32 type, const char* name, const T* value, size_t length, TPosting pos) {
        TVector<ui8>& ae = Attrs[TAttrKey(type, name)];
        size_t oldSize = ae.size();
        ui16 len = (ui16)length;
        ae.yresize(oldSize + sizeof(TPosting) + sizeof(ui16) + (len + 1) * sizeof(T));
        ui8* beg = &ae[0] + oldSize;
        memcpy(beg, &pos, sizeof(TPosting));
        beg += sizeof(TPosting);
        memcpy(beg, &len, sizeof(ui16));
        beg += sizeof(ui16);
        if (len) {
            memcpy(beg, value, len * sizeof(T));
            beg += len * sizeof(T);
        }
        *((T*)beg) = 0; // null-terminator
    }
public:
    TDocAttrInserter(TFullDocAttrs* docAttrs)
        : TInserterToDocAttrs(docAttrs)
    {}
    ~TDocAttrInserter() override {
        Flush();
    }

    void Flush() {
        for (TAttrs::const_iterator i = Attrs.begin(); i != Attrs.end(); ++i) {
            const TVector<ui8>& ae = i->second;
            TString attrValue((const char*)&ae[0], ae.size());
            DocAttrs->AddAttr(i->first.Name.data(), attrValue, i->first.Type);
        }
        Attrs.clear();
    }

    void StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) override {
        StoreData(TFullDocAttrs::AttrSearchLitPos, attrName, attrText, strlen(attrText), pos);
    }
    void StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) override {
        TString v = WideToUTF8(attrText, len);
        v.prepend(UTF8_FIRST_CHAR);
        StoreLiteralAttr(attrName, v.data(), pos);
    }
    void StoreDateTimeAttr(const char* attrName, time_t datetime) override {
        DocAttrs->AddAttr(attrName, ToString(datetime), TFullDocAttrs::AttrSearchDate);
    }
    void StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) override {
        StoreData(TFullDocAttrs::AttrSearchIntPos, attrName, attrText, strlen(attrText), pos);
    }
    void StoreKey(const char* key, TPosting pos) override {
        StoreData(TFullDocAttrs::AttrSearchLitPos, key, (const char*)nullptr, 0, pos);
    }
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) override {
        ui32 type = TFullDocAttrs::AttrArcZones;
        if (!archiveOnly)
            type |= TFullDocAttrs::AttrSearchZones;
        TVector<ui8>& ae = Attrs[TAttrKey(type, zoneName)];
        size_t oldSize = ae.size();
        ae.yresize(oldSize + 2 * sizeof(TPosting));
        ui8* beg = &ae[0] + oldSize;
        memcpy(beg, &begin, sizeof(TPosting));
        memcpy(beg + sizeof(TPosting), &end, sizeof(TPosting));
    }
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) override {
        StoreData(TFullDocAttrs::AttrArcAttrs, name, value, length, pos);
    }
    void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) override {
        TString l = WideToUTF8(lemma, lemmaLen);
        l.prepend(UTF8_FIRST_CHAR);
        TString f = WideToUTF8(form, formLen);
        TVector<ui8>& ae = Attrs[TAttrKey(TFullDocAttrs::AttrSearchLemma, l.data())];
        size_t oldSize = ae.size();
        ui16 len = (ui16)f.length();
        ae.yresize(oldSize + sizeof(TPosting) + 2 * sizeof(ui8) + sizeof(ui16) + len);
        ui8* beg = &ae[0] + oldSize;
        memcpy(beg, &pos, sizeof(TPosting));
        beg += sizeof(TPosting);
        memcpy(beg, &flags, sizeof(ui8));
        beg += sizeof(ui8);
        char t = (ui8)lang;
        memcpy(beg, &t, sizeof(ui8));
        beg += sizeof(ui8);
        memcpy(beg, &len, sizeof(ui16));
        beg += sizeof(ui16);
        memcpy(beg, f.data(), len);
    }
};

template <typename T>
void DecodeZoneValue(const char* data, size_t size, const T& handler) {
    Y_ASSERT(size % (sizeof(TPosting) * 2) == 0);
    const TPosting* postings = reinterpret_cast<const TPosting*>(data);
    const TPosting* endOfPostings = reinterpret_cast<const TPosting*>(data + size);
    while (postings != endOfPostings) {
        handler(postings[0], postings[1]);
        postings += 2;
    }
}

void StoreExtSearchData(const TFullDocAttrs* docAttrs, IDocumentDataInserter& inserter);

