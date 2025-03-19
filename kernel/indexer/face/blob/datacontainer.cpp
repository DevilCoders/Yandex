#include "datacontainer.h"

#include <library/cpp/packedtypes/longs.h>

#include <library/cpp/charset/wide.h>
#include <kernel/search_types/search_types.h>

#include <util/generic/vector.h>

namespace NIndexerCore {
namespace {
    enum EIndexDataType {
        IDT_NAME = 0,
        IDT_LITERAL_ATTR = 1,
        IDT_INTEGER_ATTR,
        IDT_DATETIME_ATTR,
        IDT_URL_ATTR,
        IDT_ZONE, // search and archive zone
        IDT_KEY,
        IDT_EXTERNAL_LEMMA,
        IDT_ARCHIVE_ZONE, // archive only zone
        IDT_ARCHIVE_ZONE_ATTR,
        IDT_ARCHIVE_DOC_ATTR
    };



    static ui32 MAGIC = 0xF187A10C;

    union TDataSize {
        struct {
            ui32 Length : 24;
            ui32 Type   : 8;
        };
        i32 Value;
    };

    static_assert(sizeof(TDataSize) == sizeof(i32), "expect sizeof(TDataSize) == sizeof(i32)");

    typedef std::pair<TDataSize, const char*> TDataPair;

    /**
     * @note advance input pointer by size of readed data.
     */
    static inline TDataPair ReadDataValue(const char** p) {
        TDataPair result;
        TDataSize val;

        *p += in_long(val.Value, *p);
        result = std::make_pair(val, *p);
        *p += val.Length;

        return result;
    }

    static inline size_t CalculateDataSize(const TDataSize data) {
        return len_long((i32)data.Value) + data.Length;
    }
}

const char* TDataContainer::LEMMA_KEY = "LEMMA";

struct TDataContainer::TDataItem {
    TDataItem* Next;
    TDataSize Size;
    char Data[];
};

TDataContainer::TDataContainer(size_t initialPoolSize)
    : Pool_(initialPoolSize)
    , Length_(sizeof(MAGIC))
{ }

void* TDataContainer::AllocateRecord(TDataItem* tag, ui8 type, size_t len) {
    TDataItem* rec = (TDataItem*)Pool_.Allocate(sizeof(TDataItem) + len);

    rec->Size.Length = len;
    rec->Size.Type   = type;

    // If list is empty then point to itself else point to first element in the list.
    if (tag->Next) {
        rec->Next = tag->Next->Next;
        tag->Next->Next = rec;
    } else {
        rec->Next = rec;
    }

    tag->Next = rec;

    Length_ += len_long((i32)rec->Size.Value) + len;

    return rec->Data;
}

TDataContainer::TDataItem* TDataContainer::InsertName(const char* name) {
    TNameSet::const_iterator ni = Names_.find(name);

    if (ni != Names_.end()) {
        return ni->second;
    }

    const size_t len = strlen(name) + 1;
    TDataItem* rec = (TDataItem*)Pool_.Allocate(sizeof(TDataItem) + len);

    rec->Next = nullptr;
    rec->Size.Length = len;
    rec->Size.Type   = IDT_NAME;
    memcpy(rec->Data, name, len);

    Length_ += (len_long((i32)rec->Size.Value) + len) + 1;

    Names_.insert(std::make_pair(static_cast<const char*>(rec->Data), rec));

    return rec;
}

void TDataContainer::InsertTextValueItem(TDataItem* tag, ui8 type, const wchar16* text, size_t len, ui32 pos) {
    const ui16 textSize = len * sizeof(wchar16);
    const size_t allocLen = sizeof(pos) + (textSize + sizeof(wchar16));

    char* data = (char*)AllocateRecord(tag, type, allocLen);

    memcpy(data, &pos, sizeof(pos));
    data += sizeof(pos);
    memcpy(data, text, textSize);
    data += textSize;
    *(wchar16*)data = 0; // null terminator
}

void TDataContainer::InsertUI32ValueItem(TDataItem* tag, ui8 type, ui32 pos) {
    char* data = (char*)AllocateRecord(tag, type, sizeof(pos));

    memcpy(data, &pos, sizeof(pos));
}

void TDataContainer::StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) {
    TDataItem* tag = InsertName(attrName);
    wchar16 buffer[MAXKEY_BUF]; // TODO use buffer allocated in the pool
    const size_t n = Min<size_t>(strlen(attrText), MAXKEY_LEN);

    CharToWide(attrText, n, buffer, csYandex);
    InsertTextValueItem(tag, IDT_LITERAL_ATTR, buffer, n, pos);
}

void TDataContainer::StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) {
    InsertTextValueItem(InsertName(attrName), IDT_LITERAL_ATTR, attrText, len, pos);
}

void TDataContainer::StoreDateTimeAttr(const char* attrName, time_t datetime) {
    // TODO: add a new method StoreDateTime to TInvCreator which will have no conversion from wchar16 to char
    //       and then call to strtol()
    InsertUI32ValueItem(InsertName(attrName), IDT_DATETIME_ATTR, (ui32)datetime);
}

