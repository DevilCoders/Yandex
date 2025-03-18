#include "command_alias.h"
#include "command_options.h"
#include "http.h"
#include "terminal.h"

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/httpdate.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/stream/null.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/system/env.h>

using namespace fetch;

namespace {

TString GetCmToken() {
    TString envToken = GetEnv("CM_TOKEN");
    if (envToken) {
        return envToken;
    } else {
        TString tokenPath = GetEnv("CM_TOKEN_PATH");
        TFsPath path(tokenPath);
        return path.IsFile() ? Strip(TIFStream(path).ReadAll()) : TString();
    }
}

TVector<TString> GetAuthenticationHeaders(const TMaybe<TString>& user, const TMaybe<TString>& password) {
    TVector<TString> result;
    if (user.Defined()) {
        result.push_back(TUrlFetcher::BasicAuthHeaderLine(*user, *password));
    }

    TString cmToken = GetCmToken();
    if (cmToken) {
        result.push_back(TUrlFetcher::OAuthHeaderLine(cmToken));
    }

    return result;
}


inline TString Escape(const TString& string) {
    TString result(string);
    UrlEscape(result);
    return result;
}

template <class T>
struct TMatch {
    const T& What;
    size_t Matches;

    inline TMatch(const T& what)
        : What(what)
        , Matches(0)
    {
    }

    inline TMatch& operator()(const T& with) noexcept {
        Matches += (What == with);
        return *this;
    }

    inline operator size_t() const noexcept {
        return Matches;
    }
};

template <class T>
inline TMatch<T> Match(const T& what) {
    return TMatch<T>(what);
}

struct TCommand {
    const char* const Command;
    const char* const Description;
};

struct TExtractCommand {
    template <class T>
    inline const char* operator()(const T& from) const noexcept {
        return from.Command;
    }
};

template <size_t SIZE>
TStringBuf GetCommandDescription(const TCommand (&table)[SIZE], const TCiString& command) noexcept {
    for (const TCommand* i = table; i != table + SIZE; ++i)
        if (command == i->Command)
            return TStringBuf(i->Description);
    return TStringBuf();
}

struct TUrlAlias {
    const char* const Alias;
    const char* const Url;

    struct TFindAlias {
        const TString Alias;

        TFindAlias(const TString& alias)
            : Alias(alias)
        {
        }

        inline bool operator()(const TUrlAlias& what) const noexcept {
            return Alias == what.Alias;
        }
    };
} UrlAliasesTable[] = {
    { "ym",                 "https://ossa2.yandex.ru/watch-yandex-merge" },
    { "YM",                 "https://ossa2.yandex.ru/yandex-merge" },
    { "rm",                 "https://ossa2.yandex.ru/watch-robot-merge" },
    { "RM",                 "https://ossa2.yandex.ru/robot-merge" },
};

const TUrlAlias* const UrlAliasesBegin = UrlAliasesTable;
const TUrlAlias* const UrlAliasesEnd = UrlAliasesBegin + Y_ARRAY_SIZE(UrlAliasesTable);

const TCommand SimpleCommandsTable[] = {
    { "reloadscript",       "Reloads clustermater control script" },
};

const TCommand ReportCommandsTable[] = {
    { "whatstate",          "Prints list of target states" },
    { "cronstate",          "Prints cron state of specified target" },
    { "whattimes",          "Prints times of specified target" },
    { "whatclusters",       "Prints some strange numbers about the target specified" },
    { "followers",          "Prints list of followers of the specified target, including indirect" },
    { "dependencies",       "Prints list of dependencies of the specified target, including indirect" },
    { "diamond",            "Prints list of targets between two specified" },
};

const TCommand VariablesCommandsTable[] = {
    { "setvar",             "Set worker variable" },
    { "unsetvar",           "Unset worker variable" },
};

const TCommand RequireCommandsTable[] = {
    { "require",            "Wait until list of states will be within the specified one" },
    { "requireall",         "Wait until list of states will be equal to the specified one" },
    { "requireany",         "Wait until list of states will instersect the specified one" },
    { "requireno",          "Wait until list of states will not intersect the specified one" },
    { "requirepath",        "Wait until list of states of target with its dependencies will be within the specified one" },
    { "requirevar",         "Require variable" },
};

const TCommand TransferCommandsTable[] = {
    { "copy",               "Copy specified file or directory from one host to another" },
};

struct TOptions: NLastGetopt::TOpts, TNonCopyable {
    TString Url;

