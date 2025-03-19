#pragma once

#include <kernel/multipart_archive/archive_impl/multipart_base.h>
#include <kernel/multipart_archive/iterators/factory.h>

namespace NRTYArchive {
    class TMultipartStorage : public TMultipartImpl<ui32> {
        using TBase = TMultipartImpl<ui32>;
    public:
        using TDocInfo = TBase::TDocInfo;
        using TIteratorPtr = typename TBase::TIterator::TPtr;

        static const ui32 DOC_REMOVED;
        static const ui32 DOC_NEW;

        typename TIterator::TPtr CreateIterator() const {
            return CreateCommonIterator(new TRawIteratorFactory());
        }

        TMultipartStorage(const TFsPath& path, const IArchivePart::TConstructContext& partsCtx);

        TPosition AppendDocument(const TBlob& document);
        TDocInfo AppendDocumentWithAddress(const TBlob& document);



        using TBase::RemoveDocument;
        bool RemoveDocument(ui32 part, ui64 index);
        bool RemoveDocument(const TDocInfo& address);
    };
}
