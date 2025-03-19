#include "http.h"

namespace NCommonProxy {

    TProcessor::TFactory::TRegistrator<THttpSource> THttpSource::Registrar("HTTP");
    TProcessorConfig::TFactory::TRegistrator<THttpSource::TConfig> THttpSource::TConfig::Registrator("HTTP");

}
