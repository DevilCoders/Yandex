#include "rpcfault.h"

using namespace NXmlRPC;

TXmlRPCFault::TXmlRPCFault(const TValue& v)
    : V_(v)
{
    try {
        (*this) << String() << TStringBuf(" at ");
    } catch (const TXmlRPCError&) {
        //ignore non-standart error code
    }
}

int TXmlRPCFault::Code() const {
    return Cast<int>(V_[TStringBuf("faultCode")]);
}

TString TXmlRPCFault::String() const {
    return Cast<TString>(V_[TStringBuf("faultString")]);
}
