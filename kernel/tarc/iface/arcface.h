#pragma once

#include <kernel/tarc/enums/searcharc.h>

#include <util/system/defaults.h>
#include <util/generic/yexception.h>

class IInputStream;
class IOutputStream;

const char* const ARCLABEL = "YARC";
const size_t ARCHIVE_FILE_HEADER_SIZE = 8;

const ui64 FAIL_DOC_OFFSET = ui64(-1);

void WriteTextArchiveHeader(IOutputStream& out, TArchiveVersion archiveVersion = ARCVERSION);

struct TBadArcFormatException : yexception {
    TBadArcFormatException()
    {
        *this << "bad arc format";
    }
};
void CheckTextArchiveHeader(IInputStream& in, TArchiveVersion& archiveVersion);

ui64 WriteEmptyDoc(IOutputStream& out, const void* blob, size_t blobSize,  const void* packedtext, size_t textSize, ui32 docId);

class TBlob;
TBlob UnpackRawExtInfo(const TBlob& rawExtInfo, TArchiveVersion archiveVersion);
TBlob PackRawExtInfo(const TBlob& extInfo, TArchiveVersion archiveVersion);

/*
packed doc: DocLen
||header: sizeof(TArchiveHeader)|extinfo: ExtLen|doctext: GetTextLen()||
*/

struct Y_PACKED TArchiveHeader {
    ui32 DocLen;     // общая длина в байтах запакованного документа, включая длину этого заголовка
    ui32 DocId;      // архивный идентификатор документа
    ui32 Flags;      // какие-то флаги на всякий случай
    ui32 ExtLen;     // длина в байтах блока 'extinfo' с внешней информацией
    TArchiveHeader()
        : DocLen(0)
        , DocId(ui32(-1))
        , Flags(0)
        , ExtLen(0)
    {}
    size_t SizeOf() const {
        return DocLen;
    }
    // длина в байтах блока 'doctext' с внешней информацией
    ui32 GetTextLen() const {
        return DocLen ? DocLen - sizeof(TArchiveHeader) - ExtLen : 0;
    }

    static const ui32 RecordSig = 0x13345667;
};

Y_DECLARE_PODTYPE(TArchiveHeader);
