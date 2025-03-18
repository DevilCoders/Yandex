#include "config_loader.h"

#include <tools/remorphc/pb_config.pb.h>

#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/folder/filelist.h>
#include <util/folder/pathsplit.h>
#include <util/system/defaults.h>

#include <google/protobuf/messagext.h>

namespace NRemorphCompiler {

namespace {

static const int RECURSION_DEPTH = 64;
static const char* ARCADIA_TESTS_DATA = "arcadia_tests_data";
static const char* ARCADIA_TESTS_DATA_VAR = "atd";

inline TString CheckPath(const TString& path, const TFsPath& basePath, bool must_exist = true) {
    if (path.empty()) {
        throw yexception() << "Empty path specified";
    }

    TFsPath p(path);

    if (p.IsRelative()) {
        p = basePath / p;
    }

    if (!p.Exists()) {
        if (must_exist) {
            throw yexception() << "Path not found: " << p.c_str();
        }

        TFsPath dir = p.Dirname();
        if (!dir.Exists()) {
            throw yexception() << "Dir not found: " << dir.c_str();
        }
    }

    return p.RealLocation().c_str();
}

inline bool DetectArcadiaTestsData(const TFsPath& path, TFsPath& arcadiaTestsDataPath) {
    arcadiaTestsDataPath = "/";
    Y_ASSERT(path.IsAbsolute());
    const TPathSplit& pathSplit = path.PathSplit();
    for (TPathSplit::const_iterator elem = pathSplit.begin(); elem != pathSplit.end(); ++elem) {
        arcadiaTestsDataPath /= *elem;
        if (*elem == ARCADIA_TESTS_DATA) {
            return true;
        }
    }

    return false;
}

class TPbErrorCollector: public NProtoBuf::io::ErrorCollector {
private:
    const TString& Path;
    IOutputStream* Log;

public:
    TPbErrorCollector(const TString& path, IOutputStream* log)
        : NProtoBuf::io::ErrorCollector()
        , Path(path)
        , Log(log)
    {
    }

    void AddError(int line, int column, const TProtoStringType& message) override {
        throw TConfigLoadingError(Path, NReMorph::TSourcePos(line, column)) << message;
    }

    void AddWarning(int line, int column, const TProtoStringType& message) override {
        if (Log) {
            *Log << "Warning: " << Path << ":" << NReMorph::TSourcePos(line, column) << ": " << message << Endl;
        }
    }
};

}

const char* GetDefaultConfigName() {
    return ".remorph";
}

TConfigLoader::TConfigLoader(TConfig& config, const TVars& vars, IOutputStream* log)
    : PbParser()
    , Config(config)
    , VarReplacer(vars)
    , GazetteerPool(new TGazetteerPool(log))
    , Log(log)
{
}

void TConfigLoader::Load(const TString& path, bool recursive) {
    TFsPath p = TFsPath(path).RealPath();

    if (!recursive) {
        Load(p);
        return;
    }

    if (!p.IsDirectory()) {
        throw yexception() << "Path is not a directory: " << path;
    }

    TFileList configPathList;
    configPathList.Fill(p.c_str(), "", GetDefaultConfigName(), RECURSION_DEPTH, true);

    if (configPathList.Size() != 0) {
        const char* filePath = nullptr;
        while (filePath = configPathList.Next()) {
            TFsPath configPath(filePath);
            if (configPath.GetName() == GetDefaultConfigName()) {
                Load(p / configPath);
            }
        }
    }
}

void TConfigLoader::Load(const TFsPath& configPath) {
    try {
        if (Log) {
            *Log << "Config: " << configPath.c_str() << Endl;
        }

        TFsPath basePath = configPath.Dirname();

        TVars configVars;
        TFsPath arcadiaTestsDataPath;
        if (DetectArcadiaTestsData(basePath, arcadiaTestsDataPath)) {
            configVars.insert(::std::make_pair(ARCADIA_TESTS_DATA_VAR, arcadiaTestsDataPath.c_str()));
        }

        NRemorph::TPbConfig pbConfig;

        TPbErrorCollector pbErrorCollector(configPath.c_str(), Log);
        PbParser.RecordErrorsTo(&pbErrorCollector);

        TUnbufferedFileInput stream(configPath.c_str());
        google::protobuf::io::TCopyingInputStreamAdaptor adaptor(&stream);

        // На ошибки бросаются исключения внутри Parse.
        bool result = PbParser.Parse(&adaptor, &pbConfig);
        Y_ASSERT(result);

        for (size_t b = 0; b < pbConfig.BuildSize(); ++b) {
            const NRemorph::TPbConfig_TBuild& pbBuild = pbConfig.GetBuild(b);

            TString buildInput = pbBuild.GetInput();
            buildInput = CheckPath(ReplaceVars(buildInput, configVars), basePath);

            TString buildOutput = pbBuild.GetOutput();
            buildOutput = CheckPath(ReplaceVars(buildOutput, configVars), basePath, false);

            NMatcher::EMatcherType buildType = NMatcher::MT_REMORPH;
            switch (pbBuild.GetType()) {
            case NRemorph::TPbConfig_TBuild_EType_REMORPH:
                buildType = NMatcher::MT_REMORPH;
                break;
            case NRemorph::TPbConfig_TBuild_EType_TOKENLOGIC:
                buildType = NMatcher::MT_TOKENLOGIC;
                break;
            case NRemorph::TPbConfig_TBuild_EType_CHAR:
                buildType = NMatcher::MT_CHAR;
                break;
            }

            TUnitConfig::TPtr unit(new TUnitConfig(buildInput, buildOutput, buildType));

            Config.GetUnits().push_back(unit);

            if (pbBuild.HasGzt()) {
                TString buildGzt = pbBuild.GetGzt();
                unit->SetGazetteerPool(GazetteerPool);
                unit->SetGazetteer(CheckPath(ReplaceVars(buildGzt, configVars), basePath));
            }

            if (pbBuild.HasGztBase()) {
                TString buildGztBase = pbBuild.GetGztBase();
                unit->SetGazetteerBase(CheckPath(ReplaceVars(buildGztBase, configVars), basePath));
            }

            for (size_t d = 0; d < pbBuild.RequireSize(); ++d) {
                TString buildRequire = pbBuild.GetRequire(d);
                unit->AddDependency(CheckPath(ReplaceVars(buildRequire, configVars), basePath, false));
            }
        }
    } catch (const TConfigLoadingError& error) {
        throw error;
    } catch (const TVarReplacingError& error) {
        if (error.Name == ARCADIA_TESTS_DATA_VAR) {
            throw TConfigLoadingError(configPath.c_str()) << error.what() << " (or just building outside of arcadia_tests_data)";
        } else {
            throw TConfigLoadingError(configPath.c_str()) << error.what();
        }
    } catch (const yexception& error) {
        throw TConfigLoadingError(configPath.c_str()) << error.what();
    }
}

TString& TConfigLoader::ReplaceVars(TString& str, const TVars& runtimeVars) const {
    VarReplacer.Parse(str, runtimeVars);
    return str;
}

} // NRemorphCompiler