    bool Verbose;
    unsigned Delay;
    unsigned Retries;
    TMaybe<TString> User;
    TMaybe<TString> Password;

    template <class T, class V = T>
    struct TValueSetter: NLastGetopt::IOptHandler {
        T* const Variable;
        const V Value;

        TValueSetter(T* variable, typename TTypeTraits<V>::TFuncParam value)
            : Variable(variable)
            , Value(value)
        {
        }

        void HandleOpt(const NLastGetopt::TOptsParser*) override {
            *Variable = Value;
        }
    };

    template <class T, class V>
    struct TSetter: NLastGetopt::IOptHandler {
        T* const Variable;

        TSetter(T* variable)
            : Variable(variable)
        {
        }

        void Set(const TStringBuf& value) {
            *Variable = FromString<V>(value);
        }

        void HandleOpt(const NLastGetopt::TOptsParser* parser) override {
            Set(parser->CurValOrDef());
        }
    };

    template <class T, class V>
    struct TFromEnvSetter: TSetter<T, V> {
        TFromEnvSetter(T* variable)
            : TSetter<T, V>(variable)
        {
        }

        void HandleOpt(const NLastGetopt::TOptsParser* parser) override {
            const TString envvar(parser->CurValOrDef());
            const TString value = GetEnv(envvar);
            if (!value)
                throw yexception() << "Environment variable \"" << envvar << "\" is not set";
            this->Set(value);
        }
    };

    TOptions(const TString& url)
        : Url(url)
        , Verbose(false)
        , Delay(1)
        , Retries(5)
    {
        SetFreeArgsMin(0);
        SetFreeArgsMax(0);
        AddCharOption('v', "Verbose output").NoArgument().Handler(new TValueSetter<bool>(&Verbose, true));
        AddCharOption('d', "Delay between retries").RequiredArgument("seconds").DefaultValue(ToString(Delay)).StoreResult(&Delay);
        AddCharOption('r', "Number of retries").RequiredArgument("count").DefaultValue(ToString(Retries)).StoreResult(&Retries);
        AddCharOption('u', "Username for basic access authentication").RequiredArgument("user").Handler(new TSetter<TMaybe<TString>, TString>(&User));
        AddLongOption("uvar", "Environment variable containing username for basic access authentication").RequiredArgument("name").Handler(new TFromEnvSetter<TMaybe<TString>, TString>(&User));
        AddLongOption("pvar", "Environment variable containing password for basic access authentication").RequiredArgument("name").Handler(new TFromEnvSetter<TMaybe<TString>, TString>(&Password));
    }

