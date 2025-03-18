#ifndef _MSC_VER
#include <paths.h>
#endif

#include <util/string/util.h>
#include <util/string/split.h>
#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/folder/dirut.h>
#include <util/datetime/cputimer.h>

#include <util/generic/bt_exception.h>

#include <tools/clustermaster/common/target_graph_parser.h>

//
// Parser specification
//
%%{
    machine ConfigScanner;

    ws = [\t ];

    dquoted = '"' [^"]* '"';
    squoted = '\'' [^']* '\'';
    identifier = [a-zA-Z0-9!_\-] ( [^\n\r\t,?= ]* [a-zA-Z0-9_\-] )?;
    option = [a-zA-Z0-9_\-_] [^\n\r\t,= ]* '=' ( '"' ( [^"] | '\\"' )* '"' | ( [^\n\r\t \\] | '\\' [\t \\] )* );
    condition = '?' '!'? [A-Za-z_][A-Za-z0-9_]* ( [~=] [a-zA-Z0-9!_\-\$] ( [^\n\r\t,?= ]* [a-zA-Z0-9_\-] )? )?;
    condepend = condition ('|' condition)*;
    #vallist = (identifier ('..' identifier)?) (',' identifier ('..' identifier)?)*;

    dependMapping = '[' ^']'* ']';

    scenario := |*
        # Ignore comments
        '#' [^\n]*;

        # Ignore backslashed line ends:
        '\\\n' {
            line++;
        };
        # Count lines
        '\n' {
            r.Stacks.push_back(token_stack);
            token_stack.Clear();

            line++;
            linestart = tokend;
        };

        # Done
        '}' {
            r.Stacks.push_back(token_stack);
            token_stack.Clear();

            fret;
        };

        ws;

        # Recognize tokens
        '^' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_DEPENDONPREV)); };
        '=' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_TARGETTYPEDEF)); };
        ':' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_EXPLDEPENDS)); };
        '%' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_GROUPDEPEND)); };
        '&' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_SEMAPHORE)); };
        '|' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_NONRECURSIVE)); };
        ',' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_LISTDIVIDER)); };
        dependMapping { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_DEPENDMAPPING)); };

        '?=' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_DEFVAR)); };
        ':=' { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_STRONGVAR)); };

        option { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_OPTION)); };
        condepend { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_CONDEPEND)); };
        squoted { token_stack.AddToken(TToken(tokstart+1, tokend-1, line, p-linestart, TT_IDENTIFIER)); };
        dquoted { token_stack.AddToken(TToken(tokstart+1, tokend-1, line, p-linestart, TT_IDENTIFIER)); };
        identifier { token_stack.AddToken(TToken(tokstart, tokend, line, p-linestart, TT_IDENTIFIER)); };

        # Literals
        '\'' [^'\n] '\'' { token_stack.AddToken(TToken(tokstart+1, tokend-1, line, p-linestart, TT_IDENTIFIER)); };
        '"' [^"\n] '"' { token_stack.AddToken(TToken(tokstart+1, tokend-1, line, p-linestart, TT_IDENTIFIER)); };

        any {
            ythrow yexception()
                << "unexpected symbols \"" << TString(tokstart, tokend - tokstart)
                << "\" at " << line << ":" << (tokstart - linestart + 1)
                << "\nline: \"" << TString(linestart, p - linestart + 1) << "\"...\n";
        };
    *|;

    main := |*
        '#!' [^ \t\n]+ (ws+ [^ \t\n]+)* {
            if (line == 1 && tokstart - linestart == 0)
                newShebang = TString(tokstart + 2, tokend - tokstart - 2);
        };
        '\n' { line++; linestart = tokend; };
        '_scenario' ws* '(' ws* ')' ws* '{' { fcall scenario; };
        any;
    *|;

}%%

// Anonymous namespace for ragel constants
namespace {
    %% write data;
};

TTargetGraphParsed::TIntermediate TTargetGraphParsed::TIntermediate::Parse(TStringBuf buf) {
    TTokenStack token_stack;

    TTargetGraphParsed::TIntermediate r;

    const char* p = buf.data();
    const char* pe = buf.data() + buf.length();

    const char* tokstart;
    const char* tokend;

    int act, cs;

    int top;
    int stack[2];

    // data
    int line = 1;
    const char* linestart = p;

#ifdef _MSC_VER
    TString newShebang = "cmd"; // maybe that will work but definitely compiles
#else
    TString newShebang = _PATH_BSHELL;
#endif

    %% write init;
    %% write exec;

    if (cs == ConfigScanner_error)
        ythrow TWithBackTrace<yexception>() << "parse error" << " at " << line << ":" << p - linestart + 1;

    r.Shebang = newShebang;

    return r;
}

%%{
    machine DependMapping;

    digitValue = digit @{ digitValue = fc - '0'; };

    singleMapping =
        digitValue @{ depLevelId = digitValue; }
        '->'
        digitValue @{
            myLevelId = digitValue;

            r.emplace_back();
            r.back().DepLevelId = depLevelId;
            r.back().MyLevelId = myLevelId;
        }
        ;

    main := '[' (singleMapping (',' singleMapping)* ','?)? ']';
}%%

namespace {
    %% write data;
}

TVector<TTargetParsed::TParamMapping> TTargetParsed::ParseParamMappings(TStringBuf buf) {
    const char* p = buf.data();
    const char* pe = buf.data() + buf.length();

    TVector<TTargetParsed::TParamMapping> r;

    int digitValue;

    int depLevelId;
    int myLevelId;

    int cs;

    %% write init;
    %% write exec;

    if (cs < DependMapping_first_final) {
        // TODO: line number
        ythrow TWithBackTrace<yexception>() << "failed to parse param mapping '" << buf << "'";
    }

    return r;
}
