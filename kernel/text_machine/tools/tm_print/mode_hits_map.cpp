#include "mode_hits_map.h"
#include "hits_map.h"

#include <search/tools/idx_proto/internal_format.h>

#include <kernel/reqbundle/serializer.h>
#include <kernel/text_machine/proto/text_machine.pb.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/getopt/last_getopt.h>


class TInputContext {
public:
    TInputContext(IInputStream& input, EInputFormat format)
        : Input(input)
        , Format(format)
    {
        Init();
    }

    void Init() {
        switch (Format) {
            case EInputFormat::ProtoPool: {
                PoolReader = MakeHolder<NIdxOps::TFileReader<NFeaturePool::TLine>>(Input);
                break;
            }
            case EInputFormat::Base64Hits: {
                break;
            }
        }
    }

    bool Next(NFeaturePool::TLine& line) {
        switch (Format) {
            case EInputFormat::ProtoPool: {
                return PoolReader->Next(line);
            }
            case EInputFormat::Base64Hits: {
                TString base64;
                if (!Input.ReadLine(base64)) {
                    return false;
                }

                TString binary = Base64Decode(base64);
                line.Clear();
                Y_ENSURE(
                    line.MutableTMHits()->ParseFromString(binary),
                    "failed to parse TPbDocHits proto");
                return true;
            }
        }
    }

private:
    IInputStream& Input;
    EInputFormat Format = EInputFormat::ProtoPool;

    THolder<NIdxOps::TFileReader<NFeaturePool::TLine>> PoolReader;
};

struct TPrinterContext {
    NReqBundle::TReqBundlePtr Bundle;
};

THolder<THitsMap::IPrinter> CreatePrinter(
    THitsMap::EPrinterType printerType,
    const NFeaturePool::TLine& line,
    TPrinterContext& context)
{
    static const TSet<THitsMap::EPrinterType> printersNeedBundle = {
        THitsMap::EPrinterType::Word,
        THitsMap::EPrinterType::Form,
        THitsMap::EPrinterType::Expansion,
        THitsMap::EPrinterType::RequestId
    };

    if (printersNeedBundle.contains(printerType)) {
        context.Bundle = new NReqBundle::TReqBundle;
        NReqBundle::NSer::TDeserializer deser;
        TString binary = line.GetTMHits().HasBinaryBundle() ? line.GetTMHits().GetBinaryBundle() : Base64Decode(line.GetTMHits().GetQBundleBase64());
        deser.Deserialize(binary, *context.Bundle);
        context.Bundle->Sequence().PrepareAllBlocks(deser);
    }

    switch (printerType) {
        case THitsMap::EPrinterType::Token: {
            return THitsMap::CreateTokenPrinter();
        }
        case THitsMap::EPrinterType::BlockId: {
            return THitsMap::CreateBlockIdPrinter();
        }
        case THitsMap::EPrinterType::Word: {
            return THitsMap::CreateWordPrinter(context.Bundle->GetSequence());
        }
        case THitsMap::EPrinterType::Form: {
            return THitsMap::CreateFormPrinter(context.Bundle->GetSequence());
        }
        case THitsMap::EPrinterType::Expansion:  {
            return THitsMap::CreateExpansionPrinter(*context.Bundle);
        }
        case THitsMap::EPrinterType::RequestId: {
            return THitsMap::CreateRequestIdPrinter(*context.Bundle);
        }
        case THitsMap::EPrinterType::Stream: {
            return THitsMap::CreateStreamPrinter();
        }
        case THitsMap::EPrinterType::BreakId: {
            return THitsMap::CreateBreakIdPrinter();
        }
    }
}

template <bool IsRequest>
int MainHitsMap(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    if (IsRequest) {
        opts.SetTitle("Use proto-pool data to print text machine hits pattern in requests");
    } else {
        opts.SetTitle("Use proto-pool data to print text machine hits pattern in document or stream");
    }

    TString inputPath;
    opts.AddLongOption('i', "input", "input proto pool or base64-encoded TPbDocHits proto")
        .Optional()
        .RequiredArgument("PATH")
        .DefaultValue("-")
        .StoreResult(&inputPath);

    EInputFormat inputFormat = EInputFormat::ProtoPool;
    opts.AddLongOption('f', "format", "input format: proto pool or base64-encoded TPbDocHits proto")
        .Optional()
        .RequiredArgument("FORMAT")
        .DefaultValue("ProtoPool")
        .StoreResult(&inputFormat);

    THitsMap::EPrinterType printerType = THitsMap::EPrinterType::Token;
    opts.AddLongOption('p', "printer", "printer type")
        .Optional()
        .RequiredArgument("TYPE")
        .DefaultValue("Token")
        .StoreResult(&printerType);

    TMaybe<NLingBoost::EExpansionType> expansionType;
    opts.AddLongOption('e', "expansion", "expansion type")
        .Optional()
        .RequiredArgument("TYPE")
        .StoreResultT<NLingBoost::EExpansionType>(&expansionType);

    TMaybe<NLingBoost::EStreamType> streamType;
    opts.AddLongOption('s', "stream", "stream type")
        .Optional()
        .RequiredArgument("TYPE")
        .StoreResultT<NLingBoost::EStreamType>(&streamType);

    TMaybe<TString> urlFilter;
    opts.AddLongOption('u', "url", "print hits only for this url")
        .Optional()
        .RequiredArgument("URL")
        .StoreResultT<TString>(&urlFilter);

    NLastGetopt::TOptsParseResult optsResult{&opts, argc, argv};

    THolder<IInputStream> input = OpenInput(inputPath);

    TInputContext inputCtx{*input, inputFormat};
    NFeaturePool::TLine line;
    while(inputCtx.Next(line)) {
        if (urlFilter.Defined() && line.GetMainRobotUrl() != urlFilter) {
            continue;
        }
        Cout
            << "---- LINE: "
            << line.GetRequestId()
            << ", " << line.GetDocId()
            << " [" << line.GetMainRobotUrl() << "]"
            << " ----" << Endl;

        if (line.HasTMHits()) {
            const NTextMachineProtocol::TPbDocHits& hits = line.GetTMHits();
            THolder<THitsMap::TDoc> doc;

            if (IsRequest) {
                doc = THitsMap::CreateForRequest(hits, expansionType, streamType);
            } else {
                doc = THitsMap::CreateForDoc(hits, expansionType, streamType);
            }

            TPrinterContext printerContext;
            THolder<THitsMap::IPrinter> printer = CreatePrinter(printerType, line, printerContext);
            printer->PrintDoc(Cout, *doc);
        }
    }

    return 0;
}

template int MainHitsMap<false>(int, const char*[]);
template int MainHitsMap<true>(int, const char*[]);
