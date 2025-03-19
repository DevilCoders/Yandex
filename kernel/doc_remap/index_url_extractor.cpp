#include "index_url_extractor.h"
#include "remap_reader.h"

#include <kernel/keyinv/indexfile/seqreader.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/yexception.h>

TIndexUrlExtractor::TIndexUrlExtractor(const TString& index)
{
    TSequentYandReader syr;
    const char* URL_KEY = "#url=\"";
    syr.Init(index.data(), URL_KEY);
    while (syr.Valid())
    {
        const YxKey &entry = syr.CurKey();
        for (TSequentPosIterator it(syr); it.Valid(); ++it)
        {
            ui32 docId = TWordPosition::Doc(*it);
            if (docId >= Urls.size())
                Urls.resize(docId + 1, nullptr);
            //printf("%s %" PRIu32 "\n", entry.Text, docId);
            size_t len = strlen(entry.Text) - sizeof(URL_KEY) + 3;
            Urls[docId] = new char[len];
            strcpy(Urls[docId], entry.Text + sizeof(URL_KEY) - 2);
        }
        syr.Next();
    }
}

TString TIndexUrlExtractor::GetString(ui32 docId)
{
    if ((docId >= Urls.size()) || !Urls[docId])
        ythrow yexception() << "docId not found";
    else
        return Urls[docId];
}

bool TIndexUrlExtractor::GetString(ui32 docId, TString* url)
{
    if ((docId >= Urls.size()) || !Urls[docId]) {
        return false;
    } else {
        *url = Urls[docId];
        return true;
    }
}

size_t TIndexUrlExtractor::GetSize() const {
    return Urls.size();
}

TIndexUrlExtractor::~TIndexUrlExtractor()
{
    for (size_t i = 0; i < Urls.size(); ++i)
        delete[] Urls[i];
}
