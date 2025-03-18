#include "lib/options.h"
#include "lib/printer.h"
#include "lib/prepare_request.h"
#include "process_print.h"
#include "printwzrd_task.h"

#include <kernel/gazetteer/gazetteer.h>

#include <search/fields/fields.h>

#include <search/wizard/config/config.h>
#include <search/wizard/core/wizglue.h>
#include <search/wizard/face/reqwizardface.h>

#include <library/cpp/threading/serial_postprocess_queue/serial_postprocess_queue.h>
#include <util/generic/algorithm.h>
#include <utility>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/file.h>
#include <util/folder/dirut.h>

namespace NPrintWzrd {

    //RulesResults
    typedef TVector<TStringBuf> TPropertiesValues;
    typedef TMap<TStringBuf, TPropertiesValues> TPropertiesRecord;
    typedef TMap<TStringBuf, TPropertiesRecord> TRulesRecord;

    TAutoPtr<TWizardResults> ProcessWizardRequest(
        const IRequestWizard* const wizard, const TString& req,
        const TPrintwzrdOptions& options, TSelfFlushLogFramePtr frame, IWizardResultsPrinter* printer)
    {
        if (!wizard) {
            ythrow yexception() << "Error in ProcessWizardRequest: Wizard pointer is NULL" << Endl;
        }

        TCgiParameters cgiParams;
        TStringBuf rulesToPrint;
        ParseRequest(req, options, cgiParams, rulesToPrint);

        if (!rulesToPrint && options.RulesToPrint)
            rulesToPrint = options.RulesToPrint;

        TSearchFields fields(cgiParams);
        wizard->Process(&fields, frame);
        TAutoPtr<TWizardResults> wizardResults(TWizardResultsCgiPacker::Deserialize(&fields.CgiParam).Release());

        if (printer) {
            printer->SetPrintRules(rulesToPrint);
            printer->Print(wizardResults->SourceRequests, wizardResults->RulesResults, fields.CgiParam, req);
        }

        return wizardResults;
    }

    void ProcessSingleTest(const IRequestWizard* const wizard, const TPrintwzrdOptions& options, TEventLogPtr eventLog) {
        TAutoPtr<IInputStream> inputStream = OpenInput(options.InputFileName);
        TAutoPtr<IOutputStream> outputStream = OpenOutput(options.OutputFileName);

        TThreadPool processQueue;
        TSerialPostProcessQueue queue(&processQueue);
        queue.Start(options.ThreadCount, options.ThreadCount);

        TString req;
        while (inputStream->ReadLine(req)) {
            TStringStream message;
            try {
                if (!PrepareRequest(req, options.TabbedInput, message))
                    continue;

            } catch (const yexception& e) {
                Cerr << "printwzrd: PrepareRequest failed: " << e.what() << Endl;
                continue;
            }

            TSelfFlushLogFramePtr frame = MakeIntrusive<TSelfFlushLogFrame>(*eventLog);
            TAutoPtr<TPrintwzrdTask> task(new TPrintwzrdTask(wizard, req, options, *outputStream, frame));
            task->SetMessage(message.Str());
            queue.SafeAdd(task.Release());
        }

        queue.Stop();
        if (options.PrintLemmaCount)
            TPrintwzrdTask::PrintTotals(*outputStream);
    }

    TVector<TString> SourceWithSpecialRequest(const ISourceRequests* const requests) {
        if (!requests) {
            ythrow yexception() << "Error in SourceWithSpecialRequest: requests pointer is NULL";
        }

        TVector<TString> specials;
        unsigned specialsCount = requests->SourceWithSpecialRequestCount();
        specials.reserve(specialsCount);

        for (unsigned index = 0; index < specialsCount; ++index) {
            specials.push_back(requests->SourceWithSpecialRequestName(index));
        }

        return specials;
    }

