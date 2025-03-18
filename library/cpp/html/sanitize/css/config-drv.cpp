#include <util/stream/file.h>
#include <util/stream/str.h>

#include "config-drv.h"
#include "config-lexer.h"

namespace NCssConfig {
    class TConfigDriver;
}

extern bool yydebug;
extern int yyparse_config(NCssConfig::TConfigDriver& driver); // see #define yyparse in config_y.y

namespace NCssConfig {
    TConfigDriver::TConfigDriver()
#if 0 // to do
    : RegexCheck("[\\w-_]+")
#endif
#ifdef WITH_DEBUG_OUT
        : TraceScanning(false)
        , TraceParsing(false)
#endif
    {
    }

    bool TConfigDriver::ParseStream(IInputStream& in, const TString& sname) {
        Streamname = sname;

        TConfigLexer lex(in);
#ifdef WITH_DEBUG_OUT
        lex.SetDebug(TraceScanning);
        if (TraceParsing)
            SetEnv("YYDEBUG", "1");
#endif
        this->Lexer = &lex;

        return yyparse_config(*this) == 0;
    }

    bool TConfigDriver::ParseFile(const TString& filename) {
        //std::ifstream in(filename.c_str());
        try {
            TIFStream in(filename);
            //if (!in.good())
            //{
            //ParseErrorStream << "Could not open file: " << filename;
            //return false;
            //}

            return ParseStream(in, filename);
        } catch (...) {
            ParseErrorStream << "Could not open file: " << filename;
            return false;
        }
    }

    bool TConfigDriver::ParseString(const TString& input, const TString& sname) {
        //std::istringstream iss(input);
        TStringInput iss(input);
        return ParseStream(iss, sname);
    }

    void TConfigDriver::error(const char* msg) {
        ParseErrorStream << Streamname << ':' << Lexer->lineno() << ' ' << msg << '\n';
    }

    void TConfigDriver::error(const TString& m) {
        ParseErrorStream << Streamname << ':' << Lexer->lineno() << ' ' << m << '\n';
    }

}
