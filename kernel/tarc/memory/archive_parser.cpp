#include "archive_parser.h"

TArchiveParser::TArchiveParser()
    : TMemoryArchiveBuffer(/*maxDocs =*/1)
{}

void TArchiveParser::Parse(const TString& archiveString, TBlob *extInfo, TBlob *docText) {
    ui32 tempDocId = 0;
    if (HasDoc(tempDocId))
        EraseDoc(tempDocId);

    AddDoc(tempDocId);
    DoWrite((void*)archiveString.data(), archiveString.size());
    TMemoryArchiveBuffer::GetExtInfoAndDocText(tempDocId, *extInfo, *docText,
        /*getExtInfo =*/true, /*getDocText =*/true);
}
