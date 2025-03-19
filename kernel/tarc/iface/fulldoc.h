#pragma once

#include <util/system/maxlen.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/typetraits.h>
#include <utility>

/*
extinfo:
||TFullArchiveDocHeader||
*/

struct TFullArchiveDocHeader {
    ui32 Flags;
    i32  IndexDate;
    ui8  Language;
    ui8  Language2;
    ui8  MimeType;
    i8   Encoding;
    char Url[URL_MAX];

    TFullArchiveDocHeader()
        : Flags(0)
        , IndexDate(0)
        , Language(0)
        , Language2(0)
        , MimeType(0)
        , Encoding(0)
    {
        Url[0] = '\0';
    }
};

enum EArchiveFlags {
    AF_Frameset  = 1,
    AF_FullHtml  = 2,
    AF_Noindex   = 4,
    AF_Nofollow  = 8,
    AF_Noarchive = 16,
    AF_Refresh   = 32,
    AF_Fake      = 64
};

class TBuffer;

/*
doctext:
||header: sizeof(TFullArchiveTextHeader)|blocksdir: BlockCount*sizeof(TFullArchiveTextBlockInfo)|blocks||
*/

struct TFullArchiveTextHeader {
    ui16 BlockCount; // количество блоков
    ui16 Reserved;
    TFullArchiveTextHeader()
        : BlockCount(0)
        , Reserved(0)
    {}
};

Y_DECLARE_PODTYPE(TFullArchiveTextHeader);

enum EFullArchiveBlockType {
    FABT_ORIGINAL = 0,
    FABT_HTMLCONV = 1,
    FABT_ANCHORTEXT = 2,  // Для хранения линковых текстов из Orange в БР
};

struct TFullArchiveTextBlockInfo {
    ui32 EndOffset; // смещение в байтах конца блока относительно начала массива блоков
    ui16 BlockType; // один из EFullArchiveBlockType
    ui16 Reserved;
    TFullArchiveTextBlockInfo()
        : EndOffset(0)
        , BlockType(FABT_ORIGINAL)
        , Reserved(0)
    {}
};

Y_DECLARE_PODTYPE(TFullArchiveTextBlockInfo);
