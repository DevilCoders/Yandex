#include "server.h"

#include <library/cpp/xmlrpc/protocol/protocol.h>
#include <library/cpp/xmlrpc/protocol/compress.h>

using namespace NNeh;
using namespace NXmlRPC;

namespace {
    struct TServer: public IServer, public THashMap<TString, TXmlRPCCallback> {
        void ServeRequest(const IRequestRef& request) override {
            TDataSaver res;

            try {
                TString funcname;
                TArray params;

                if (request->Scheme() == TStringBuf("http")) {
                    ParseRequest(request->Data(), &funcname, &params);
                } else {
                    ParseRequest(DecompressXml(request->Data()), &funcname, &params);
                }

                if (TXmlRPCCallback* cb = FindPtr(funcname)) {
                    SerializeReply((*cb)(params), res);
                } else {
                    ythrow TXmlRPCError() << "function " << funcname << "not registered";
                }
            } catch (const TXmlRPCFault& f) {
                res.clear();

                SerializeFailure(f.Fault(), res);
            } catch (...) {
                res.clear();

                SerializeFailure(GenericFailure(-1, CurrentExceptionMessage()), res);
            }

            request->SendReply(res);
        }

        void Add(const TString& funcname, TXmlRPCCallback cb) override {
            (*this)[funcname] = cb;
        }
    };
}

IServerRef NXmlRPC::CreateServer() {
    return new TServer();
}
