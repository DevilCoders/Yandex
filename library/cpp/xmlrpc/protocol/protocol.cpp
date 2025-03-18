#include "protocol.h"
#include "xml.h"

#include <util/stream/output.h>
#include <util/stream/mem.h>

namespace NXmlRPC {
    //client part
    void SerializeRequest(const TStringBuf& func, const TArray& params, IOutputStream& out) {
        out << "<?xml version=\"1.0\"?>\n<methodCall><methodName>"sv << func << TStringBuf("</methodName><params>");

        for (const auto& param : params) {
            out << TStringBuf("<param>") << param << TStringBuf("</param>");
        }

        out << TStringBuf("</params></methodCall>");
    }

    TString SerializeRequest(const TStringBuf& func, const TArray& params) {
        TStringStream ss;

        SerializeRequest(func, params, ss);

        return ss.Str();
    }

    TValue ParseResult(IInputStream& in) {
        const TString data = in.ReadAll();

        if (!data) {
            ythrow TXmlRPCError() << "empty response";
        }

        NXml::TDocument doc(data, NXml::TDocument::String);
        NXml::TConstNode root(FirstChild(doc.Root()));

        if (root.Name() == TStringBuf("params")) {
            return TValue(FirstChild(FirstChild(root)));
        }

        ythrow TXmlRPCFault(TValue(FirstChild(root))) << "xml rpc fault";
    }

    TValue ParseResult(const TStringBuf& res) {
        TMemoryInput mi(res.data(), res.size());

        return ParseResult(mi);
    }

    //server part
    void SerializeReply(const TValue& result, IOutputStream& out) {
        out << "<?xml version=\"1.0\"?>\n<methodResponse><params><param>"sv << result << TStringBuf("</param></params></methodResponse>");
    }

    TString SerializeReply(const TValue& result) {
        TStringStream ss;

        SerializeReply(result, ss);

        return ss.Str();
    }

    void SerializeFailure(const TValue& fail, IOutputStream& out) {
        out << "<?xml version=\"1.0\"?><methodResponse><fault>"sv << fail << TStringBuf("</fault></methodResponse>");
    }

    TString SerializeFailure(const TValue& fail) {
        TStringStream ss;

        SerializeFailure(fail, ss);

        return ss.Str();
    }

    TValue GenericFailure(int code, const TString& descr) {
        return TStruct().Set("failtCode", code).Set("faultString", descr);
    }

    void ParseRequest(IInputStream& in, TString* funcname, TArray* params) {
        const TString data = in.ReadAll();

        if (!data) {
            ythrow TXmlRPCError() << "empty request";
        }

        NXml::TDocument doc(data, NXml::TDocument::String);
        NXml::TConstNode root(doc.Root());

        *funcname = root.Node("methodName").Value<TString>();

        NXml::TConstNodes nodes = root.Node("params").Nodes("param");

        for (size_t i = 0; i < nodes.Size(); ++i) {
            params->push_back(TValue(FirstChild(nodes[i])));
        }
    }

    void ParseRequest(const TStringBuf& req, TString* funcname, TArray* params) {
        TMemoryInput mi(req.data(), req.size());

        ParseRequest(mi, funcname, params);
    }
}
