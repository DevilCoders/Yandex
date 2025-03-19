#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

class IIndexDataInserter {
public:
    virtual ~IIndexDataInserter() {}
    // value is Yandex encoded
    virtual void StoreLiteralAttr(const char* name, const char* value, TPosting pos) = 0;
    virtual void StoreLiteralAttr(const char* name, const wchar16* value, TPosting pos) = 0;
    virtual void StoreDateTimeAttr(const char* name, time_t datetime) = 0;
    virtual void StoreIntegerAttr(const char* name, const char* value, TPosting pos) = 0;
    virtual void StoreUrlAttr(const char* name, const wchar16* value, TPosting pos) = 0; // TODO value should be const char*
    virtual void StoreKey(const char* key, TPosting pos) = 0;
    virtual void StoreZone(const char* name, TPosting beg, TPosting end) = 0;
    virtual void StoreExternalLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, ui8 lang, TPosting pos) = 0;
};

class IArchiveDataInserter {
public:
    virtual ~IArchiveDataInserter() {}
    virtual void StoreArchiveZone(const char* name, TPosting beg, TPosting end) = 0;
    virtual void StoreArchiveZoneAttr(const char* name, const wchar16* value, TPosting pos) = 0;
    // value is UTF8 encoded
    virtual void StoreArchiveDocAttr(const char* name, const char* value) = 0;
};

class IDocumentDataInserter {
public:
    virtual ~IDocumentDataInserter() {}
    // const char* attrText - в CODES_YANDEX
    // для документного атрибута pos == 0
    virtual void StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) = 0;
    virtual void StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) = 0;
    virtual void StoreDateTimeAttr(const char* attrName, time_t datetime) = 0;
    virtual void StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) = 0;
    // сохранение при работе нумератора ключей, не подлежащих лемматизации
    virtual void StoreKey(const char* key, TPosting pos) = 0;
    virtual void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly = false) = 0;
    virtual void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) = 0;
    virtual void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) = 0;
    virtual void StoreTextArchiveDocAttr(const TString& name, const TString& value) = 0;
    virtual void StoreFullArchiveDocAttr(const TString& name, const TString& value) = 0;
    virtual void StoreErfDocAttr(const TString& name, const TString& value) = 0;
    virtual void StoreGrpDocAttr(const TString& name, const TString& value, bool isInt) = 0;
};
