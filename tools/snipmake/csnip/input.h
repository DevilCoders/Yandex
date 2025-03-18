#pragma once

#include "job.h"

#include <google/protobuf/messagext.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/input.h>

namespace NSnippets {

    struct TRandomFilter {
        size_t P1 = 0;
        size_t P = 0;

        TRandomFilter(float p);
        bool Accept();
    };

    struct TInputFilter {
        TRandomFilter RndFilter;
        TString NextLine;
        THolder<IInputStream> LinesFile;
        size_t Line = 0;

        TInputFilter(float p, const TString& linesFile);
        bool Accept();
        bool AcceptLineFile();
    };

    struct TStreamInput : public IInputProcessor {
        TContextData Data;
        THolder<IInputStream> Input;
        size_t Line = 0;
        TString Tmp;

        TStreamInput(IInputStream* input);

        /**
         * Possible input formats
         * context
         * id<TAB>context
         * requestId<TAB>snippetId<TAB>context
         */
        bool Next() override;

        TContextData& GetContextData() override;
    };

    struct TNSAInput : public IInputStream {
        TString Bro;
        IInputStream* Slave;

        explicit TNSAInput(IInputStream* slave);
        size_t DoSkip(size_t len) override;
        size_t DoRead(void* buf, size_t len) override;
    };

    struct THumanStreamInput : public IInputProcessor {
        TContextData Data;
        THolder<IInputStream> Input;
        TNSAInput CInput;
        google::protobuf::io::TCopyingInputStreamAdaptor GInput;
        bool ReadOnce = false;

        THumanStreamInput(IInputStream* input);

        bool Next() override;
        TContextData& GetContextData() override;
    };

    struct TStdInput : public TStreamInput {
        TStdInput();
    };

    struct THumanStdInput : public THumanStreamInput {
        THumanStdInput();
    };

    struct TUnbufferedFileInput : public TStreamInput {
        TUnbufferedFileInput(TString fileName);
    };

    struct THumanFileInput : public THumanStreamInput {
        THumanFileInput(TString fileName);
    };

} //namespace NSnippets