    // Checks for elements of requests2 in requests1 (TRemoteWizard's results in TReqWizard's results)
    size_t CompareSourceRequests(const ISourceRequests* const requests1,
                const ISourceRequests* const requests2)
    {
        if (!requests1 || !requests2) {
            ythrow yexception() << "Error in CompareSourceRequests: requests pointer is NULL";
        }

        size_t mismatches = 0;
        TVector<TString> sourceNames1 = requests1->SourceNames();
        TVector<TString> sourceNames2 = requests2->SourceNames();

        TString req1, req2;

        for (TVector<TString>::const_iterator cit = sourceNames2.begin(); cit != sourceNames2.end(); ++cit) {
            if (Find(sourceNames1.begin(), sourceNames1.end(), *cit) == sourceNames1.end()) {
                ++mismatches;
                continue;
            }
            requests1->GetRequestForSources(*cit, req1);
            requests2->GetRequestForSources(*cit, req2);

            if (req1 != req2) {
                ++mismatches;
            }
        }

        TVector<TString> specials1 = SourceWithSpecialRequest(requests1);
        TVector<TString> specials2 = SourceWithSpecialRequest(requests2);

        for (TVector<TString>::const_iterator cit = specials2.begin(); cit != specials2.end(); ++cit) {
            if (Find(specials1.begin(), specials1.end(), *cit) == specials1.end()) {
                ++mismatches;
            }
        }

        return mismatches;
    }

    TRulesRecord ExtractRulesResults(const IRulesResults* const results) {
        if (!results) {
            ythrow yexception() << "Error in ExtractRulesResults: results pointer is NULL";
        }

        TRulesRecord resultsMap;
        const unsigned rulesNames = results->GetRuleSize();

        for (unsigned namePos = 0; namePos < rulesNames; ++namePos) {
            const TStringBuf ruleName = results->GetRuleName(namePos);
            unsigned totalProperties = results->GetRulePropertySize(ruleName);
            TPropertiesRecord properties;

            for (unsigned property = 0; property < totalProperties; ++property) {
                const TStringBuf propertyName = results->GetPropertyName(ruleName, property);
                const unsigned propertyCount = results->GetPropertyValueSize(ruleName, propertyName);
                TPropertiesValues propertyValues;
                propertyValues.reserve(propertyCount);

                for (unsigned index = 0; index < propertyCount; ++index) {
                    propertyValues.push_back(results->GetProperty(ruleName, propertyName, index));
                }
                properties.insert(std::pair<TStringBuf, TPropertiesValues>(propertyName, propertyValues));
            }

            resultsMap.insert(std::pair<TStringBuf, TPropertiesRecord >(ruleName, properties));
        }

        return resultsMap;
    }

    // Checks for elements of results2 in results1 (TRemoteWizard's results in TReqWizard's results)
    size_t CompareRulesResults(const IRulesResults* const results1,
                const IRulesResults* const results2)
    {
        if (!results1 || !results2) {
            ythrow yexception() << "Error in CompareRulesResults: results pointer is NULL";
        }

        size_t mismatches = 0;
        const TRulesRecord rules1 = ExtractRulesResults(results1);
        const TRulesRecord rules2 = ExtractRulesResults(results2);

        for (TRulesRecord::const_iterator rule = rules2.begin(); rule != rules2.end(); ++rule) {
            if (rule->first == "Gzt") {
                continue;
            }

            TRulesRecord::const_iterator rule1 = rules1.find(rule->first);

            if (rule1 == rules1.end()) {
                ++mismatches;
                continue;
            }

            for (TPropertiesRecord::const_iterator property = rule->second.begin();
                property != rule->second.end(); ++property)
            {
                TPropertiesRecord::const_iterator property1 =
                    rule1->second.find(property->first);
                if (property1 == rule1->second.end()) {
                    ++mismatches;
                    continue;
                }

                const size_t properties2 = property->second.size();
                const size_t properties1 = property1->second.size();

                const size_t values = properties2 < properties1 ? properties2 : properties1;

                for (size_t index = 0; index < values; ++index) {
                    if (property->second.at(index) == property1->second.at(index)) {
                        continue;
                    }
                    ++mismatches;
                }

                if (properties2 > properties1) {
                    mismatches += properties2 - properties1;
                }
            }
        }

        return mismatches;
    }

