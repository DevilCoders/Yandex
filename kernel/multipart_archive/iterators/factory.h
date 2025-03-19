#pragma once

#include "part_iterator.h"

#include <kernel/multipart_archive/archive_impl/multipart_base.h>


namespace NRTYArchive {

    class TOffsetsIteratorFactory : public TIteratorFactory {
    public:
        TOffsetsIteratorFactory(IFat* fat);

        virtual typename IPartIterator::TPtr CreatePartIterator(TArchivePartThreadSafe::TPtr /*part*/, IFat* /*fat*/) const override;

    private:
        TMap<ui32, TOffsetsPtr> OffsetsByPart;
    };

    class TOptimizeIteratorFactory: public TIteratorFactory {
    public:
        virtual typename IPartIterator::TPtr CreatePartIterator(TArchivePartThreadSafe::TPtr /*part*/, IFat* /*fat*/) const override;
    };

    class TRawIteratorFactory: public TIteratorFactory {
    public:
        virtual typename IPartIterator::TPtr CreatePartIterator(TArchivePartThreadSafe::TPtr /*part*/, IFat* /*fat*/) const override;
    };
}
