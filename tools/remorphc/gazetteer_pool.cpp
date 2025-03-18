#include "gazetteer_pool.h"


namespace NRemorphCompiler {

TGazetteerPool::TGazetteerPool(IOutputStream* log)
    : Pool()
    , Log(log)
{
}

const NGzt::TGazetteer* TGazetteerPool::Add(const TString& path) {
    auto inserted = Pool.insert(::std::make_pair(path, TAutoPtr<NGzt::TGazetteer>()));
    auto found = inserted.first;
    if (inserted.second) {
        found->second.Reset(new NGzt::TGazetteer(path));
        if (Log) {
            *Log << "Gazetteer: " << path << Endl;
        }
    }
    return found->second.Get();
}

} // NRemorphCompiler
