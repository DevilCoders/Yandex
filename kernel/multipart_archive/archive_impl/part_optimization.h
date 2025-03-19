#pragma once

#include <kernel/multipart_archive/abstract/optimization_options.h>
#include <kernel/multipart_archive/abstract/part.h>

#include <kernel/multipart_archive/statistic/archive_info.h>

#include <library/cpp/logger/global/global.h>

#include <util/generic/string.h>
#include <util/generic/ymath.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>


namespace NRTYArchive {

    class IPartState {
    public:
        virtual ~IPartState() = default;

        virtual ui64 GetDocsCount(bool withRemoved) const = 0;
        virtual ui64 GetSizeInBytes() const = 0;
        virtual ui64 GetPartSizeLimit() const = 0;
        virtual TFsPath GetPath() const = 0;
    };

    class TPartOptimizationCheckVisitor: public TNonCopyable {
    public:
        TPartOptimizationCheckVisitor(const NRTYArchive::TOptimizationOptions& optimizationOptions);
        void Start();
        void VisitPart(ui32 partNum, const IPartState& part);
        TSet<ui32> GetOptimizablePartNums() const;
        NRTYArchive::TArchiveInfo::TOptimizationInfo GetOptimizationInfo() const;

    private:
        bool UndersizedPartOptimizationRequired() const;

        TVector<ui32> OptimizablePartNums;
        TVector<ui32> UndersizedPartNums;

        const NRTYArchive::TOptimizationOptions& OptimizationOptions;
        ui64 DocsInOptimizableParts = 0;
        ui64 DocsInUndersizedParts = 0;
        bool Started = false;
    };

} //namespace NRTYArchive
