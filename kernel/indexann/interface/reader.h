#pragma once

#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/system/type_name.h>
#include <util/generic/function.h>
#include <util/generic/maybe.h>

#include <kernel/doom/search_fetcher/search_fetcher.h>
#include <kernel/indexann/hit/hit.h>

namespace NIndexAnn {

    class IDocDataIndex;

    class IDocDataIterator {
    public:
        virtual const THit& Current() const = 0;
        virtual bool Valid() const = 0;
        virtual const THit* Next() = 0;
        virtual void Restart(const THitMask& mask = THitMask()) = 0;

        virtual void AnnounceDocIds(TConstArrayRef<ui32> docIds);
        virtual void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader);
        virtual bool HasDoc(ui32 doc);
        virtual THolder<IDocDataIterator> Clone(const IDocDataIndex* parent);

        virtual ~IDocDataIterator() = default;
    };

    class IDocDataIndex {
    protected:
        virtual THolder<IDocDataIterator> DoCreateIterator() const = 0;

    public:
        THolder<IDocDataIterator> CreateIterator(const THitMask& mask = THitMask()) const {
            auto res = DoCreateIterator();
            if (!mask.IsNull())
                res->Restart(mask);
            return res;
        }

        virtual bool HasDoc(ui32 doc, IDocDataIterator* /* iterator */) const {
            return HasDoc(doc);
        }

        virtual bool HasDoc(ui32 doc) const = 0;

        virtual ~IDocDataIndex() = default;
    };

    Y_FORCE_INLINE ui32 DataRegionToUi32(const char* data, size_t length) {
        ui32 res = 0;
        Y_ASSERT(length <= sizeof(res));

        switch (length) {
#define CASE_COPY_NUM(NUM)                                     \
        case NUM:                                              \
            MemCopy(reinterpret_cast<char*>(&res), data, NUM); \
            break;

        CASE_COPY_NUM(sizeof(res));
        CASE_COPY_NUM(sizeof(res) - 1);
        CASE_COPY_NUM(sizeof(res) - 2);
        CASE_COPY_NUM(sizeof(res) - 3);

#undef CASE_COPY_NUM
        default:
            Y_VERIFY(false);
        }
        return res;
    }
}
