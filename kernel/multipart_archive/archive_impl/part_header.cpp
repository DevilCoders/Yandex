#include "part_header.h"


#include <library/cpp/logger/global/global.h>
#include <util/system/rwlock.h>


namespace NRTYArchive {

    TPartHeader::TPartHeader(const TFsPath& path, EOpenMode mode)
        : IPartHeader(path, mode == RdOnly)
        , TResizableMappedFile(path, mode) {
        DocsCount = Size() / sizeof(ui32);
        DocsLimit = DocsCount;
    }

    TPartHeader::~TPartHeader() {}

    ui32 TPartHeader::operator[](ui64 position) const {
        VERIFY_WITH_LOG(position < DocsCount, "invalid position: position=%lu; docs_count=%lu; path=%s", position, DocsCount, Path.GetPath().data());
        const ui32* data = GetPtr<ui32>();
        return data[position];
    }

    void TPartHeader::Set(ui64 position, ui32 val) {
        VERIFY_WITH_LOG(position < DocsCount, "invalid position: position=%lu; docs_count=%lu; path=%s", position, DocsCount, Path.GetPath().data());
        ui32* data = GetPtr<ui32>();
        data[position] = val;
    }

    ui32& TPartHeader::operator[](ui64 position) {
        VERIFY_WITH_LOG(position < DocsCount, "invalid position: position=%lu; docs_count=%lu; path=%s", position, DocsCount, Path.GetPath().data());
        ui32* data = GetPtr<ui32>();
        return data[position];
    }

    ui64 TPartHeader::GetDocsCount() const {
        return DocsCount;
    }

    void TPartHeader::DoClose() {
        Resize(DocsCount * sizeof(ui32));
    }

    ui32 TPartHeader::PushDocument(ui32 docid) {
        CHECK_WITH_LOG(!IsClosed());
        if (DocsLimit == 0) {
            Resize(InitialDocsCount * sizeof(ui32));
        }

        if (DocsCount + 1 == DocsLimit) {
            Resize(2 * DocsCount * sizeof(ui32));
        }

        ui32* data = GetPtr<ui32>();
        data[DocsCount] = docid;
        return DocsCount++;
    }

    void TPartHeader::DoResize(ui64 oldSize, ui64 newSize) {
        CHECK_WITH_LOG(!IsClosed());
        ui64 start = oldSize / sizeof(ui32);
        DocsLimit = newSize / sizeof(ui32);
        CHECK_WITH_LOG(start * sizeof(ui32) == oldSize);
        CHECK_WITH_LOG(DocsLimit * sizeof(ui32) == newSize);

        for (ui32 i = start; i < DocsLimit; ++i) {
            ui32* data = GetPtr<ui32>();
            data[i] = 0;
        }
    }

    TFakePartHeader::TFakePartHeader(const TFsPath& path, EOpenMode mode)
        : IPartHeader(path, mode == RdOnly)
        , Fake(static_cast<ui32>(EFlags::DOC_REMOVED)) {}

    ui32 TFakePartHeader::operator[](ui64) const {
        return Fake;
    }

    ui32& TFakePartHeader::operator[](ui64) {
        return Fake;
    }

    ui64 TFakePartHeader::GetDocsCount() const {
        return 0;
    }

    void TFakePartHeader::DoClose() {}

    ui32 TFakePartHeader::PushDocument(ui32) {
        return 0;
    }

    void TFakePartHeader::Set(ui64, ui32) {}
}
