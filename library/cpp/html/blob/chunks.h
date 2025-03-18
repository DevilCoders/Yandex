#pragma once

#include "document.h"

#include <library/cpp/html/face/onchunk.h>

namespace NHtml {
    namespace NBlob {
        //! Генерирует события html-парсера, соответствующие данному документу.
        bool NumerateHtmlChunks(const TDocumentRef& doc, IParserResult* result);

    }
}
