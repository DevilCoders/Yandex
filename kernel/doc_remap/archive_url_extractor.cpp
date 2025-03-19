#include    <util/generic/string.h>
#include    <util/folder/dirut.h>

#include    "archive_url_extractor.h"

TArchiveUrlExtractor::TArchiveUrlExtractor(const TString& input)
{
    Archive.Open(input);
    FullArchive.Open(input);
}

TString TArchiveUrlExtractor::GetString(ui32 docId)
{
    return ::GetDocumentUrl(Archive, FullArchive, docId);
}
