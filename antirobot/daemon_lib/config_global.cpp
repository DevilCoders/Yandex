#include "config_global.h"

#include <library/cpp/config/sax.h>

#include <util/stream/file.h>

using namespace NConfig;

namespace NAntiRobot {
    class TLuaConfParser : public IConfig::IFunc {
        START_PARSE {
            {
                TString confStr;
                ON_KEY("conf", confStr) {
                    Conf->GetDaemonConfig().LoadFromString(confStr);

                    Conf->GetDaemonConfig().JsonConfig.LoadFromFile(
                        Conf->GetDaemonConfig().JsonConfFilePath,
                        Conf->GetDaemonConfig().JsonServiceRegExpFilePath
                    );

                    Conf->GetDaemonConfig().GlobalJsonConfig.LoadFromFile(
                        Conf->GetDaemonConfig().GlobalJsonConfFilePath
                    );

                    Conf->GetDaemonConfig().ExperimentsConfig.LoadFromFile(
                        Conf->GetDaemonConfig().ExperimentsConfigFilePath
                    );

                    return;
                }

                ON_KEY("non_branded_partners", Conf->NonBrandedPartners)
                    return;
            }
        } END_PARSE

    public:
        TLuaConfParser(TAntirobotConfig* conf)
            : Conf(conf)
        {
        }

    private:
        TAntirobotConfig* Conf;
    };

    TAntirobotConfig& TAntirobotConfig::Get() {
        static TAntirobotConfig cfg;
        return cfg;
    }

    TAntirobotConfig::TAntirobotConfig()
        : DaemonConfig(new TAntirobotDaemonConfig())
    {
    }

    void TAntirobotConfig::LoadFromPath(const TString& filePath) {
        FilePath = filePath;

        TFileInput fileInput(FilePath);
        THolder<IConfig> conf = ConfigParser(fileInput);
        TLuaConfParser confParser(this);
        conf->ForEach(&confParser);
    }
}
