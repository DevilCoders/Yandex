#pragma once

#include <FactExtract/Parser/common/sdocattributes.h>

#include <kernel/tarc/iface/tarcio.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NRemorphParser {

class TArcReader {
private:
    TArchiveIterator ArchiveIterator;

public:
    explicit TArcReader(const TString& fileName);

    bool GetNextTextData(SDocumentAttribtes* textInfo, TUtf16String* text, const ELanguage onlyOfLang = LANG_UNK);
};

} // NRemorphParser