void TDataContainer::StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) {
    char* data = (char*)AllocateRecord(
        InsertName(attrName), IDT_INTEGER_ATTR, sizeof(pos) + strlen(attrText) + 1
    );

    memcpy(data, &pos, sizeof(pos));
    data += sizeof(pos);
    strcpy(data, attrText);
}

void TDataContainer::StoreKey(const char* key, TPosting pos) {
    InsertUI32ValueItem(InsertName(key), IDT_KEY, pos);
}

void TDataContainer::StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) {
    char* data = (char*)AllocateRecord(
        InsertName(zoneName), archiveOnly ? IDT_ARCHIVE_ZONE : IDT_ZONE, sizeof(begin) + sizeof(end)
    );

    memcpy(data, &begin, sizeof(begin));
    data += sizeof(begin);
    memcpy(data, &end, sizeof(end));
}

void TDataContainer::StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) {
    InsertTextValueItem(InsertName(name), IDT_ARCHIVE_ZONE_ATTR, value, length, pos);
}

void TDataContainer::StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) {
    TDataItem* tag = InsertName(LEMMA_KEY);
    const size_t lemmaSize = lemmaLen * sizeof(wchar16);
    const size_t formSize = formLen * sizeof(wchar16);
    const size_t allocSize = sizeof(pos) + lemmaSize + formSize + sizeof(ui16) * 2 + 2;

    char* data = (char*)AllocateRecord(tag, IDT_EXTERNAL_LEMMA, allocSize);

    memcpy(data, &pos, sizeof(pos));
    data += sizeof(pos);
    ui16 len = lemmaLen;
    memcpy(data, &len, sizeof(len));
    data += sizeof(len);
    memcpy(data, lemma, lemmaSize);
    data += lemmaSize;
    len = formLen;
    memcpy(data, &len, sizeof(len));
    data += sizeof(len);
    memcpy(data, form, formSize);
    data += formSize;
    *data++ = flags;
    *data++ = (ui8)lang;
}

void TDataContainer::StoreTextArchiveDocAttr(const TString& name, const TString& value) {
    char* data = (char*)AllocateRecord(
        InsertName(name.c_str()), IDT_ARCHIVE_DOC_ATTR, value.size() + 1
    );

    strcpy(data, value.c_str());
}

void TDataContainer::StoreFullArchiveDocAttr(const TString&, const TString&) {
    // ythrow yexception() << "StoreFullArchiveDocAttr() not implemented";
}

void TDataContainer::StoreErfDocAttr(const TString&, const TString&) {
    ythrow yexception() << "StoreErfDocAttr() not implemented";
}

void TDataContainer::StoreGrpDocAttr(const TString&, const TString&, bool) {
    ythrow yexception() << "StoreGrpDocAttr() not implemented";
}

void TDataContainer::StoreUrlAttr(const char* name, const char* text, TPosting pos) {
    TDataItem* tag = InsertName(name);
    TTempArray<wchar16> buffer; // TODO URL should be stored as is - without conversion
    const size_t n = Min<size_t>(strlen(text), buffer.Size());
    Copy(text, n, buffer.Data());

    InsertTextValueItem(tag, IDT_URL_ATTR, buffer.Data(), n, pos);
}

TBuffer TDataContainer::SerializeToBuffer() const {
    TBuffer tmp(Length_);
    SaveToImpl(&tmp);
    return tmp;
}

void TDataContainer::SaveToImpl(TBuffer* buf) const {
    char* p = buf->Data();

    memcpy(p, &MAGIC, sizeof(MAGIC));
    p += sizeof(MAGIC);

    for (TNameSet::const_iterator ni = Names_.begin(); ni != Names_.end(); ++ni) {
        const TDataItem* rec = ni->second;

        #define COPY_DATA_VALUE(p, val) \
            p += out_long((i32)val->Size.Value, p); \
            memcpy(p, val->Data, val->Size.Length); p += val->Size.Length;

        COPY_DATA_VALUE(p, rec);

        const TDataItem* val = rec->Next->Next;

        do {
            COPY_DATA_VALUE(p, val);

            val = val->Next;
        } while (val != rec->Next->Next);

        *p++ = 0;

        #undef COPY_DATA_VALUE
    }

    buf->Advance(Length_);
}


