#pragma once

#include "client.h"
#include "simpleaddrresolver.h"

#include <library/cpp/deprecated/split/split_iterator.h>

namespace NCommunism {

struct TParserError: yexception {};

template <class T, class TAddressResolver>
void ParseDefinition(const TStringBuf& string, T& resultClaims, THolder<ISockAddr>& resultSolver, TAddressResolver& resolver) {
    TStringBuf claims;
    TStringBuf solver;

    if (string.empty()) {
        return;
    } else {
        TStringBuf token(string);

        const size_t atsign = token.rfind('@');

        if (atsign != TStringBuf::npos) {
            solver = TStringBuf(token, atsign + 1, TStringBuf::npos);
            token.Trunc(atsign);
        }

        claims = token;
    }

    if (!claims.empty()) {
        TSplitDelimiters delims(",");
        TDelimitersStrictSplit split(claims.data(), claims.size(), delims);

        TDelimitersStrictSplit::TIterator it = split.Iterator();

        while (!it.Eof()) {
            const TString& token = it.NextString();
            const size_t percent = token.rfind('%');
            const size_t colon = token.rfind(':');

            const TStringBuf key(token, 0, Min(percent, colon));
            TStringBuf val;
            TStringBuf name;

            if (key.empty()) {
                throw TParserError() << "\"" << string << "\": \"" << token << "\": empty key";
            }

            if (percent != TStringBuf::npos) {
                name = TStringBuf(token, percent + 1, TStringBuf::npos);
            }

            if (colon != TStringBuf::npos) {
                val = TStringBuf(token, colon + 1, percent - colon - 1);
            }

            if (!val.empty()) {
                const size_t slash = val.rfind('/');

                if (slash != TStringBuf::npos) {
                    double numerator = 0.0;
                    double denominator = 0.0;

                    try {
                        numerator = FromString<double>(TStringBuf(val, 0, slash));
                        denominator = FromString<double>(TStringBuf(val, slash + 1, TStringBuf::npos));
                    } catch (const TFromStringException& e) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": " << e.what();
                    }

                    if (numerator < 0.0) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": numerator must be >= 0";
                    }

                    if (denominator <= 0.0) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": denominator must be > 0";
                    }

                    if (denominator < numerator) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": relative value cannot be greater than 1.0";
                    }

                    if (name.empty()) {
                        resultClaims.AddClaim(TString(key), numerator / denominator);
                    } else {
                        resultClaims.AddSharedClaim(TString(key), numerator / denominator, TString(name));
                    }
                } else {
                    double value = 0.0;

                    try {
                        value = FromString<double>(val);
                    } catch (const TFromStringException& e) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": " << e.what();
                    }

                    if (value < 0.0) {
                        throw TParserError() << "\"" << string << "\": \"" << token << "\": value must be > 0";
                    }

                    if (name.empty()) {
                        resultClaims.AddClaim(TString(key), -value);
                    } else {
                        resultClaims.AddSharedClaim(TString(key), -value, TString(name));
                    }
                }
            } else {
                if (name.empty()) {
                    resultClaims.AddClaim(TString(key), 1.0);
                } else {
                    resultClaims.AddSharedClaim(TString(key), 1.0, TString(name));
                }
            }
        }
    }

    if (!solver.empty()) {
        if (solver.find('/') == TStringBuf::npos) {
            TIpPort port;

            const size_t colon = solver.find(':');

            if (colon != TStringBuf::npos) try {
                port = FromString<TIpPort>(TStringBuf(solver, colon + 1, TStringBuf::npos));
            } catch (const TFromStringException& e) {
                throw TParserError() << "\"" << string << "\": invalid solver port \"" << TStringBuf(solver, colon + 1, TStringBuf::npos) << "\": " << e.what();
            } else {
                port = NCommunism::GetDefaultPort();
            }

            TString ipOrHost(TStringBuf(solver, 0, colon));

            THolder<TNetworkAddress> solverAddrPtr;
            try {
                solverAddrPtr.Reset(new TNetworkAddress(resolver(ipOrHost, port)));
            } catch (const TNetworkResolutionError& e) {
                throw TParserError() << "\"" << string
                                     << "\": cannot resolve host \""
                                     << TStringBuf(solver, 0, colon)
                                     << "\": " << e.what();
            }

            THolder<ISockAddr> sockAddr;
            for (TNetworkAddress::TIterator it = solverAddrPtr->Begin(); it != solverAddrPtr->End(); ++it) {
                if (it->ai_family == AF_INET) {
                    sockAddr.Reset(new TSockAddrInet());
                    memcpy(sockAddr->SockAddr(), it->ai_addr, it->ai_addrlen);
                    break;
                } else if (it->ai_family == AF_INET6) {
                    sockAddr.Reset(new TSockAddrInet6());
                    memcpy(sockAddr->SockAddr(), it->ai_addr, it->ai_addrlen);
                    break;
                }
            }
            if (!sockAddr) {
                throw TParserError() << "\"" << string
                                     << "\": no appropriate address family \""
                                     << TStringBuf(solver, 0, colon);
            }
            resultSolver.Reset(sockAddr.Release());
        } else {
            resultSolver.Reset(new TSockAddrLocal(TString(solver).data()));
        }
    }
}

template <class T>
void ParseDefinition(const TStringBuf& string, T& resultClaims, THolder<ISockAddr>& resultSolver) {
    TSimpleAddressResolver resolver;
    ParseDefinition<T, TSimpleAddressResolver>(string, resultClaims, resultSolver, resolver);
}

}
