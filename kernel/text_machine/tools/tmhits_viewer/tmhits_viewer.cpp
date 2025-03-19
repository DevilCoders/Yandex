#include "hits_map.h"

#include <kernel/reqbundle/serializer.h>
#include <kernel/text_machine/proto/text_machine.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/html/escape/escape.h>
#include <library/cpp/http/server/http_ex.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/subst.h>

namespace {
    static THolder<THitsMap::IPrinter> CreatePrinter(
        THitsMap::EPrinterType printerType,
        const NTextMachineProtocol::TPbDocHits& parsed,
        NReqBundle::TReqBundlePtr& bundle)
    {
        static const TSet<THitsMap::EPrinterType> printersNeedBundle = {
            THitsMap::EPrinterType::Word,
            THitsMap::EPrinterType::Expansion,
            THitsMap::EPrinterType::RequestId
        };

        if (printersNeedBundle.contains(printerType)) {
            bundle = new NReqBundle::TReqBundle;
            NReqBundle::NSer::TDeserializer deser;
            TString binary = parsed.HasBinaryBundle() ? parsed.GetBinaryBundle() : Base64Decode(parsed.GetQBundleBase64());
            deser.Deserialize(binary, *bundle);
            bundle->Sequence().PrepareAllBlocks(deser);
        }

        switch (printerType) {
            case THitsMap::EPrinterType::Token: {
                return THitsMap::CreateTokenPrinter();
            }
            case THitsMap::EPrinterType::BlockId: {
                return THitsMap::CreateBlockIdPrinter();
            }
            case THitsMap::EPrinterType::Word: {
                return THitsMap::CreateWordPrinter(bundle->GetSequence());
            }
            case THitsMap::EPrinterType::Expansion:  {
                return THitsMap::CreateExpansionPrinter(*bundle);
            }
            case THitsMap::EPrinterType::RequestId: {
                return THitsMap::CreateRequestIdPrinter(*bundle);
            }
            case THitsMap::EPrinterType::Stream: {
                return THitsMap::CreateStreamPrinter();
            }
            case THitsMap::EPrinterType::BreakId: {
                return THitsMap::CreateBreakIdPrinter();
            }
        }
    }

    TString GenerateText(const TCgiParameters& data)
    {
        const TString& dochits = data.Get("data");
        const TString& mode = data.Get("mode");
        const TString& expansionString = data.Get("expansion");
        const TString& streamString = data.Get("stream");
        TString result;
        TStringBuf hitsBuf(dochits);
        while (hitsBuf && (hitsBuf.back() == '\r' || hitsBuf.back() == '\n'))
            hitsBuf.Chop(1);
        if (hitsBuf) {
            NTextMachineProtocol::TPbDocHits parsed;
            Y_ENSURE(parsed.ParseFromString(Base64Decode(hitsBuf)));
            THitsMap::EPrinterType printerType = FromString(data.Get("printer"));
            TMaybe<NLingBoost::EExpansionType> expansion;
            if (expansionString)
                expansion = FromString<NLingBoost::EExpansionType>(expansionString);
            TMaybe<NLingBoost::EStreamType> stream;
            if (streamString)
                stream = FromString<NLingBoost::EStreamType>(streamString);
            if (mode == "prototext") {
                result = parsed.Utf8DebugString();
            } else if (mode == "limits-monitor") {
                result = parsed.GetLimitsMonitor().Utf8DebugString();
            } else {
                THolder<THitsMap::TDoc> doc;
                if (mode == "rhits-map") {
                    doc = THitsMap::CreateForRequest(parsed, expansion, stream);
                } else {
                    doc = THitsMap::CreateForDoc(parsed, expansion, stream);
                }
                TStringOutput out(result);
                NReqBundle::TReqBundlePtr bundle;
                THolder<THitsMap::IPrinter> printer = CreatePrinter(printerType, parsed, bundle);
                printer->PrintDoc(out, *doc);
            }
        }
        return result;
    }

    template<typename T>
    TString GenerateOptionsList(TArrayRef<T> values, TStringBuf selected, bool addEmpty, const TStringBuf* humanReadable)
    {
        TString result;
        if (addEmpty)
            result = TString::Join("<option ", selected ? "" : "selected ", "value=\"\">(no filter)</option>");
        size_t index = 0;
        for (auto value : values) {
            TString str = ToString(value);
            result += TString::Join("<option ", str != selected ? "" : "selected ", "value=\"", str, "\">", humanReadable ? humanReadable[index] : str, "</option>");
            index++;
        }
        return result;
    }