    TAutoPtr<NLastGetopt::TOptsParseResultException> Parse(TCiString command, int argc, char** argv, const TStringBuf& description) {
        TAutoPtr<NLastGetopt::TOptsParseResultException> result;

        GetCharOption('d').DefaultValue(ToString(Delay));
        GetCharOption('r').DefaultValue(ToString(Retries));

        if (TCiString("help") == Url) {
            Cerr << command << ": " << description << '\n';
            PrintUsage(command, Cerr);
            return result;
        }

        try {
            result.Reset(new NLastGetopt::TOptsParseResultException(this, argc, const_cast<const char**>(argv)));
        } catch (const NLastGetopt::TUsageException& e) {
            TString usage;
            TStringOutput usageOutput(usage);
            PrintUsage(command, usageOutput);
            throw yexception() << e.what() << '\n' << command << ": " << description << '\n' << usage;
        }

        return result;
    }

};

int CommonCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    opts.AddLongOption("target", "Target").RequiredArgument("name").Required();
    opts.AddLongOption("worker", "Worker").RequiredArgument("host").Optional();
    opts.AddLongOption("task", "Task index number").RequiredArgument("number").Optional();
    opts.AddLongOption("state", "Affect only the state specified").RequiredArgument("state").Optional();

    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, NCA::GetDescription(command)));

    if (!optr.Get())
        return 1;

    TString request;

    request.append("/command/").append(command).append("?target=").append(Escape(optr->Get("target")));

    if (optr->Has("worker"))
        request.append("&worker=").append(Escape(optr->Get("worker")));

    if (optr->Has("task"))
        request.append("&task=").append(Escape(optr->Get("task")));

    if (optr->Has("state"))
        request.append("&state=").append(Escape(optr->Get("state")));

    if (opts.Verbose)
        Cerr << "The " << command << " command url: " << opts.Url << request << '\n';

    TUrlFetcher fetcher(TString(opts.Url).append(request));

    for (unsigned retry = 1; retry <= opts.Retries; ++retry) {
        TNullOutput nullOutput;

        if (retry > 1) {
            Sleep(TDuration::Seconds(opts.Delay));

            if (opts.Verbose)
                Cerr << "Retrying, attempt " << retry << " of " << opts.Retries << '\n';
        }

        TVector<TString> authHeaders = GetAuthenticationHeaders(opts.User, opts.Password);
        int status;
        try {
            status = fetcher.Fetch(nullOutput, authHeaders);
        } catch (...) {
            continue;
        }

        if (status == HTTP_NO_CONTENT || status == HTTP_OK) {
            if (opts.Verbose)
                Cerr << "Command OK" << Endl;
            return 0;
        }

        if (retry == opts.Retries || opts.Verbose)
            Cerr << "Failed, HTTP status " << status << ' ' << HttpCodeStr(status) << "\n";
    }

    return 1;
}

int SimpleCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, GetCommandDescription(SimpleCommandsTable, command)));

    if (!optr.Get())
        return 1;

    TString request;

    request.append("/command/");

    if (command == "reloadscript")
        request.append("reload");

    if (opts.Verbose)
        Cerr << "The " << command << " command url: " << opts.Url << request << '\n';

    TUrlFetcher fetcher(TString(opts.Url).append(request));

    TVector<TString> authHeaders = GetAuthenticationHeaders(opts.User, opts.Password);

    for (unsigned retry = 1; retry <= opts.Retries; ++retry) {
        TNullOutput nullOutput;

        if (retry > 1) {
            Sleep(TDuration::Seconds(opts.Delay));

            if (opts.Verbose)
                Cerr << "Retrying, attempt " << retry << " of " << opts.Retries << '\n';
        }

        const int status = fetcher.Fetch(nullOutput, authHeaders);

        if (status == HTTP_NO_CONTENT || status == HTTP_OK) {
            if (opts.Verbose)
                Cerr << "Command OK" << Endl;
            return 0;
        }

        if (retry == opts.Retries || opts.Verbose)
            Cerr << "Failed, HTTP status " << status << ' ' << HttpCodeStr(status) << "\n";
    }

    return 1;
}

int ReportCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    if (Match(command)("whatclusters")("whattimes")) {
        opts.AddLongOption("target", "Target to inquire about").RequiredArgument("name").Required();
    }
    if (Match(command)("whatclusters")) {
        opts.AddLongOption("worker", "Worker").RequiredArgument("host").Required();
    }
    if (Match(command)("whatstate")) {
        opts.AddLongOption("target", "Target to inquire about").RequiredArgument("name").Optional();
        opts.AddLongOption("worker", "Worker").RequiredArgument("host").Optional();
    }
    if (Match(command)("cronstate")) {
        opts.AddLongOption("target", "Target to inquire about").RequiredArgument("name").Required();
    }
    if (Match(command)("followers")) {
        opts.AddLongOption("top", "Target to print followers of").RequiredArgument("name").Required();
    }
    if (Match(command)("dependencies")) {
        opts.AddLongOption("bottom", "Target to print dependencies of").RequiredArgument("name").Required();
    }
    if (Match(command)("diamond")) {
        opts.AddLongOption("first", "First target").RequiredArgument("name").Required();
        opts.AddLongOption("last", "Last target").RequiredArgument("name").Required();
    }

    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, GetCommandDescription(ReportCommandsTable, command)));

    if (!optr.Get())
        return 1;

    TString request;

    if (Match(command)("whatclusters"))
        request.append("/target_text/").append(Escape(optr->Get("target")).append('/').append(Escape(optr->Get("worker"))));

    if (Match(command)("whattimes"))
        request.append("/targettimes/").append(Escape(optr->Get("target")));

    if (Match(command)("cronstate")) {
        request.append("/cronstatus/").append(Escape(optr->Get("target")));
    }

    if (Match(command)("whatstate")) {
        request.append("/targetstatus");

        if (optr->Has("target")) {
            request.append('/').append(Escape(optr->Get("target")));

            if (optr->Has("worker")) {
                request.append('/').append(Escape(optr->Get("worker")));
            }
        }
    }

    if (Match(command)("followers"))
        request.append("/targets_followers/").append(Escape(optr->Get("top")));
    if (Match(command)("dependencies"))
        request.append("/targets_dependencies/").append(Escape(optr->Get("bottom")));
    if (Match(command)("diamond"))
        request.append("/targets_diamond/").append(Escape(optr->Get("first"))).append('/').append(Escape(optr->Get("last")));

    if (opts.Verbose)
        Cerr << "The " << command << " command url: " << opts.Url << request << '\n';

    TUrlFetcher fetcher(TString(opts.Url).append(request));

    TVector<TString> authHeaders = GetAuthenticationHeaders(opts.User, opts.Password);

    for (unsigned retry = 1; retry <= opts.Retries; ++retry) {
        TString response;
        TStringOutput responseOutput(response);

        if (retry > 1) {
            Sleep(TDuration::Seconds(opts.Delay));

            if (opts.Verbose)
                Cerr << "Retrying, attempt " << retry << " of " << opts.Retries << '\n';
        }

        const int status = fetcher.Fetch(responseOutput, authHeaders);

        if (status == HTTP_OK) {
            if (opts.Verbose)
                Cerr << "Command OK, response:" << Endl;
            Cout << response << Endl;
            return 0;
        }

        if (retry == opts.Retries || opts.Verbose)
            Cerr << "Failed, HTTP status " << status << ' ' << HttpCodeStr(status) << "\n";
    }

    return 1;
}

int VariablesCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    if (Match(command)("setvar")("unsetvar")) {
        opts.AddLongOption("name", "Variable to be set or unset").RequiredArgument("name").Required();
        opts.AddLongOption("worker", "Worker").RequiredArgument("host").Optional();
    }
    if (Match(command)("setvar")) {
        opts.AddLongOption("value", "Value to be set").RequiredArgument("value").Optional();
    }

    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, GetCommandDescription(VariablesCommandsTable, command)));

    if (!optr.Get())
        return 1;

    TString request;

    request.append("/variables/");

    if (command == "setvar")
        request.append("set");

    if (command == "unsetvar")
        request.append("unset");

    request.append("?name=").append(Escape(optr->Get("name")));

    if (Match(command)("setvar") && optr->Has("value"))
        request.append("&value=").append(Escape(optr->Get("value")));

    if (optr->Has("worker"))
        request.append("&worker=").append(Escape(optr->Get("worker")));

    if (opts.Verbose)
        Cerr << "The " << command << " command url: " << opts.Url << request << '\n';

    TUrlFetcher fetcher(TString(opts.Url).append(request));

    TVector<TString> authHeaders = GetAuthenticationHeaders(opts.User, opts.Password);

    for (unsigned retry = 1; retry <= opts.Retries; ++retry) {
        TNullOutput nullOutput;

        if (retry > 1) {
            Sleep(TDuration::Seconds(opts.Delay));

            if (opts.Verbose)
                Cerr << "Retrying, attempt " << retry << " of " << opts.Retries << '\n';
        }

        const int status = fetcher.Fetch(nullOutput, authHeaders);

        if (status == HTTP_NO_CONTENT || status == HTTP_OK) {
            if (opts.Verbose)
                Cerr << "Command OK" << Endl;
            return 0;
        }

        if (retry == opts.Retries || opts.Verbose)
            Cerr << "Failed, HTTP status " << status << ' ' << HttpCodeStr(status) << "\n";
    }

    return 1;
}

int RequireCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    opts.Retries = Max<unsigned>();

    if (Match(command)("require")("requireall")("requireany")("requireno")("requirepath")) {
        opts.AddLongOption("states", "List of states delimeted by comma").RequiredArgument("state1,state2...").Required();

        auto& targetOpt = opts.AddLongOption("target", "Target to request states for").RequiredArgument("name");
        if (Match(command)("requirepath")) {
            targetOpt.Required();
        } else {
            targetOpt.Optional();
        }

        opts.AddLongOption("task", "Task number to request states for").RequiredArgument("name").Optional();
    }
    if (Match(command)("requirevar")) {
        opts.AddLongOption("name", "Variable to be checked").RequiredArgument("name").Required();
        opts.AddLongOption("values", "List of suitable values delimeted by comma").RequiredArgument("value1,value2...").Required();
    }

    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, GetCommandDescription(RequireCommandsTable, command)));

    if (!optr.Get())
        return 1;

    TString request;

    TVector<TString> wanted, received;

    if (Match(command)("require")("requireall")("requireany")("requireno")("requirepath")) {
        if (Match(command)("requirepath")) {
            if (optr->Has("task")) {
                request.append("/taskpathstatus");
            } else {
                request.append("/targetpathstatus");
            }
        } else {
            if (optr->Has("task")) {
                request.append("/taskstatus");
            } else {
                request.append("/targetstatus");
            }
        }

        if (optr->Has("target"))
            request.append('/').append(Escape(optr->Get("target")));

        if (optr->Has("task"))
            request.append('/').append(Escape(optr->Get("task")));

        Split(optr->Get("states"), ",", wanted);
    } else if (command == "requirevar") {
        request.append("/varstatus/").append(Escape(optr->Get("name")));
        Split(optr->Get("values"), ",", wanted);
    }

    Sort(wanted.begin(), wanted.end());

    if (opts.Verbose)
        Cerr << "The " << command << " command url: " << opts.Url << request << '\n';

    TUrlFetcher fetcher(TString(opts.Url).append(request));

    TVector<TString> authHeaders = GetAuthenticationHeaders(opts.User, opts.Password);

    for (unsigned retry = 1; retry <= opts.Retries; ++retry) {
        TString response;
        TStringOutput responseOutput(response);

        if (retry > 1) {
            Sleep(TDuration::Seconds(opts.Delay));

            if (opts.Verbose)
                Cerr << "Retrying, attempt " << retry << " of " << opts.Retries << '\n';
        }

        const int status = fetcher.Fetch(responseOutput, authHeaders);

        if (status == HTTP_OK) {
            if (opts.Verbose)
                Cerr << "Response:\n" << response << Endl;

            Split(response.data(), ",", received);

            bool match = false;

            if (Match(command)("require")("requirevar")("requirepath")) {
                if (received.size() > 0) {
                    match = true;
                    for (TVector<TString>::const_iterator i = received.begin(); i != received.end(); ++i) {
                        if (Find(wanted.begin(), wanted.end(), *i) == wanted.end()) {
                            match = false;
                            break;
                        }
                    }
                } else { // Is essential for 'requirevar' - if variable isn't set then response would be empty. In case of target statuses empty response isn't possible.
                    match = false;
                }
            } else if (Match(command)("requireany")("requireno")) {
                bool found = false;
                for (TVector<TString>::const_iterator i = wanted.begin(); i != wanted.end(); ++i) {
                    if (Find(received.begin(), received.end(), *i) != received.end()) {
                        found = true;
                        break;
                    }
                }
                match = ((command == "requireany") == found);
            } else if (command == "requireall" && received.size() == wanted.size()) {
                Sort(received.begin(), received.end());
                match = Equal(received.begin(), received.end(), wanted.begin());
            }

            if (match) {
                if (opts.Verbose)
                    Cerr << "Condition succeeded" << Endl;
                return 0;
            } else {
                if (retry == opts.Retries || opts.Verbose)
                    Cerr << "Condition failed" << Endl;
                continue;
            }
        } else if (retry == opts.Retries || opts.Verbose) {
            Cerr << "Failed, HTTP status " << status << ' ' << HttpCodeStr(status) << "\n";
        }

        Cerr << "Condition failed" << Endl;
    }

    return 1;
}

