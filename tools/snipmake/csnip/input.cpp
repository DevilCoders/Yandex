#include "input.h"

#include <kernel/snippets/idl/snippets.pb.h>

#include <google/protobuf/text_format.h>

#include <util/generic/strbuf.h>
#include <util/random/random.h>
#include <util/stream/buffered.h>
#include <util/stream/file.h>

namespace NSnippets {

    TRandomFilter::TRandomFilter(float p) {
        if (p < 0) {
            P = 0;
            P1 = 0;
        } else {
            P = 1000000;
            P1 = p * P;
        }
    }

    bool TRandomFilter::Accept() {
        if (!P || RandomNumber<size_t>(P) < P1) {
            return true;
        }
        return false;
    }


    TInputFilter::TInputFilter(float p, const TString& linesFile)
      : RndFilter(p)
    {
        if (linesFile) {
            LinesFile.Reset(new TFileInput(linesFile));
        }
    }

    bool TInputFilter::Accept() {
        ++Line;
        if (LinesFile.Get() && !AcceptLineFile()) {
            return false;
        }
        return RndFilter.Accept();
    }

    bool TInputFilter::AcceptLineFile() {
        while (!NextLine) {
            if (!LinesFile->ReadLine(NextLine)) {
                NextLine = "nope";
                return false;
            }
        }
        if (NextLine == ToString(Line)) {
            NextLine.clear();
            return true;
        } else {
            return false;
        }
    }


    TStreamInput::TStreamInput(IInputStream* input)
        : Input(input)
    {
    }

    bool TStreamInput::Next() {
        ++Line;
        if (!Input->ReadLine(Tmp)) {
            return false;
        }
        size_t t = Tmp.find('\t');
        TString recId;
        TString recSubId;
        TStringBuf data;
        if (t != TString::npos) {
            recId = TStringBuf(Tmp.data(), Tmp.data() + t);
            data = TStringBuf(Tmp.data() + t + 1, Tmp.data() + Tmp.size());
            t = data.find('\t');
            if (t != TString::npos) {
                recSubId = TStringBuf(data.data(), data.data() + t);
                data = TStringBuf(data.data() + t + 1, data.data() + data.size());
            }
        } else {
            recId = ToString<size_t>(Line);
            data = Tmp;
        }
        Data.SetFromBase64Data(recId, recSubId, data);
        return true;
    }

    TContextData& TStreamInput::GetContextData() {
        return Data;
    }


    TNSAInput::TNSAInput(IInputStream* slave)
      : Slave(slave)
    {
    }

    size_t TNSAInput::DoSkip(size_t len) {
        return Slave->Skip(len);
    }

    size_t TNSAInput::DoRead(void* buf, size_t len) {
        const size_t res = Slave->Read(buf, len);
        Bro.append(static_cast<const char*>(buf), res);
        return res;
    }


    THumanStreamInput::THumanStreamInput(IInputStream* input)
      : Input(input)
      , CInput(Input.Get())
      , GInput(&CInput)
    {
    }

    bool THumanStreamInput::Next() {
        if (ReadOnce) {
            return false;
        }
        ReadOnce = true;
        NSnippets::NProto::TSnippetsCtx ctx;
        if (!google::protobuf::TextFormat::Parse(&GInput, &ctx)) {
            return false;
        }
        if (CInput.Bro.empty()) {
            return false;
        }
        Data.SetFromProtobufData("1", "", CInput.Bro, ctx);
        return true;
    }

    TContextData& THumanStreamInput::GetContextData() {
        return Data;
    }


    TStdInput::TStdInput()
        : TStreamInput(new TBufferedInput(&Cin))
    {
    }


    THumanStdInput::THumanStdInput()
        : THumanStreamInput(new TBufferedInput(&Cin))
    {
    }


    TUnbufferedFileInput::TUnbufferedFileInput(TString fileName)
        : TStreamInput(new TFileInput(fileName))
    {
    }


    THumanFileInput::THumanFileInput(TString fileName)
        : THumanStreamInput(new TFileInput(fileName))
    {
    }


} //namespace NSnippets
