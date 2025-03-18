#pragma once

#include "options.h"

#include <kernel/qtree/richrequest/richnode_fwd.h>
#include <library/cpp/charset/recyr.hh>
#include <search/wizard/face/wizinfo.h>

#include <util/generic/set.h>
#include <util/stream/output.h>
#include <library/cpp/cgiparam/cgiparam.h>

typedef THolder<ISourceRequests> TSourceResPtr;
typedef THolder<IRulesResults> TWizardResPtr;

struct TRequestsInfo {
    size_t LemmaCount;
    size_t WordCount;
    size_t RequestCount;
    TRequestsInfo();
    void Update(const TRichTreePtr& blob);
    void Update(const TRequestsInfo& other);
    void Print(IOutputStream& out) const;
};

class IWizardResultsPrinter {
public:
    virtual void Print(const TSourceResPtr& sr, const TWizardResPtr& wr, const TCgiParameters& cgi, const TString& request) = 0;
    virtual const TRequestsInfo* GetInfo() const { return nullptr; }
    virtual void SetPrintRules(TStringBuf rulesToPrint) { Y_UNUSED(rulesToPrint);}

    virtual ~IWizardResultsPrinter() {}
};

class TPrintwzrdPrinter : public IWizardResultsPrinter {
private:
    IOutputStream& Out;
    TPrintwzrdOptions Options;
    TRequestsInfo RequestsInfo;

    TSet<TString> RulesToPrint;
private:
    void PrintRichTree(const NSearchQuery::TRequest* tree);
    void PrintRequestForSources(const TSourceResPtr& sr);
    void PrintProperties(const IRulesResults* rrr);
    void PrintSuccessfulRules(const IRulesResults* rrr);
    void PrintCgiParamRelev(const TCgiParameters& cgiParam);
    void PrintRequest(const TRichTreePtr& tree);
    void PrintQtree(const TBinaryRichTree& blob);

public:
    TPrintwzrdPrinter(IOutputStream& out, const TPrintwzrdOptions& options)
        : Out(out)
        , Options(options)
    {}
    void Print(const TSourceResPtr& sr, const TWizardResPtr& wr, const TCgiParameters& cgi, const TString& request) override;
    const TRequestsInfo* GetInfo() const override;

    void SetPrintRules(TStringBuf rulesToPrint) override;
};

class TUpperSearchPrinter : public IWizardResultsPrinter {
public:
    TUpperSearchPrinter(IOutputStream& out)
        : Out(out)
    {
    }

    void Print(const TSourceResPtr& sr, const TWizardResPtr& wr, const TCgiParameters& cgi, const TString& request) override {
        Y_UNUSED(sr);
        Y_UNUSED(wr);
        Y_UNUSED(request);
        const TString req = cgi.Get("user_request", 0);
        const TString lr = cgi.Get("lr", 0);
        TCgiParameters cgiParam(cgi.Print().data());
        cgiParam.EraseAll("dbgwzr");
        Out << req << "\t" << RecodeToYandex(CODES_UTF8, req) << "\t";
        Out << cgiParam.Print();
        Out << "\t" << lr;
        Out << Endl;
    }
private:
    IOutputStream& Out;
};

TAutoPtr<IWizardResultsPrinter> GetResultsPrinter(const TPrintwzrdOptions& options, IOutputStream& out);