int TransferCommand(TOptions& opts, const TCiString& command, int argc, char** argv) {
    opts.Retries = Max<unsigned>();

    if (Match(command)("copy")) {
        opts.AddLongOption("from", "Path is a file or directory").RequiredArgument("<worker>:<path>").Required();
        opts.AddLongOption("to", "Path is a directory").RequiredArgument("<worker>:<path>").Required();
    }

    const TAutoPtr<NLastGetopt::TOptsParseResultException> optr(opts.Parse(command, argc, argv, GetCommandDescription(TransferCommandsTable, command)));

    if (!optr.Get())
        return 1;

    TVector<TString> from;
    Split(optr->Get("from"), ":", from);

    TVector<TString> to;
    Split(optr->Get("to"), ":", to);

    TString pathFrom;
    TStringOutput outPathFrom(pathFrom);
    NUri::TEncoder::Encode(outPathFrom, from[1]);

    TUrlFetcher fetcher1(opts.Url + "/proxy/" + from[0] + "/torrent/share/" + pathFrom);

    TString lastModified = FormatHttpDate(0);
    TString recvdTorrent;

    bool ok = false;
    while (!ok) {
        TString response;
        TStringOutput responseOutput(response);

        TString header;
        header.append("If-Modified-Since: " + lastModified + "\r\n");

        switch (fetcher1.Fetch(responseOutput, {header})) {
            case HTTP_OK: {
                recvdTorrent = response;
                ok = true;
                break;
            }
            case HTTP_ACCEPTED: {
                lastModified = response;
                break;
            }
            default: {
                //error
                return 1;
            }
        }
        Sleep(TDuration::Seconds(5 + RandomNumber<unsigned>(3)));
    }

    TString torrent;
    TStringOutput outTorrent(torrent);
    NUri::TEncoder::Encode(outTorrent, recvdTorrent);

    TString pathTo;
    TStringOutput outPathTo(pathTo);
    NUri::TEncoder::Encode(outPathTo, to[1]);

    TUrlFetcher fetcher2(opts.Url + "/proxy/" + to[0] + "/torrent/load/" + pathTo + "/" + torrent);
    lastModified = FormatHttpDate(0);

    while (1) {
        TString response;
        TStringOutput responseOutput(response);

        TString header;
        header.append("If-Modified-Since: " + lastModified + "\r\n");

        switch (fetcher2.Fetch(responseOutput, {header})) {
            case HTTP_OK: {
                return 0;
            }
            case HTTP_ACCEPTED: {
                lastModified = response;
                break;
            }
            default: {
                //error
                return 1;
            }
        }
        Sleep(TDuration::Seconds(5 + RandomNumber<unsigned>(3)));
    }

    return 1;
}

inline void PrintUsageTableLine(const TStringBuf& first, const TStringBuf& second) {
    const int ColumnSize = 30;
    Cerr << "    " << first;
    for (int i = 0; i < ColumnSize - static_cast<int>(first.size()); ++i)
        Cerr << ' ';
    Cerr << second << '\n';
}

