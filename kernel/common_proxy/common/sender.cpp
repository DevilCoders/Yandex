#include "sender.h"

namespace NCommonProxy {

    TSender::TSender(const TString& name, const TProcessorsConfigs& configs)
        : TProcessor(name, configs)
    {}

}
