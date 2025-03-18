#include "processor.h"
#include <util/system/file.h>
#include <util/stream/output.h>

namespace NSnippets {

class TThreadData::TImpl {
public:
    bool DoLog = false;
    TFileHandle DeathContext;

    TImpl(bool log, const TString& name)
       : DoLog(log)
       , DeathContext(DoLog ? TFileHandle(name, OpenAlways | RdWr) : TFileHandle(INVALID_FHANDLE))
    {
        if (DoLog) {
            if (!DeathContext.IsOpen()) {
                Cerr << "Failed to open death log: " << name << Endl;
                DoLog = false;
                return;
            }
            Cerr << "death log: " << name << Endl;
            Log("");
        }
    }

    void Log(const TStringBuf& buf) {
        if (DoLog) {
            DeathContext.Seek(0, sSet);
            DeathContext.Resize(0);
            DeathContext.Write(buf.data(), buf.size());
            DeathContext.Write("\n", 1);
            DeathContext.Flush();
        }
    }

    void Log(const IContextData& ctxData) {
        Log(TStringBuf(ctxData.GetRawInput().data(), ctxData.GetRawInput().size()));
    }
};

TThreadData::TThreadData(bool log, const TString& name)
    : Impl(new TImpl(log, name))
{
}

TThreadData::~TThreadData() {
}

void TThreadData::Log(const IContextData& ctxData) {
    Impl->Log(ctxData);
}

};