    // Checks for elements of results2 in results1 (TRemoteWizard's results in TReqWizard's results)
    size_t CompareResults(const TWizardResults* wizardResults1,
        const TWizardResults* wizardResults2)

    {
        if (!wizardResults1 || !wizardResults2) {
            ythrow yexception() << "Error in CompareResults: results pointer is NULL";
        }

        size_t mismatches = CompareSourceRequests(wizardResults1->SourceRequests.Get(),
            wizardResults2->SourceRequests.Get());
        mismatches += CompareRulesResults(wizardResults1->RulesResults.Get(),
            wizardResults2->RulesResults.Get());

        return mismatches;
    }

    void ProcessCompareTest(const IRequestWizard* const wizard1,
        const IRequestWizard* const wizard2,
        const TPrintwzrdOptions& options, TEventLogPtr eventLog)
    {
        if (!wizard1 || !wizard2) {
            ythrow yexception() << "Error in ProcesscompareTest: wizard pointer is NULL";
        }

        TString req;
        TAutoPtr<TWizardResults> wizardResults1;
        TAutoPtr<TWizardResults> wizardResults2;
        TAutoPtr<IInputStream> inputStream = OpenInput(options.InputFileName);
        TAutoPtr<IOutputStream> outputStream = OpenOutput(options.OutputFileName);
        TUnbufferedFileOutput file1(options.LocalWizardOutFile);
        TUnbufferedFileOutput file2(options.RemoteWizardOutFile);
        yexception ex1, ex2;
        while (inputStream->ReadLine(req)) {
            try {
                if (!PrepareRequest(req, options.TabbedInput, *outputStream)) {
                    continue;
                }
            } catch (yexception& ) {
                continue;
            }
            bool failed1 = false;
            bool failed2 = false;
            try {
                TSelfFlushLogFramePtr frame = MakeIntrusive<TSelfFlushLogFrame>(*eventLog);
                wizardResults1 = ProcessWizardRequest(wizard1, req, options, frame);
            } catch (const yexception& e) {
                ex1 = e;
                failed1 = true;
            }
            try {
                TSelfFlushLogFramePtr frame = MakeIntrusive<TSelfFlushLogFrame>(*eventLog);
                wizardResults2 = ProcessWizardRequest(wizard2, req, options, frame);
            } catch (const yexception& e) {
                ex2 = e;
                failed2 = true;
            }

            if (failed1 && failed2) {
                continue;
            } else if (failed1) {
                PrintFailInfo(file1, req);

                TAutoPtr<IWizardResultsPrinter> printer(GetResultsPrinter(options, file2));
                TSearchFields fields(MakeCgiParams(req, options));
                printer->Print(wizardResults2->SourceRequests, wizardResults2->RulesResults, fields.CgiParam, req);

                continue;
            } else if (failed2) {
                PrintFailInfo(file2, req);

                TAutoPtr<IWizardResultsPrinter> printer(GetResultsPrinter(options, file1));
                TSearchFields fields(MakeCgiParams(req, options));
                printer->Print(wizardResults1->SourceRequests, wizardResults1->RulesResults, fields.CgiParam, req);

                continue;
            }

            size_t mismatches = CompareResults(wizardResults1.Get(), wizardResults2.Get());

            if (mismatches) {
                TAutoPtr<IWizardResultsPrinter> printer(GetResultsPrinter(options, file1));
                TSearchFields fields(MakeCgiParams(req, options));
                printer->Print(wizardResults1->SourceRequests, wizardResults1->RulesResults, fields.CgiParam, req);

                printer = GetResultsPrinter(options, file2);
                printer->Print(wizardResults2->SourceRequests, wizardResults2->RulesResults, fields.CgiParam, req);

                *outputStream << "Request: [" << req << "]" << Endl;
                *outputStream << "Total mismatches: " << mismatches << Endl << Endl;
            }
        }
    }
}
