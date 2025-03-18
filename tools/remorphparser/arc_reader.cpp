#include "arc_reader.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/arcface.h>
#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/system/defaults.h>

namespace NRemorphParser {

namespace {

static const TUtf16String SENTENCE_DELIMITER = u"\n\n";

inline void GetTextHeaders(const TBlob& extInfo, SDocumentAttribtes* textInfo) {
    TDocDescr docDescr;
    docDescr.UseBlob(extInfo.Data(), static_cast<unsigned int>(extInfo.Size()));
    if (docDescr.IsAvailable()) {
        textInfo->m_strUrl = docDescr.get_url();
        textInfo->m_Charset = docDescr.get_encoding();
        textInfo->m_MimeType = docDescr.get_mimetype();
        {
            TDocInfos docInfo;
            docDescr.ConfigureDocInfos(docInfo);
            if (docInfo.find("lang") != docInfo.end()) {
                textInfo->m_Language = ::LanguageByName(docInfo["lang"]);
            }
        }
    }
}

inline void GetText(const TBlob& docText, TUtf16String* text) {
    TVector<TArchiveSent> outSents;
    {
        TVector<int> sentNumbers;
        ::GetSentencesByNumbers(static_cast<const ui8*>(docText.Data()), sentNumbers, &outSents, nullptr, true);
    }
    text->clear();
    TArchiveMarkupZones markupZones;
    ::GetArchiveMarkupZones(static_cast<const ui8*>(docText.Data()), &markupZones);
    TVector<ui32> sentencesOffsets(outSents.size());
    ui32 offset = 0;
    TSentReader sentReader;
    for (size_t sentenceIndex = 0; sentenceIndex < outSents.size(); ++sentenceIndex) {
        sentencesOffsets[sentenceIndex] = offset;
        text->append(sentReader.GetText(outSents[sentenceIndex], markupZones.GetSegVersion()));
        text->append(SENTENCE_DELIMITER);
        offset = text->size();
    }
}

}

TArcReader::TArcReader(const TString& fileName) {
    ArchiveIterator.Open(fileName.c_str());
}

bool TArcReader::GetNextTextData(SDocumentAttribtes* textInfo, TUtf16String* text, const ELanguage onlyOfLang) {
    while(true) {
        textInfo->Reset();
        TArchiveHeader* currentHeader = nullptr;
        try {
            currentHeader = ArchiveIterator.NextAuto();
            if (!currentHeader) {
                return false;
            }
            textInfo->m_iDocID = currentHeader->DocId;
            GetTextHeaders(ArchiveIterator.GetExtInfo(currentHeader), textInfo);
            if (onlyOfLang != LANG_UNK && textInfo->m_Language != onlyOfLang) {
                continue;
            }
            GetText(ArchiveIterator.GetDocText(currentHeader), text);
            return true;
        } catch (const yexception& error) {
            if (currentHeader) {
                throw yexception() << "Error in docid " << currentHeader->DocId << ": " << error.what() << Endl;
            }
            throw error;
        }
    }
}

} // NRemorphParser
