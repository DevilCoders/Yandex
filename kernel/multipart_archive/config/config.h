#pragma once

#include <kernel/multipart_archive/abstract/part.h>
#include <kernel/multipart_archive/abstract/optimization_options.h>

#include <library/cpp/yconf/conf.h>
#include <library/cpp/object_factory/object_factory.h>

#include <util/generic/size_literals.h>

namespace NRTYArchive {
    struct TMultipartConfig {
        void Init(const TYandexConfig::Section& section);
        void Check() const;
        void ToString(IOutputStream& so) const;
        TString ToString(const char* sectionName) const;
        IArchivePart::TConstructContext CreateReadContext(bool isFlatCompatible = false) const;
        IArchivePart::TConstructContext CreateContext(bool isFlatCompatible = false) const;
        TOptimizationOptions CreateOptimizationOptions() const;

        float PopulationRate = 0.6f;
        float PartSizeDeviation = 0.2f;
        ui64 PartSizeLimit = IArchivePart::TConstructContext::NOT_LIMITED_PART;

        ui32 MaxUndersizedPartsCount = 0;
        ui32 WritableThreadsCount = 1;

        ui32 WriteSpeedBytes = 0;

        constexpr static ui64 MAX_PART_SIZE_FOR_PREALLOCATION = 2_GB;

        bool PreallocateParts = false;

        IDataAccessor::TType ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        IDataAccessor::TType WriteContextDataAccessType = IDataAccessor::DIRECT_FILE;
        IArchivePart::TType Compression = IArchivePart::RAW;
        IArchivePart::TConstructContext::TCompressionParams CompressionParams;

        bool WithoutSizes = false;
        bool LockFAT = false;
        bool PrechargeFAT = false;

        class IChecker {
        public:
            virtual ~IChecker() {}
            virtual void Check(const TMultipartConfig& config) const = 0;
        };

        typedef NObjectFactory::TObjectFactory<IChecker, TString> TCheckerFactory;
    };
}
