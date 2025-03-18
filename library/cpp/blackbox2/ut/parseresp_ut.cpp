#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/blackbox2/blackbox2.h>
#include <library/cpp/blackbox2/src/utils.h>

#include <util/folder/path.h>
#include <util/stream/file.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace NBlackbox2;

Y_UNIT_TEST_SUITE(CheckResp) {
    enum RespType {
        tResp,
        tBulk,
        tLogin,
        tSession,
        tMultiSession,
    };

    const TString strResp("resp");
    const TString strBulk("bulk");
    const TString strLogin("login");
    const TString strSession("session");
    const TString strMultiSession("multisession");

    RespType getType(const TString& filename) {
        if (filename.StartsWith("log"))
            return tLogin;
        else if (filename.StartsWith("sess"))
            return tSession;
        else if (filename.StartsWith("multisess"))
            return tMultiSession;
        else if (filename.StartsWith("oauth"))
            return tSession;
        else if (filename.StartsWith("resp"))
            return tResp;
        else if (filename.StartsWith("bulk"))
            return tBulk;

        return tResp;
    }

    TString foo(RespType type, const TString& filename) {
        std::stringstream out;
        TString response;

        {
            TFileInput file(filename.c_str());
            response = file.ReadAll();
        }

        try {
            switch (type) {
                case tLogin: {
                    THolder<TLoginResp> pR = LoginResponse(response);
                    out << pR.Get();
                    break;
                }
                case tSession: {
                    THolder<TSessionResp> pR = SessionIDResponse(response);
                    out << pR.Get();
                    break;
                }
                case tMultiSession: {
                    THolder<TMultiSessionResp> pR = SessionIDResponseMulti(response);
                    out << pR.Get();
                    break;
                }
                case tBulk: {
                    THolder<TBulkResponse> pR = InfoResponseBulk(response);
                    out << pR.Get();
                    break;
                }
                case tResp:
                default: {
                    THolder<TResponse> pR = InfoResponse(response);
                    out << pR.Get();
                    break;
                }
            };
        } catch (TFatalError& err) {
            return TStringBuilder() << "Got a fatal blackbox exception:"
                                    << " code=" << err.Code() << ", message='" << err.what() << "'" << Endl;
        } catch (TTempError& err) {
            return TStringBuilder() << "Got a temporary blackbox exception:"
                                    << " code=" << err.Code() << ", message='" << err.what() << "'" << Endl;
        } catch (const std::exception& e) {
            return TStringBuilder() << "Error: Got unknown exception!: " << e.what() << Endl;
        }

        return TString(out.str());
    }

    Y_UNIT_TEST(req) {
        TString dir = ArcadiaSourceRoot() + "/library/cpp/blackbox2/ut/response/";
        TFsPath path(dir);

        TVector<TString> childes;
        path.ListNames(childes);

        for (const TString& f : childes) {
            if (f.EndsWith(".out")) {
                continue;
            }

            TString file = dir + f;

            TStringBuf b(file);
            b.ChopSuffix(TFsPath(file).GetExtension());

            TFileInput out(TString(b) + "out");

            UNIT_ASSERT_VALUES_EQUAL_C(out.ReadAll(), foo(getType(f), TString(b) + "xml"), file);
        }
    }
}
