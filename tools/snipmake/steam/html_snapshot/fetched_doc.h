#pragma once

#include <library/cpp/mime/types/mime.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NSteam
{

class TFetchedDoc
{
public:
    TString JobId;
    TString Url;
    TString FinalUrl;
    TString Content;

    TString RawMimeType;
    MimeTypes MimeType;

    TString RawEncoding;
    ECharset Encoding;

    ELanguage Language;

    bool Failed;
    int HttpCode;
    TString ErrorMessage;

    TFetchedDoc()
        : MimeType(MIME_UNKNOWN)
        , Encoding(CODES_UNKNOWN)
        , Language(LANG_UNK)
        , Failed(false)
        , HttpCode(0)
    {
    }
};

}
