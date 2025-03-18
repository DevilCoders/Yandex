#pragma once

#include <library/cpp/http/io/stream.h>

#include <util/generic/strbuf.h>
#include <util/stream/mem.h>
#include <util/string/join.h>

class TAmmoPrinter {
public:
    virtual ~TAmmoPrinter() = default;

    virtual void Print(TStringBuf request) = 0;

    void Print(const TVector<TString>& requests) {
        Print(JoinSeq(TStringBuf(), requests));
    }
};

class TTankAmmoPrinter: public TAmmoPrinter {
public:
    void Print(TStringBuf request) override {
        Cout << request.size() << '\n';
        Cout << request << '\n';
    }
};

class TDolbilkaAmmoPrinter: public TAmmoPrinter {
public:
    void Print(TStringBuf request) override {
        // For some reason, dolbilka splits HTTP chunks by itself.
        // So we should remove all encoding information not to mess up.
        TMemoryInput memoryInput(request);
        THttpInput input(&memoryInput);
        TStringStream ss;
        ss << input.FirstLine();
        ss << "\r\n";
        input.Headers().OutTo(&ss);
        ss << "\r\n";
        try {
            TransferData(&input, &ss);
        } catch (const yexception& ex) {
            Cerr << "Exception while parsing HTTP request: " << ex.what() << Endl;
            return;
        }
        Cout << ss.Size() << '\n';
        Cout << ss.Str() << "\r\n";
    }
};

THolder<TAmmoPrinter> CreateAmmoPrinter(bool dolbilkaFormat) {
    return dolbilkaFormat ? THolder<TAmmoPrinter>{new TDolbilkaAmmoPrinter}
                          : THolder<TAmmoPrinter>{new TTankAmmoPrinter};
}
