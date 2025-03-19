#include "factory.h"


namespace NRTYArchive {


    TOffsetsIteratorFactory::TOffsetsIteratorFactory(IFat* fat) {
        OffsetsByPart = TOffsetsIterator::CreateOffsets(fat);
    }

    IPartIterator::TPtr TOffsetsIteratorFactory::CreatePartIterator(TArchivePartThreadSafe::TPtr part, IFat* fat) const {
        auto it = OffsetsByPart.find(part->GetPartNum());
        if (it != OffsetsByPart.end()) {
            return new TOffsetsIterator(part, fat, it->second);
        } else {
            return new TOffsetsIterator(part, fat, MakeSimpleShared<TOffsets>());
        }
    }

    IPartIterator::TPtr TRawIteratorFactory::CreatePartIterator(TArchivePartThreadSafe::TPtr part, IFat* /*fat*/ ) const {
        return new TRawIterator(part);
    }
}
