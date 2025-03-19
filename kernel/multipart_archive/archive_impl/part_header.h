#pragma once
#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/abstract/part_header.h>
#include <kernel/multipart_archive/common/mapping.h>


namespace NRTYArchive {

    class TPartHeader : public IPartHeader, public TResizableMappedFile {
    public:
        TPartHeader(const TFsPath& path, EOpenMode mode);

        virtual ui32 PushDocument(ui32 docid) override;

        virtual ui32& operator[](ui64 position) override;

        virtual void Set(ui64 position, ui32 val) override;

        virtual ui32 operator[](ui64 position) const override;

        virtual ui64 GetDocsCount() const override;

        virtual ~TPartHeader();

    private:
        //TResizableMappedFile
        virtual void DoResize(size_t, size_t) override;
        //IClosable
        virtual void DoClose() override;

    private:
        ui64 DocsLimit;
        ui64 DocsCount;
    };

    class TFakePartHeader : public IPartHeader {
    public:
        TFakePartHeader(const TFsPath& path, EOpenMode mode);

        virtual ui32& operator[](ui64) override;

        virtual ui32 operator[](ui64) const override;

        virtual ui64 GetDocsCount() const override;

        virtual void DoClose() override;

        virtual ui32 PushDocument(ui32) override;

        virtual void Set(ui64 position, ui32 val) override;

    private:
        ui32 Fake = 0;
    };
}
