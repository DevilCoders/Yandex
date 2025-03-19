#include "directins.h"
#include "invcreator.h"
#include <library/cpp/charset/wide.h>

namespace NIndexerCore {

TInvCreatorInserter::TInvCreatorInserter(TInvCreator& creator, TFullDocAttrs* docAttrs)
    : TInserterToDocAttrs(docAttrs)
    , Creator(creator)
{
}

void TInvCreatorInserter::StoreLiteralAttr(const char* , const char* , TPosting ) {
    Y_FAIL("Not Implemented");
}

void TInvCreatorInserter::StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) {
    TUtf16String wvalue(attrText, len);
    Creator.StoreAttr(DTAttrSearchLiteral, attrName, wvalue.data(), pos);
}

void TInvCreatorInserter::StoreDateTimeAttr(const char* attrName, time_t datetime) {
    TUtf16String wvalue = ASCIIToWide(ToString(datetime));
    Creator.StoreAttr(DTAttrSearchDate, attrName, wvalue.data(), 0); // the last argument is unused here
}

void TInvCreatorInserter::StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) {
    TUtf16String wvalue = CharToWide(attrText, csYandex);
    Creator.StoreAttr(DTAttrSearchInteger, attrName, wvalue.data(), pos);
}

void TInvCreatorInserter::StoreKey(const char* key, TPosting pos) {
    Creator.StoreKey(key, pos);
}

void TInvCreatorInserter::StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) {
    if (!archiveOnly)
        Creator.StoreZone(zoneName, begin, end);
}

void TInvCreatorInserter::StoreArchiveZoneAttr(const char *, const wchar16* , size_t , TPosting ) {
    Y_FAIL("Not Implemented");
}

void TInvCreatorInserter::StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) {
    Creator.StoreExternalLemma(lemma, lemmaLen, form, formLen, flags, (ui8)lang, pos);
}

}
