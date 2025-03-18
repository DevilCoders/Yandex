#include "unit_config.h"

#include "gazetteer_pool.h"

#include <util/folder/path.h>
#include <util/generic/yexception.h>

namespace NRemorphCompiler {

TUnitConfig::TUnitConfig(const TString& path, NMatcher::EMatcherType type)
    : Output()
    , Type(type)
    , LoadOptions(path)
    , BaseDirExplicit(false)
    , Dependencies()
    , GazetteerPool()
{

}

TUnitConfig::TUnitConfig(const TString& path, const TString& outputPath, NMatcher::EMatcherType type)
    : TUnitConfig(path, type)
{
    SetOutput(outputPath);
}

void TUnitConfig::SetOutput(const TString& outputPath) {
    Output = outputPath;
}

void TUnitConfig::SetGazetteer(const TString& gazetteerPath) {
    if (!GazetteerPool) {
        GazetteerPool = TGazetteerPoolPtr(new TGazetteerPool());
    }

    TFsPath gazetteerRealPath = TFsPath(gazetteerPath).RealPath();
    LoadOptions.Gazetteer = GazetteerPool->Add(gazetteerRealPath.c_str());

    // Встроенный в TFileLoadOptions механизм установки базовой директории по-умолчанию использует директорию
    // файла с правилами. Компилятор также использует директорию с газеттиром, причем с наибольшим приоритетом.
    if (!BaseDirExplicit) {
        LoadOptions.SetBaseDirPath(gazetteerRealPath.Parent().c_str());
    }
}

void TUnitConfig::SetGazetteerBase(const TString& gazetteerBasePath) {
    LoadOptions.SetBaseDirPath(gazetteerBasePath);
    BaseDirExplicit = true;
}

void TUnitConfig::SetGazetteerPool(TGazetteerPoolPtr gazetteerPool) {
    GazetteerPool = gazetteerPool;
}

void TUnitConfig::AddDependency(const TString& dependencyPath) {
    Dependencies.push_back(dependencyPath);
}

} // NRemorphCompiler
