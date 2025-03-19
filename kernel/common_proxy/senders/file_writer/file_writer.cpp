#include "file_writer.h"
#include <util/stream/file.h>

namespace NCommonProxy {
    class TFileWriter::TInputMetaData : public TMetaData {
    public:
        TInputMetaData() {
            Register("post", TMetaData::dtBLOB);
        }
    };

    TFileWriter::TFileWriter(const TString& name, const TProcessorsConfigs& configs)
        : TSender(name, configs)
        , Config(*configs.Get<TConfig>(name))
    {}

    void TFileWriter::DoStart() {
        Output.Reset(new TFixedBufferFileOutput(Config.OutputFile));
    }

    void TFileWriter::DoStop() {
        Output.Reset(nullptr);
    }

    void TFileWriter::DoWait() {
    }

    const NCommonProxy::TMetaData& TFileWriter::GetInputMetaData() const {
        return Default<TInputMetaData>();
    }

    const NCommonProxy::TMetaData& TFileWriter::GetOutputMetaData() const {
        return TMetaData::Empty;
    }

    void TFileWriter::DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const {
        TGuard<TMutex> g(Mutex);
        TBlob doc = input->Get<TBlob>("post");
        Output->Write(doc.AsCharPtr(), doc.Size());
        replier->AddReply(GetName());
    }

    bool TFileWriter::TConfig::DoCheck() const {
        return true;
    }

    void TFileWriter::TConfig::DoInit(const TYandexConfig::Section& componentSection) {
        TProcessorConfig::DoInit(componentSection);
        const TYandexConfig::Directives& dir = componentSection.GetDirectives();
        TString outFile;
        if (!dir.GetValue("Output", outFile))
            ythrow yexception() << "Cannot create FILE_WRITER: set Output";
        OutputFile = outFile;
    }

    void TFileWriter::TConfig::DoToString(IOutputStream& so) const {
        TProcessorConfig::DoToString(so);
        so << "Output: " << OutputFile << Endl;
    }

}
