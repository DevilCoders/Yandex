#include "part_optimization.h"

using namespace NRTYArchive;

TPartOptimizationCheckVisitor::TPartOptimizationCheckVisitor(const TOptimizationOptions& optimizationOptions)
    : OptimizationOptions(optimizationOptions)
{
}

void TPartOptimizationCheckVisitor::Start() {
    OptimizablePartNums.clear();
    UndersizedPartNums.clear();
    DocsInOptimizableParts = 0;
    DocsInUndersizedParts = 0;
    Started = true;
}

void TPartOptimizationCheckVisitor::VisitPart(ui32 partNum, const IPartState& part) {
    CHECK_WITH_LOG(Started);

    const ui64 partHeaderSize = part.GetDocsCount(true);
    const ui64 partSizeLimit = part.GetPartSizeLimit();
    const ui64 partDocsCount = part.GetDocsCount(false);
    const TString partPath = part.GetPath().GetName();
    DEBUG_LOG << "Checking part " << partPath << " : (" << partDocsCount << "/" << partHeaderSize << "/" << partSizeLimit << ") docs..." << Endl;

    CHECK_WITH_LOG(partHeaderSize != 0);

    const float population = (partDocsCount + 0.0f) / partHeaderSize;
    const float minPopulationRate = OptimizationOptions.GetPopulationRate();
    if (population < minPopulationRate) {
        OptimizablePartNums.push_back(partNum);
        DocsInOptimizableParts += partDocsCount;
        DEBUG_LOG << partPath << ": Has underpopulated part (" << ToString(population) << "<" << ToString(minPopulationRate) << ")" << Endl;
        return;
    }

    if (partSizeLimit == IArchivePart::TConstructContext::NOT_LIMITED_PART || !partSizeLimit) {
        return;
    }

    const float fullSizeDeviation = (static_cast<long long int>(part.GetSizeInBytes() - partSizeLimit) + 0.0f) / partSizeLimit;
    const float maxPartSizeDeviation = OptimizationOptions.GetPartSizeDeviation();
    if (fullSizeDeviation > maxPartSizeDeviation) {
        OptimizablePartNums.push_back(partNum);
        DocsInOptimizableParts += partDocsCount;
        DEBUG_LOG << partPath << ": Has oversized part (" << ToString(fullSizeDeviation) << ">" << ToString(maxPartSizeDeviation) << ")" << Endl;
    } else if (-fullSizeDeviation > maxPartSizeDeviation) {
        UndersizedPartNums.push_back(partNum);
        DocsInUndersizedParts += partDocsCount;
        DEBUG_LOG << partPath << ": Has undersized part (" << ToString(-fullSizeDeviation) << ">" << ToString(maxPartSizeDeviation) << ")" << Endl;
    }
}

TSet<ui32> TPartOptimizationCheckVisitor::GetOptimizablePartNums() const {
    CHECK_WITH_LOG(Started);
    TSet<ui32> optimizablePartNums(OptimizablePartNums.cbegin(), OptimizablePartNums.cend());
    if (UndersizedPartOptimizationRequired()) {
        optimizablePartNums.insert(UndersizedPartNums.cbegin(), UndersizedPartNums.cend());
    }
    return optimizablePartNums;
}

TArchiveInfo::TOptimizationInfo TPartOptimizationCheckVisitor::GetOptimizationInfo() const {
    CHECK_WITH_LOG(Started);
    TArchiveInfo::TOptimizationInfo info;
    info.Opts = OptimizationOptions;
    info.PartsToOptimize = OptimizablePartNums.size();
    info.DocsToMove = DocsInOptimizableParts;

    if (UndersizedPartOptimizationRequired()) {
        info.PartsToOptimize += UndersizedPartNums.size();
        info.DocsToMove += DocsInUndersizedParts;
    }
    return info;
}

bool TPartOptimizationCheckVisitor::UndersizedPartOptimizationRequired() const {
    return (!OptimizablePartNums.empty() || UndersizedPartNums.size() > OptimizationOptions.GetMaxUndersizedPartsCount());
}
