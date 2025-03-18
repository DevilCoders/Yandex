#pragma once

//#include <iostream>
//#include <sstream>
#include <library/cpp/html/sanitize/common/filter_entities/filter_entities.h>

#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/generic/ptr.h>

//#include "css2-lexer.h"
#include "str_tools.h"
#include "sanit_policy.h"
#include "gc.h"

//using std::ostream;
//using std::istream;
//using std::istringstream;
//using std::ostringstream;

class IUrlProcessor;

//class IFilterEntities;

namespace NCssSanit {
    extern int yyparse(void*);

    using namespace NCssConfig;

    class TCSS2Lexer;

    class TCSS2Driver {
        const char* TagName;
        IUrlProcessor* UrlProcessor;
        IFilterEntities* FilterEntities;
        IOutputStream* OutStream;
        TSanitPolicy* Policy;
        bool ProcessAllUrls;

        TStringStream ParseErrorStream;
        int ErrorCount;
        //TString CurPropertyName;
        TGc Gc;
#ifdef WITH_DEBUG_OUT
        bool TraceScanning;
        bool TraceParsing;
        bool CanDebugOut;
#endif
    private:
        TString CorrectSelector(const TString& selector);

        void DoOut(IOutputStream&) {
        }

        template <class T, class... R>
        void DoOut(IOutputStream& out, const T& t, const R&... r) {
            out << t;
            DoOut(out, r...);
        }

    public:
        template <class T>
        TNodePtr<T>* New() {
            return Gc.New<T>();
        }

        template <class... R>
        void Out(const R&... r) {
            if (OutStream) {
                DoOut(*OutStream, r...);
            }
        }

#ifdef WITH_DEBUG_OUT
        template <class... R>
        void OutDebug(const R&... r) {
            if (CanDebugOut) {
                DoOut(Cerr, r...);
            }
        }
#endif

        TString HandleImportUrl(const TString& url);

        //void OutExprList(const TContextList & list);
        TString ExprListToString(const TString& prop_name, const TContextList& context);
        TString ExprToString(const TString& prop_name, const TContext& context);

        TString PropertyToString(const TString& prop_name, const TContextList& list);

        bool PassSelector(const TString& selector_name);
        bool DenySelector(const TString& selector_name);
        TString MakeSelectorsAll(const TStrokaList& list);
        TString MakeSelectorsDefault(const TStrokaList& list);

    public:
        /// stream name (file or input stream) used for error messages.
        TString StreamName;

    public:
        TCSS2Driver();

#ifdef WITH_DEBUG_OUT
        void SetDebugOut(bool val) {
            CanDebugOut = val;
        }
        bool GetDebugOut() const {
            return CanDebugOut;
        }

        void SetTraceScanning(bool value) {
            TraceScanning = value;
        }
        bool GetTraceScanning() const {
            return TraceScanning;
        }

        void SetTraceParsing(bool value) {
            TraceParsing = value;
        }
        bool GetTraceParsing() const {
            return TraceParsing;
        }
#endif

        void SetOutStream(IOutputStream& ostr);

        void SetPolicy(TSanitPolicy* aPolicy) {
            Policy = aPolicy;
        }

        void SetTagName(const char* tagName) {
            TagName = tagName;
        }

        void SetUrlProcessor(IUrlProcessor* urlProcessor) {
            UrlProcessor = urlProcessor;
        }

        void SetProcessAllUrls(bool processAllUrls) {
            ProcessAllUrls = processAllUrls;
        }

        void SetFilterEntities(IFilterEntities* filterEntities) {
            FilterEntities = filterEntities;
        }

        IFilterEntities* GetFilterEntities() {
            return FilterEntities;
        }

        const TSanitPolicy& GetPolicy() const;

        /** Invoke the scanner and parser for a stream.
     * @param in        input stream
     * @param sname     stream name for error messages
     * @return          true if successfully parsed
     */
        bool ParseStream(IInputStream& in, const TString& sname = "stream input", bool inline_css = false);

        /** Invoke the scanner and parser on an input string.
     * @param input     input string
     * @param sname     stream name for error messages
     * @return          true if successfully parsed
     */
        bool ParseString(const TString& input,
                         const TString& sname = "string stream");

        /** Invoke the scanner and parser on a file. Use parse_stream with a
     * std::ifstream if detection of file reading errors is required.
     * @param filename  input file name
     * @return          true if successfully parsed
     */
        bool ParseFile(const TString& filename);

        /** Invoke the scanner and parser on an input string for inline css style.
     * @param input     input string
     * @param sname     stream name for error messages
     * @return          true if successfully parsed
     */
        bool ParseInlineStyle(const TString& input,
                              const TString& sname = "string stream");

        // To demonstrate pure handling of parse errors, instead of
        // simply dumping them on the standard error output, we will pass
        // them to the driver using the following two member functions.

        /** Error handling with associated line number. This can be modified to
     * output the error e.g. to a dialog box. */
        void Error(const char* msg);

        /** General error handling. This can be modified to output the error
     * e.g. to a dialog box. */
        void Error(const TString& m);

        /** Pointer to the current lexer instance, this is used to connect the
     * parser to the scanner. It is used in the yylex macro. */
        TCSS2Lexer* Lexer;

        int GetErrorCount() const {
            return ErrorCount;
        }
        TString GetParseError() const {
            return ParseErrorStream.Str();
        }
    };

    inline bool TCSS2Driver::PassSelector(const TString& selector_name) {
        bool result = Policy ? Policy->PassSelector(selector_name) : false;
#ifdef WITH_DEBUG_OUT
        if (result)
            OutDebug("<<Selector passed: ", selector_name, ">>\n");
        else
            OutDebug("<<Selector denied: ", selector_name, ">>\n");
#endif
        return result;
    }

    inline bool TCSS2Driver::DenySelector(const TString& selector_name) {
        bool result = Policy ? Policy->DenySelector(selector_name) : true;
#ifdef WITH_DEBUG_OUT
        if (result)
            OutDebug("<<Selector denied: ", selector_name, ">>\n");
        else
            OutDebug("<<Selector passed: ", selector_name, ">>\n");
#endif
        return result;
    }

}
