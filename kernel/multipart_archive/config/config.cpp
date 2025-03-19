#include "config.h"

#include <util/string/builder.h>

namespace NRTYArchive {

void TMultipartConfig::Init(const TYandexConfig::Section& section) {
    const TYandexConfig::Directives& dir = section.GetDirectives();
    dir.GetValue("PopulationRate", PopulationRate);
    dir.GetValue("PartSizeDeviation", PartSizeDeviation);
    dir.GetValue("PartSizeLimit", PartSizeLimit);
    dir.GetValue("MaxUndersizedPartsCount", MaxUndersizedPartsCount);
    dir.GetValue("WritableThreadsCount", WritableThreadsCount);
    dir.GetValue("PreallocateParts", PreallocateParts);

    Y_ENSURE(!PreallocateParts || PartSizeLimit <= MAX_PART_SIZE_FOR_PREALLOCATION, TStringBuilder{} << "Part size is too large for preallocation: " << PartSizeLimit);

    bool enumPackEnabled = false;
    dir.GetValue("EnumPackEnabled", enumPackEnabled);
    Y_ENSURE(!enumPackEnabled, "EnumPackEnabled is not supported anymore");

    TString readContextDataAccessType;
    dir.GetValue("ReadContextDataAccessType", readContextDataAccessType);
    if (!readContextDataAccessType.empty())
        if (!TryFromString(readContextDataAccessType, ReadContextDataAccessType))
            ythrow yexception() << "Unknown DataAccessor type" << readContextDataAccessType;

    TString writeContextDataAccessType;
    dir.GetValue("WriteContextDataAccessType", writeContextDataAccessType);
    if (!writeContextDataAccessType.empty())
        if (!TryFromString(writeContextDataAccessType, WriteContextDataAccessType))
            ythrow yexception() << "Unknown DataAccessor type" << writeContextDataAccessType;

    TString compression;
    dir.GetValue("Compression", compression);
    if (!compression.empty())
        if(!TryFromString(compression, Compression))
            ythrow yexception() << "Unknown DataCompression type" << compression;
    TYandexConfig::TSectionsMap sm = section.GetAllChildren();
    TYandexConfig::TSectionsMap::const_iterator i = sm.find("CompressionParams");
    if (i != sm.end()) {
        const TYandexConfig::Directives& cpDir = i->second->GetDirectives();
        cpDir.GetValue("Algorithm", CompressionParams.Algorithm);
        cpDir.GetValue("Level", CompressionParams.Level);
    }
    auto it = sm.find("CompressionExtParams");
    if (it != sm.end()) {
        const TYandexConfig::Directives& cpDir = it->second->GetDirectives();
        cpDir.GetValue("CodecName", CompressionParams.ExtParams.CodecName);
        cpDir.GetValue("BlockSize", CompressionParams.ExtParams.BlockSize);
        cpDir.GetValue("LearnSize" , CompressionParams.ExtParams.LearnSize);
    }
    Check();
}

void TMultipartConfig::ToString(IOutputStream& so) const {
    so << "PopulationRate: " << PopulationRate << Endl;
    so << "PartSizeDeviation: " << PartSizeDeviation << Endl;
    so << "PartSizeLimit: " << PartSizeLimit << Endl;
    so << "ReadContextDataAccessType: " << ReadContextDataAccessType << Endl;
    so << "WriteContextDataAccessType: " << WriteContextDataAccessType << Endl;
    so << "MaxUndersizedPartsCount: " << MaxUndersizedPartsCount << Endl;
    so << "WritableThredsCount:" << WritableThreadsCount << Endl;
    so << "PreallocateParts: " << PreallocateParts << Endl;
    so << "Compression: " << Compression << Endl;
    so << "<CompressionParams>" << Endl;
    so << "Algorithm: " << CompressionParams.Algorithm << Endl;
    so << "Level: " << CompressionParams.Level << Endl;
    so << "</CompressionParams>" << Endl;
    so << "<CompressionExtParams>" << Endl;
    so << "CodecName: " << CompressionParams.ExtParams.CodecName << Endl;
    so << "BlockSize: " << CompressionParams.ExtParams.BlockSize << Endl;
    so << "LearnSize: " << CompressionParams.ExtParams.LearnSize << Endl;
    so << "</CompressionExtParams>" << Endl;
}

TString TMultipartConfig::ToString(const char* sectionName) const {
    TStringStream so;
    so << "<" << sectionName << ">" << Endl;
    ToString(so);
    so << "</" << sectionName << ">" << Endl;
    return so.Str();
}

IArchivePart::TConstructContext TMultipartConfig::CreateReadContext(bool isFlatCompatible) const {
    return IArchivePart::TConstructContext(Compression,
            ReadContextDataAccessType,
            PartSizeLimit,
            PreallocateParts,
            isFlatCompatible,
            CompressionParams,
            WriteContextDataAccessType);
}

NRTYArchive::IArchivePart::TConstructContext TMultipartConfig::CreateContext(bool isFlatCompatible /*= false*/) const {
    return CreateReadContext(isFlatCompatible);
}

NRTYArchive::TOptimizationOptions TMultipartConfig::CreateOptimizationOptions() const {
    TOptimizationOptions oo;
    oo.SetPartSizeDeviation(PartSizeDeviation);
    oo.SetPopulationRate(PopulationRate);
    return oo;
}

void TMultipartConfig::Check() const {
    TSet<TString> checkrs;
    Singleton<TCheckerFactory>()->GetKeys(checkrs);
    for (const auto& checkerName : checkrs) {
        THolder<IChecker> checker(TCheckerFactory::Construct(checkerName));
        checker->Check(*this);
    }
}

}
