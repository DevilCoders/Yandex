#include <tools/printwzrd/lib/prepare_request.h>

#include <util/string/strip.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NPrintWzrd {
    bool IsMetaInfo(const TStringBuf& line) {
        // Meta-info lines either are blank lines or starting with '@'.
        // They are not processed and not printed to output.
        return line.StartsWith(TStringBuf("@")) || StripString(line).empty();
    }

    bool PrepareRequest(TString& req, bool tabbedInput, IOutputStream& message) {
        if (IsMetaInfo(req)) {
            message << req << Endl;
            return false;
        }

        if (UTF8Detect(req.c_str(), req.length()) == NotUTF8)
            ythrow yexception() << "Request has to be in UTF8";

        if (tabbedInput) {
            size_t tabPos = req.find_first_of('\t');
            if (tabPos != TString::npos)
                req.replace(tabPos, 1, "$cgi:lr=");
        }

        return true;
    }

    void PrintFailInfo(IOutputStream& out, const TString& request, bool StripLineNumbers) {
        out << "src: [" << request << "]" << Endl;
        if (!StripLineNumbers) {
            out << "[search]: [" << CurrentExceptionMessage() << "]" << Endl << Endl;
            return;
        }

        // Exception message could contain __LOCATION__ if the exception was thrown by ythrow
        // for the test purposes we would like to strip all the line numbers to restrict 'false diffs', which come from line changes
        static constexpr TStringBuf LineProxy = "LINE_NUMBER";
        static const TSetDelimiter<const char> delim(":");
        const TString message = CurrentExceptionMessage();

        TVector<TStringBuf> tokens;
        for (const auto it: StringSplitter(message).Split(':')) {
            const TStringBuf token = it.Token();
            unsigned dummy = 0;

            if (Y_UNLIKELY(TryFromString<unsigned>(token, dummy))) {
                tokens.push_back(LineProxy);
            } else {
                tokens.push_back(token);
            }
        }

        out << "[search]: [" << JoinSeq(":", tokens) << "]" << Endl << Endl;
    }

    const TStringBuf CGI_MARKER = "$cgi:";
    const TStringBuf PRINT_MARKER = "$print:";


    // head -> head marker tail
    bool SplitByMarker(TStringBuf& head, const TStringBuf& marker, TStringBuf& tail) {
        size_t pos = head.find(marker);
        if (pos == TStringBuf::npos)
            return false;

        tail = head.SubStr(pos + marker.size());
        head = head.SubStr(0, pos);
        return true;
    }


    void SplitRequestOptions(TStringBuf& request, TStringBuf& cgi, TStringBuf& print) {
        SplitByMarker(request, CGI_MARKER, cgi);
        if (!SplitByMarker(request, PRINT_MARKER, print))
            SplitByMarker(cgi, PRINT_MARKER, print);
        cgi = StripString(cgi);
        print = StripString(print);
    }

    void FillCgiParams(const TStringBuf& request, const TStringBuf& cgi, const TPrintwzrdOptions& options, TCgiParameters& cgiParams) {
        cgiParams.ScanAdd(cgi);
        if (!options.CgiInput) {
            cgiParams.InsertUnescaped("user_request", request);
            cgiParams.InsertUnescaped("text", request); //report puts ParsedRequest
        } else {
            cgiParams.ScanAdd(request.substr(request.find('?') + 1));
        }

        // Run internal wizard in debug mode always (except remote mode without explicit -w)
        // but output internal properties only in debug mode.
        if (options.DebugMode || options.Port == 0) {
            cgiParams.ScanAdd("dbgwzr=2");
            cgiParams.ScanAdd("wizextra=print_metadata");
        }

        if (!options.AppendCgi.empty())
            cgiParams.ScanAdd(options.AppendCgi);
    }

    void ParseRequest(TStringBuf request, const TPrintwzrdOptions& options, TCgiParameters& cgiParams, TStringBuf& rulesToPrint) {
        TStringBuf cgi;
        SplitRequestOptions(request, cgi, rulesToPrint);
        FillCgiParams(request, cgi, options, cgiParams);
    }

    TCgiParameters MakeCgiParams(TStringBuf request, const TPrintwzrdOptions& options) {
        TCgiParameters cgi;
        TStringBuf print;
        ParseRequest(request, options, cgi, print);
        return cgi;
    }

}
