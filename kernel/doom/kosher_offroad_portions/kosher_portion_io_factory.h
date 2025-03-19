#pragma once

#include <kernel/doom/standard_models_storage/standard_models_storage.h>

namespace NDoom {

template <class Io>
class TKosherPortionIoFactory {
public:
    using TIo = Io;
    using TKeyModel = typename TIo::TKeyModel;
    using THitModel = typename TIo::THitModel;
    using TWriter = typename TIo::TWriter;
    using TReader = typename TIo::TReader;
    using TKeyWriterTable = typename TWriter::TKeyTable;
    using THitWriterTable = typename TWriter::THitTable;
    using TKeyReaderTable = typename TReader::TKeyTable;
    using THitReaderTable = typename TReader::THitTable;

    static const TKeyModel& KeyModel() {
        static const TKeyModel model = TStandardIoModelsStorage::Model<TKeyModel>(Io::DefaultKeyModel);
        return model;
    }

    static const THitModel& HitModel() {
        static const THitModel model = TStandardIoModelsStorage::Model<THitModel>(Io::DefaultHitModel);
        return model;
    }

    static const TKeyWriterTable* KeyWriterTable() {
        static const TKeyWriterTable table(KeyModel());
        return &table;
    }

    static const THitWriterTable* HitWriterTable() {
        static const THitWriterTable table(HitModel());
        return &table;
    }

    static const TKeyReaderTable* KeyReaderTable() {
        static const TKeyReaderTable table(KeyModel());
        return &table;
    }

    static const THitReaderTable* HitReaderTable() {
        static const THitReaderTable table(HitModel());
        return &table;
    }
};

} // namespace NDoom
