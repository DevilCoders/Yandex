#include "part.h"
#include <library/cpp/logger/global/global.h>

namespace NRTYArchive {

    const IArchivePart::TSize IArchivePart::TConstructContext::NOT_LIMITED_PART = (ui64) 1 << 40;


    IArchivePart::TConstructContext::TConstructContext(TType type, IDataAccessor::TType daType,
                                                       TSize sizeLimit /*= NOT_LIMITED_PART*/,
                                                       bool preallocationFlag /*= false*/,
                                                       bool withoutSizes /*= false*/,
                                                       const TCompressionParams& compression /*= Default<TCompressionParams>()*/,
                                                       IDataAccessor::TType writeDAType /*= IDataAccessor::DIRECT_FILE*/,
                                                       ui32 writeSpeed /*= 0*/)
        : Type(type)
        , SizeLimit(sizeLimit)
        , PreallocationFlag(preallocationFlag)
        , WithoutSizes(withoutSizes)
        , MapHeader(false)
        , WriteSpeedBytes(writeSpeed)
        , DataAccessorContext(daType)
        , Compression(compression)
        , WriteDAType(writeDAType)
    {
        CHECK_WITH_LOG(!WithoutSizes) << "Not supported now" << Endl;
    }

    IArchivePart::TConstructContext& IArchivePart::TConstructContext::SetAccessorType(IDataAccessor::TType daType) {
        DataAccessorContext.Type = daType;
        return *this;
    }

    IArchivePart::TConstructContext& IArchivePart::TConstructContext::SetWithoutSizesFlag(bool flag) {
        WithoutSizes = flag;;
        return *this;
    }

    IArchivePart::TConstructContext& IArchivePart::TConstructContext::SetSizeLimit(TSize sizeLimit) {
        SizeLimit = sizeLimit;
        return *this;
    }

    IArchivePart::TConstructContext& IArchivePart::TConstructContext::SetMapHeader(bool flag) {
        MapHeader = flag;
        return *this;
    }

    TString IArchivePart::TConstructContext::ToString() const {
        TString result = ::ToString(Type) + ";" + ::ToString(SizeLimit) + ";" + ::ToString(WithoutSizes);
        if (Type == COMPRESSED)
            result += Compression.ToString();
        return result;
    }

    TString IArchivePart::TConstructContext::TCompressionParams::ToString() const {
        if (ExtParams.CodecName.empty()) {
            return ::ToString(Algorithm) + ";" + ::ToString(Level);
        }
        return ::ToString(Algorithm) + ";" + ::ToString(Level) + ";ext_params{" + ExtParams.ToString() + "}";
    }

    TString IArchivePart::TConstructContext::TCompressionExtParams::ToString() const {
        return TString::Join("COMP_EXT;", CodecName, ';', ::ToString(BlockSize),
                ';', ::ToString(LearnSize));
    }
}
