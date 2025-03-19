#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/folder/path.h>
#include <util/system/file.h>

#include "part.h"

namespace NRTYArchive {

    class IPartHeader : public IClosable {
    protected:
        static const ui64 InitialDocsCount = 1000;

    public:
        static void SaveRestoredHeader(const TFsPath& path, const TVector<ui32>& headerData);

    public:
        using TPtr = TAtomicSharedPtr<IPartHeader>;

        enum struct EFlags : ui32 {
            DOC_REMOVED = ui32(-1),
            DOC_NEW = ui32(0)
        };

        IPartHeader(const TFsPath& path, bool closed)
            : IClosable(closed)
            , Path(path)
        {}

        virtual ~IPartHeader() {
        }

        TFsPath GetPath() const {
            return Path;
        }

        virtual ui32& operator[](ui64 position) = 0;
        virtual ui32 operator[](ui64 position) const = 0;
        virtual ui64 GetDocsCount() const = 0;
        virtual void Set(ui64, ui32) = 0;
        virtual ui32 PushDocument(ui32 docid) = 0;

    protected:
        const TFsPath Path;
    };
}
