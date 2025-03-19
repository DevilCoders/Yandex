#include "docattrinserter.h"
#include <library/cpp/charset/wide.h>
#include <kernel/search_types/search_types.h>
#include <ysite/yandex/common/prepattr.h>

namespace {

    class TZoneInserter {
        const char* const ZoneName;
        const bool ArchiveOnly;
        IDocumentDataInserter& Inserter;
    public:
        TZoneInserter(const char* zoneName, bool archiveOnly, IDocumentDataInserter& inserter)
            : ZoneName(zoneName)
            , ArchiveOnly(archiveOnly)
            , Inserter(inserter)
        {
        }
        void operator()(TPosting beg, TPosting end) const {
            Inserter.StoreZone(ZoneName, beg, end, ArchiveOnly);
        }
    };

} // namespace

void StoreExtSearchData(const TFullDocAttrs* docAttrs, IDocumentDataInserter& inserter) {
    for (TFullDocAttrs::TConstIterator i = docAttrs->Begin(); i != docAttrs->End(); ++i) {
        if (i->Type & TFullDocAttrs::AttrSearchZones || i->Type & TFullDocAttrs::AttrArcZones) {
            DecodeZoneValue(i->Value.c_str(), i->Value.size(),
                TZoneInserter(i->Name.c_str(), ((i->Type & TFullDocAttrs::AttrSearchZones) == 0), inserter));
            continue;
        }

        if (i->Type & TFullDocAttrs::AttrArcAttrs || i->Type & TFullDocAttrs::AttrSearchLitPos || i->Type & TFullDocAttrs::AttrSearchIntPos) {
            const char* beg = (const char*)i->Value.data();
            const char* end = beg + i->Value.size();
            while (beg < end) {
                TPosting pos = *((TPosting*)beg);
                beg += sizeof(TPosting);
                ui16 len = *((ui16*)beg);
                beg += sizeof(ui16);
                if (i->Type & TFullDocAttrs::AttrArcAttrs) {
                    Y_ASSERT(((const wchar16*)beg)[len] == 0); // value must be null-terminated
                    inserter.StoreArchiveZoneAttr(i->Name.data(), (const wchar16*)beg, len, pos);
                    beg += (len + 1) * sizeof(wchar16);
                } else if (i->Type & TFullDocAttrs::AttrSearchIntPos) {
                    Y_ASSERT(beg[len] == 0); // value must be null-terminated
                    inserter.StoreIntegerAttr(i->Name.data(), beg, pos);
                    beg += (len + 1);
                } else if (i->Type & TFullDocAttrs::AttrSearchLitPos) {
                    Y_ASSERT(beg[len] == 0); // value must be null-terminated
                    if (len == 0) {
                        inserter.StoreKey(i->Name.data(), pos);
                        beg += 1; // null-terminator only
                    } else {
                        if (*beg == UTF8_FIRST_CHAR) {
                            TTempArray<wchar16> buf(len);
                            wchar16* data = buf.Data();
                            size_t n = 0;
                            UTF8ToWide(beg + 1, len - 1, data, n); // TODO: remove conversion, use negative length to mark wchar16
                            inserter.StoreLiteralAttr(i->Name.data(), data, n, pos);
                        } else {
                            inserter.StoreLiteralAttr(i->Name.data(), beg, pos);
                        }
                        beg += len + 1;
                    }
                }
            }
            Y_ASSERT(beg == end);
            continue;
        }

        if (i->Type & TFullDocAttrs::AttrSearchLemma) {
            const char* beg = (const char*)i->Value.data();
            const char* end = beg + i->Value.size();
            while (beg < end) {
                TPosting pos = *((TPosting*)beg);
                beg += sizeof(TPosting);
                ui8 flag = *((ui8*)beg);
                beg += sizeof(ui8);
                ui8 lang = *((ui8*)beg);
                beg += sizeof(ui8);
                ui16 len = *((ui16*)beg);
                beg += sizeof(ui16);
                TString forma(beg, len);
                TString lemma = i->Name;
                TUtf16String l, f;
                if (*lemma.data() == UTF8_FIRST_CHAR) {
                    l = UTF8ToWide(lemma.data() + 1, lemma.size() - 1);
                    f = UTF8ToWide(forma);
                } else {
                    l = CharToWide(lemma, csYandex);
                    f = CharToWide(forma, csYandex);
                }
                inserter.StoreLemma(l.data(), l.size(), f.data(), f.size(), flag, pos, (ELanguage)lang);
                beg += len;
            }
            Y_ASSERT(beg == end);
            continue;
        }

        if (i->Type & TFullDocAttrs::AttrSearchDate) {
            char *ptr;
            ui32 tm = strtoul(i->Value.data(), &ptr, 10);
            if (*ptr == 0)
                inserter.StoreDateTimeAttr(i->Name.data(), tm);
        }

        char preparedValue[MAXKEY_BUF];
        if (i->Type & TFullDocAttrs::AttrSearchLiteral) {
            if (PrepareLiteral(i->Value.data(), preparedValue))
                inserter.StoreLiteralAttr(i->Name.data(), preparedValue, 0);
        }
        if (i->Type & TFullDocAttrs::AttrSearchUrl) {
            if (PrepareURL(i->Value.data(), preparedValue))
                inserter.StoreLiteralAttr(i->Name.data(), preparedValue, 0);
        }
        if (i->Type & TFullDocAttrs::AttrSearchInteger) {
            if (PrepareInteger(i->Value.data(), preparedValue))
                inserter.StoreIntegerAttr(i->Name.data(), preparedValue, 0);
        }
    }
}

