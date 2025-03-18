#pragma once

#include <library/cpp/html/blob/document.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NHtml {
    namespace NBlob {
        //! Получить метаданные документа.
        bool GetMetadata(const TStringBuf& data, TDocumentPack* meta);

        //! Загрузить список строк из указанного блока памяти.
        bool GetStrings(const TStringBuf& data, TVector<TString>* result);

    }
}