int Usage(const TStringBuf& program) {
    Cerr << "Usage: " << program << " <Clustermaster WebUI URL> <Command> [Command aruments]\n";
    Cerr << program << " --svnrevision\n";
    Cerr << program << " --version\n";

    Cerr << "\nTo view usage of a specific command, call \"" << program << " help <Command>\"\n\n";
    Cerr << "Commands:\n";

    for (const NCA::TCommandAlias* i = NCA::CommandAliasesBegin(); i != NCA::CommandAliasesEnd(); ++i)
        PrintUsageTableLine(i->Alias, i->Description);

    Cerr << '\n';

    for (const TCommand* i = SimpleCommandsTable; i != SimpleCommandsTable + Y_ARRAY_SIZE(SimpleCommandsTable); ++i)
        PrintUsageTableLine(i->Command, i->Description);

    Cerr << '\n';

    for (const TCommand* i = ReportCommandsTable; i != ReportCommandsTable + Y_ARRAY_SIZE(ReportCommandsTable); ++i)
        PrintUsageTableLine(i->Command, i->Description);

    Cerr << '\n';

    for (const TCommand* i = VariablesCommandsTable; i != VariablesCommandsTable + Y_ARRAY_SIZE(VariablesCommandsTable); ++i)
        PrintUsageTableLine(i->Command, i->Description);

    Cerr << '\n';

    for (const TCommand* i = RequireCommandsTable; i != RequireCommandsTable + Y_ARRAY_SIZE(RequireCommandsTable); ++i)
        PrintUsageTableLine(i->Command, i->Description);

    Cerr << '\n';

    for (const TCommand* i = TransferCommandsTable; i != TransferCommandsTable + Y_ARRAY_SIZE(TransferCommandsTable); ++i)
        PrintUsageTableLine(i->Command, i->Description);

    Cerr << "\nURL aliases:\n";

    for (const TUrlAlias* i = UrlAliasesTable; i != UrlAliasesTable + Y_ARRAY_SIZE(UrlAliasesTable); ++i)
        PrintUsageTableLine(i->Alias, i->Url);

    Cerr << '\n';

    return 1;
}

} // anonymous namespace

int remote(int argc, char **argv) {
    if (argc <= 0)
        return Usage("cmremote");

    const TStringBuf program(argv[0]);

    --argc;
    ++argv;

    if (argc <= 0)
        return Usage(program);

    const TUrlAlias* urlAlias = FindIf(UrlAliasesBegin, UrlAliasesEnd, TUrlAlias::TFindAlias(argv[0]));

    const TString url(urlAlias != UrlAliasesEnd ? urlAlias->Url : argv[0]);

    if (TCiString("--svnrevision") == url)
        NLastGetopt::PrintVersionAndExit(nullptr);

    if (TCiString("--version") == url) {
        Cerr << GetProgramSvnVersion() << Endl;
        return 0;
    }

    TOptions opts(url);

    --argc;
    ++argv;

    if (argc <= 0)
        return Usage(program);

    const TCiString command(argv[0]);

    try {
        SslStaticInit();

        // Common commands

        if (NCA::IsCommandAlias(command))
            return CommonCommand(opts, command, argc, argv);

        // Simple commands

        const TStaticStringSet SimpleCommands(TExtractCommand(), SimpleCommandsTable);

        if (SimpleCommands(command))
            return SimpleCommand(opts, command, argc, argv);

        // Report commands

        const TStaticStringSet ReportCommands(TExtractCommand(), ReportCommandsTable);

        if (ReportCommands(command))
            return ReportCommand(opts, command, argc, argv);

        // Variables commands

        const TStaticStringSet VariablesCommands(TExtractCommand(), VariablesCommandsTable);

        if (VariablesCommands(command))
            return VariablesCommand(opts, command, argc, argv);

        // Require commands

        const TStaticStringSet RequireCommands(TExtractCommand(), RequireCommandsTable);

        if (RequireCommands(command))
            return RequireCommand(opts, command, argc, argv);

        // Transfer commands

        const TStaticStringSet TransferCommands(TExtractCommand(), TransferCommandsTable);

        if (TransferCommands(command))
            return TransferCommand(opts, command, argc, argv);

    } catch (const yexception& e) {
        Cerr << e.what() << Endl;
        return 1;
    }

    Cerr << "Unknown command \"" << command << "\"" << Endl;

    return Usage(program);
}

