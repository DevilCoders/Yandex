#pragma once

#include "factory.h"

namespace NTextMachine {
    class TWebCores : public NTextMachine::TCoreFactoryHolder {};
    class TVideoCores : public NTextMachine::TCoreFactoryHolder {};
    class TImagesCores : public NTextMachine::TCoreFactoryHolder {};
    class TGeosearchCores : public NTextMachine::TCoreFactoryHolder {};
    class TMarketCores : public NTextMachine::TCoreFactoryHolder {};
    class TMarketGoodsParallelCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvRsyaCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvContextMachineCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvSpyMachineCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvSearchCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvSearchTnCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvSearchSynCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvMarketCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvSearchDynamicCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvPhraseCores : public NTextMachine::TCoreFactoryHolder {};
    class TAdvPhrase2Cores : public NTextMachine::TCoreFactoryHolder {};
    class TGeoMachineCores : public NTextMachine::TCoreFactoryHolder {};
    class TMusicCores : public NTextMachine::TCoreFactoryHolder {};
    class TKinopoiskCores : public NTextMachine::TCoreFactoryHolder {};
} // NTextMachine
