#pragma once

#include "offroad_io_factory.h"

namespace NDoom {


template<class IndexReader>
class TIndexTables: public IndexReader::TKeyTable, public IndexReader::THitTable {
public:
    using TKeyModel = typename IndexReader::TKeyModel;
    using THitModel = typename IndexReader::THitModel;
    using TKeyTable = typename IndexReader::TKeyTable;
    using THitTable = typename IndexReader::THitTable;

    TIndexTables(TOffreadReaderInputs* inputs) {
        THitModel hitModel;
        hitModel.Load(inputs->HitModelStream.Get());
        THitTable::Reset(hitModel);

        TKeyModel keyModel;
        keyModel.Load(inputs->KeyModelStream.Get());
        TKeyTable::Reset(keyModel);
    }

    TIndexTables(const THitModel& hitModel, const TKeyModel& keyModel) {
        THitTable::Reset(hitModel);
        TKeyTable::Reset(keyModel);
    }
};


} // namespace NDoom