    enum EInsertPoint
    {
        EInsertData,
        EInsertModeOptions,
        EInsertPrinterOptions,
        EInsertExpansionOptions,
        EInsertStreamOptions,
        EInsertResult,
        EInsertMax
    };

    static const THashMap<TStringBuf, EInsertPoint> names = {
        {TStringBuf("$data$"), EInsertData},
        {TStringBuf("$mode_options$"), EInsertModeOptions},
        {TStringBuf("$printer_options$"), EInsertPrinterOptions},
        {TStringBuf("$expansion_options$"), EInsertExpansionOptions},
        {TStringBuf("$stream_options$"), EInsertStreamOptions},
        {TStringBuf("$result$"), EInsertResult},
    };

    struct TPageTemplate
    {
        TVector<std::pair<TStringBuf, EInsertPoint>> Texts;
        TStringBuf Last;
    };

    TPageTemplate ParsePageTemplate(const TString& text)
    {
        TPageTemplate result;
        size_t pos = 0;
        size_t textStart = 0;
        for (;;) {
            pos = text.find('$', pos);
            if (pos == TString::npos)
                break;
            size_t pos2 = text.find('$', pos + 1);
            if (pos2 == TString::npos)
                break;
            const EInsertPoint* p = names.FindPtr(TStringBuf(text).SubStr(pos, pos2 - pos + 1));
            if (p) {
                result.Texts.emplace_back(TStringBuf(text).SubStr(textStart, pos - textStart), *p);
                textStart = pos = pos2 + 1;
            } else {
                pos = pos2;
            }
        }
        result.Last = TStringBuf(text).Tail(textStart);
        return result;
    }

    TString GeneratePage(const TCgiParameters& data, const TString& result, const TPageTemplate& page)
    {
        static const TStringBuf allModes[] = {TStringBuf("hits-map"), TStringBuf("rhits-map"), TStringBuf("limits-monitor"), TStringBuf("prototext")};
        static const TStringBuf allModesHR[] = {TStringBuf("Document hits"), TStringBuf("Request hits"), TStringBuf("Limits monitor"), TStringBuf("Dump full prototext")};
        static const THitsMap::EPrinterType allPrinters[] = {
            THitsMap::EPrinterType::Token,
            THitsMap::EPrinterType::BlockId,
            THitsMap::EPrinterType::Word,
            THitsMap::EPrinterType::Expansion,
            THitsMap::EPrinterType::RequestId,
            THitsMap::EPrinterType::Stream,
            THitsMap::EPrinterType::BreakId,
        };
        TString insertPoints[EInsertMax];
        insertPoints[EInsertData] = NHtml::EscapeText(data.Get("data"));
        insertPoints[EInsertModeOptions] = GenerateOptionsList(
            TArrayRef<const TStringBuf>(std::begin(allModes), std::end(allModes)),
            data.Has("mode") ? TStringBuf(data.Get("mode")) : TStringBuf("hits-map"),
            false,
            allModesHR
        );
        insertPoints[EInsertPrinterOptions] = GenerateOptionsList(
            TArrayRef<const THitsMap::EPrinterType>(std::begin(allPrinters), std::end(allPrinters)),
            data.Has("printer") ? TStringBuf(data.Get("printer")) : TStringBuf("Word"),
            false,
            nullptr
        );
        insertPoints[EInsertExpansionOptions] = GenerateOptionsList(NLingBoost::TExpansionStruct::GetValues(), data.Get("expansion"), true, nullptr);
        insertPoints[EInsertStreamOptions] = GenerateOptionsList(NLingBoost::TStreamStruct::GetValues(), data.Get("stream"), true, nullptr);
        insertPoints[EInsertResult] = NHtml::EscapeText(result);
        TString finalPage;
        size_t finalPageSize = page.Last.Size();
        for (const auto& it : page.Texts)
            finalPageSize += it.first.Size() + insertPoints[it.second].Size();
        finalPage.reserve(finalPageSize);
        for (const auto& it : page.Texts) {
            finalPage += it.first;
            finalPage += insertPoints[it.second];
        }
        finalPage += page.Last;
        return finalPage;
    }

    class TRequest final : public THttpClientRequestExtension
    {
        THttpServer& Parent;
        const TString& ExternalHtmlName;
        const TPageTemplate& BuiltinPage;
    public:
        TRequest(THttpServer& parent, const TString& externalHtmlName, const TPageTemplate& builtinPage)
            : Parent(parent)
            , ExternalHtmlName(externalHtmlName)
            , BuiltinPage(builtinPage)
        {}

