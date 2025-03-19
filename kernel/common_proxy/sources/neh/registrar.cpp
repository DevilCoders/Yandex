#include "neh.h"

namespace NCommonProxy {

    TProcessor::TFactory::TRegistrator<TNehSource> TNehSource::Registrar("NEH");
    TProcessorConfig::TFactory::TRegistrator<TNehSource::TConfig> TNehSource::TConfig::Registrator("NEH");

}
