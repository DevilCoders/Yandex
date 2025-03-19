#include "protoconf.h"

#include <kernel/gazetteer/articlepool.h>

namespace NProtoConf {

    bool LoadFromFile(const TString& fileName, const NProtoBuf::Descriptor* descr, NProtoBuf::Message& res) {
        return NGzt::TArticlePoolBuilder::ParseSingleArticle(fileName, *descr, res);
    }

}   // namespace NProtoConf
