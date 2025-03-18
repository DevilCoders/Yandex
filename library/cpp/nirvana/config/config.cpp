#include "config.h"

#include <util/stream/file.h>

namespace {
    using TLoader = NConfig::TConfig (*)(IInputStream& in, const NConfig::TGlobals&);

    const THashMap<TString, TLoader> LOADER_REGISTRY{
        {"json", &NConfig::TConfig::FromJson},
        {"lua", &NConfig::TConfig::FromLua},
        {"ini", &NConfig::TConfig::FromIni},
    };
}

namespace NNirvana {
    NConfig::TConfig LoadFromFile(const TFsPath& filename, const NConfig::TGlobals& globals) {
        const auto ext = filename.GetExtension();

        TFileInput input(filename);
        if (!LOADER_REGISTRY.contains(ext)) {
            return NConfig::TConfig::FromStream(input, globals);
        }

        const auto loader = LOADER_REGISTRY.at(ext);
        return loader(input, globals);
    }
}
