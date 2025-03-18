#pragma once

//#include <iostream>
//#include <sstream>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <library/cpp/regex/pcre/regexp.h>

#include "sanit_config.h"
#include "sanit_policy.h"

namespace NCssConfig {
    class TConfigLexer;

    class TConfigDriver {
        //SanitPolicy * policy;

        //friend int yylex(YYSTYPE *locp, void * driver);

        TStringStream ParseErrorStream;
#if 0 // to do
    TRegExMatch RegexCheck;
#endif
    public:
        /// stream name (file or input stream) used for error messages.
        TString Streamname;

        TConfig Config;

#ifdef WITH_DEBUG_OUT
    private:
        bool TraceScanning;
        bool TraceParsing;

    public:
        //void SetDebugOut(bool val) { debug_out = val; }
        //bool GetDebugOut() const { return debug_out; }

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

    public:
        /// construct a new parser driver context
        TConfigDriver();

        /** Invoke the scanner and parser for a stream.
     * @param in        input stream
     * @param sname     stream name for error messages
     * @return          true if successfully parsed
     */
        bool ParseStream(IInputStream& in,
                         const TString& sname = "stream input");

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

        // To demonstrate pure handling of parse errors, instead of
        // simply dumping them on the standard error output, we will pass
        // them to the driver using the following two member functions.

        /** Error handling with associated line number. This can be modified to
     * output the error e.g. to a dialog box. */
        void error(const char* msg);

        /** General error handling. This can be modified to output the error
     * e.g. to a dialog box. */
        void error(const TString& m);

        /** Pointer to the current lexer instance, this is used to connect the
     * parser to the scanner. It is used in the yylex macro. */
        TConfigLexer* Lexer;

        const TConfig& GetConfig() const {
            return Config;
        }

        TString GetParseError() const {
            return ParseErrorStream.Str();
        }
    };

}