static const char* DispatchName(const char* name, const char* p, IIndexDataInserter* indexDataInserter, IArchiveDataInserter* archiveDataInserter) {
    while (*p != 0) {
        TDataSize val;
        const char* data;

        p += in_long(val.Value, p);
        data = p;
        p += val.Length;

        switch (val.Type) {
        case IDT_ARCHIVE_ZONE_ATTR:
            if (archiveDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                const wchar16* text = (const wchar16*)(data + sizeof(TPosting));
                archiveDataInserter->StoreArchiveZoneAttr(name, text, *pos);
            }
            break;
        case IDT_INTEGER_ATTR:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                const char* text = (const char*)(data + sizeof(TPosting));
                indexDataInserter->StoreIntegerAttr(name, text, *pos);
            }
            break;
        case IDT_LITERAL_ATTR:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                const wchar16* text = (const wchar16*)(data + sizeof(ui32));
                indexDataInserter->StoreLiteralAttr(name, text, *pos);
            }
            break;
        case IDT_URL_ATTR:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                const wchar16* text = (const wchar16*)(data + sizeof(ui32));
                indexDataInserter->StoreUrlAttr(name, text, *pos);
            }
            break;
        case IDT_KEY:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                indexDataInserter->StoreKey(name, *pos);
            }
            break;
        case IDT_DATETIME_ATTR:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                indexDataInserter->StoreDateTimeAttr(name, *pos);
            }
            break;
        case IDT_ZONE:
        case IDT_ARCHIVE_ZONE: {
            const TPosting* beg = (const TPosting*)data;
            const TPosting* end = (const TPosting*)(data + sizeof(TPosting));
            if (archiveDataInserter)
                archiveDataInserter->StoreArchiveZone(name, *beg, *end);
            if (val.Type == IDT_ZONE && indexDataInserter)
                indexDataInserter->StoreZone(name, *beg, *end);
            break;
        }
        case IDT_EXTERNAL_LEMMA:
            if (indexDataInserter) {
                const TPosting* pos = (const TPosting*)data;
                data += sizeof(TPosting);
                const ui16* lemmaLen = (const ui16*)data;
                data += sizeof(ui16);
                const wchar16* lemma = (const wchar16*)data;
                data += *lemmaLen * sizeof(wchar16);
                const ui16* formLen = (const ui16*)data;
                data += sizeof(ui16);
                const wchar16* form = (const wchar16*)data;
                data += *formLen * sizeof(wchar16);
                const ui8* flags = (const ui8*)data++;
                const ui8* lang = (const ui8*)data;
                indexDataInserter->StoreExternalLemma(lemma, *lemmaLen, form, *formLen, *flags, *lang, *pos);
            }
            break;
        case IDT_ARCHIVE_DOC_ATTR:
            if (archiveDataInserter)
                archiveDataInserter->StoreArchiveDocAttr(name, data);
            break;
        default:
            Y_ASSERT(false);
            break;
        }
    }

    return p;
}

void InsertDataTo(const TStringBuf& data, IIndexDataInserter* indexDataInserter, IArchiveDataInserter* archiveDataInserter) {
    if (data.empty()) {
        return;
    }

    if (!IsDataContainer(data)) {
        ythrow yexception() << "invalid data format";
    }

    for (const char* p = data.data() + sizeof(MAGIC); p < data.data() + data.size(); p += 1) {
        TDataSize val;
        const char* name;

        p += in_long(val.Value, p);
        name = p;
        p += val.Length;
        p  = DispatchName(name, p, indexDataInserter, archiveDataInserter);
    }
}

bool IsDataContainer(const TStringBuf& data) {
    return data.size() >= sizeof(MAGIC) && memcmp(data.data(), &MAGIC, sizeof(MAGIC)) == 0;
}

void MergeDataContainer(const TVector<TStringBuf>& data, TBuffer* result) {
    typedef THashMap<const char*, TVector<TDataPair> > THash;

    THash names;
    size_t length = sizeof(MAGIC); // Required length of output buffer

    for (const TStringBuf& item : data) {
        if (!IsDataContainer(item)) {
            ythrow yexception() << "invalid data format";
        }

        for (const char* p = item.data() + sizeof(MAGIC); p < item.data() + item.size(); p += 1) {
            TDataPair name = ReadDataValue(&p);

            THash::iterator ni = names.find(name.second);
            if (ni == names.end()) {
                ni = names.insert(std::make_pair(name.second, TVector<TDataPair>())).first;

                length += CalculateDataSize(name.first) + 1;
            }

            while (*p != 0) {
                TDataPair val = ReadDataValue(&p);
                ni->second.push_back(val);

                length += CalculateDataSize(val.first);
            }
        }
    }

    //
    // Save data to the result buffer
    //

    {
        TBuffer(length).Swap(*result);

        char* p = result->Data();

        memcpy(p, &MAGIC, sizeof(MAGIC));
        p += sizeof(MAGIC);

        for (THash::const_iterator ni = names.begin(); ni != names.end(); ++ni) {
            TDataPair name;
            name.first.Length = strlen(ni->first) + 1;
            name.first.Type   = 0;
            name.second       = ni->first;

            #define COPY_DATA_VALUE(p, val) \
                p += out_long((i32)val.first.Value, p); \
                memcpy(p, val.second, val.first.Length); p += val.first.Length;

            COPY_DATA_VALUE(p, name);

            for (const TDataPair& val : ni->second) {
                COPY_DATA_VALUE(p, val);
            }

            *p++ = 0;

            #undef COPY_DATA_VALUE
        }

        result->Advance(length);
    }
}

} // namespace NIndexerCore

