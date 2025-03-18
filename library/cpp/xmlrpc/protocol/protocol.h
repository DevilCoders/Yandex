#pragma once

#include "value.h"
#include "rpcfault.h"

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

class IInputStream;
class IOutputStream;

namespace NXmlRPC {
    //client part
    void SerializeRequest(const TStringBuf& func, const TArray& params, IOutputStream& out);
    TString SerializeRequest(const TStringBuf& func, const TArray& params);
    TValue ParseResult(IInputStream& in);
    TValue ParseResult(const TStringBuf& res);

    //server part
    void SerializeReply(const TValue& result, IOutputStream& out);
    TString SerializeReply(const TValue& result);
    void SerializeFailure(const TValue& fail, IOutputStream& out);
    TString SerializeFailure(const TValue& fail);
    TValue GenericFailure(int code, const TString& descr);
    void ParseRequest(IInputStream& in, TString* funcname, TArray* params);
    void ParseRequest(const TStringBuf& req, TString* funcname, TArray* params);
}
