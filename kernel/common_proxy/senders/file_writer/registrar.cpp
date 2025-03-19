#include "file_writer.h"

namespace NCommonProxy {

    static const TString FILE_WRITER = "FILE_WRITER";

    TProcessor::TFactory::TRegistrator<TFileWriter> TFileWriter::Registarar(FILE_WRITER);
    NCommonProxy::TProcessorConfig::TFactory::TRegistrator<TFileWriter::TConfig> TFileWriter::TConfig::Registrator(FILE_WRITER);

}