        bool Reply(void*) override {
            try {
                Cout << "start: " << Now().ToString() << '\n';
                TServerRequestData parsed;
                TBlob postData;
                if (!ProcessHeaders(parsed, postData))
                    return true;
                Cout << "got data: " << Now().ToString() << '\n';
                parsed.Scan();
                TStringBuf path(parsed.ScriptName());
                if (path == "/admin" && parsed.CgiParam.Get("action") == "shutdown") {
                    if (!CheckLoopback())
                        return true;
                    Output() << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nshutting down\n";
                    Parent.Shutdown();
                    return true;
                }
                if (path == "/dochits/favicon.png") {
                    TString favicon = NResource::Find("/favicon.png");
                    Output() << "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: " << favicon.size() << "\r\n\r\n" << favicon;
                    return true;
                }
                if (path != "/dochits" && path != "/dochits/text") {
                    Output() << "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\n404 Not Found\n";
                    return true;
                }
                parsed.CgiParam.ScanAdd(TStringBuf(postData.AsCharPtr(), postData.Size()));
                TString text;
                bool textOk = true;
                try {
                    text = GenerateText(parsed.CgiParam);
                } catch (yexception& e) {
                    text = TString::Join("exception: ", e.what());
                    textOk = false;
                } catch (...) {
                    text = "[error]";
                    textOk = false;
                }
                if (path == "/dochits") {
                    TString result;
                    if (ExternalHtmlName) {
                        TIFStream f(ExternalHtmlName);
                        TString externalHtml = f.ReadAll();
                        TPageTemplate page = ParsePageTemplate(externalHtml);
                        result = GeneratePage(parsed.CgiParam, text, page);
                    } else {
                        result = GeneratePage(parsed.CgiParam, text, BuiltinPage);
                    }
                    Cout << "end GeneratePage: " << Now().ToString() << '\n';
                    Output() << "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " << result.Size() << "\r\n\r\n" << result;
                } else {
                    Cout << "end GenerateText: " << Now().ToString() << '\n';
                    Output() << "HTTP/1.1 " << (textOk ? "200 OK" : "400 Bad Request") << "\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: " << text.Size() << "\r\n\r\n" << text;
                }
                Cout << "done: " << Now().ToString() << '\n';
            } catch (...) {
                Output() << "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nsomething went wrong\n";
            }
            return true;
        }
    };

    class TCallback final : public THttpServer::ICallBack
    {
        THttpServer* Parent = nullptr;
        TString ExternalHtmlName;
        TString BuiltinHtml;
        TPageTemplate BuiltinPage;
    public:
        TCallback(const TString& externalHtmlName) {
            ExternalHtmlName = externalHtmlName;
            if (!externalHtmlName) {
                BuiltinHtml = NResource::Find("/dochits.html");
                BuiltinPage = ParsePageTemplate(BuiltinHtml);
            }
        }
        void SetServer(THttpServer& parent) {
            Y_ASSERT(!Parent);
            Parent = &parent;
        }
        TClientRequest* CreateClient() override {
            Y_ASSERT(Parent);
            return new TRequest(*Parent, ExternalHtmlName, BuiltinPage);
        }
    };
}

int main(int argc, const char* argv[])
{
    try {
        NLastGetopt::TOpts opts;
        TString externalHtmlName;
        opts.AddLongOption('h', "html", "[for development] serve the page from this file, reload on every query").RequiredArgument("FILE").StoreResult(&externalHtmlName);
        opts.SetFreeArgsNum(1);
        opts.SetFreeArgTitle(0, "PORT", "TCP port to serve on");
        NLastGetopt::TOptsParseResult cmdLine(&opts, argc, argv);

        ui16 port = FromString<ui16>(cmdLine.GetFreeArgs()[0]);

        TCallback callback(externalHtmlName);
        THttpServer server(&callback, THttpServerOptions(port)
            .EnableKeepAlive(true)
            .EnableCompression(true));
        callback.SetServer(server);
        if (!server.Start()) {
            Cerr << "failed to start the server: " << server.GetError() << '\n';
            return 1;
        }
        server.Wait();
    } catch (yexception& e) {
        Cerr << "exception: " << e.what() << '\n';
        return 1;
    } catch (...) {
        Cerr << "unknown exception\n";
        return 1;
    }
    return 0;
}
