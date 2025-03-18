#pragma once

#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/http/server/http_ex.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/pcdata/pcdata.h>

#include <util/random/random.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/strip.h>

#include <cstdio>
#include <cstdlib>

namespace NSegutils {

const char * const FileMode = "file_mode";
const char * const UrlForm = "url_form";
const char * const FileForm = "file_form";
const char * const WebUrl = "web_url";
const char * const FileLocation = "file_location";
const char * const FileUrl = "file_url";
const char * const Charset = "charset";
const char * const Randurl = "randurl";
const char * const ShowHtml = "show_html";
const char * const IgnoreNoindex = "ignore_noindex";

inline TString RenderLinkUrl(TString url, bool escapehtml, bool hideref = true) {
    if (hideref) {
        Quote(url, "");
        url = Sprintf("http://h.yandex.net/?%s", url.data());
    }

    if (escapehtml)
        url = EncodeHtmlPcdata(url.data(), true);

    return url;
}

class TRequestBase: public THttpClientRequestEx {
protected:
    TParserContext ParserContext;
    TString ConfigDir;
    TVector<TString> Errors;

    TString Location;
    THtmlDocument RawDoc;

    THashSet<TString> DisabledModes;

public:
    TRequestBase(const TString& configDir)
        : THttpClientRequestEx()
        , ParserContext(configDir)
        , ConfigDir(configDir)
    {
    }

    void DisableMode(const TString& mode) {
        DisabledModes.insert(mode);
    }

    bool IsDisabled(const TString& mode) const {
        return DisabledModes.find(mode) != DisabledModes.end();
    }

    bool HasLocation() const {
        return !Location.empty();
    }

    TString GetFileMode() const {
        TString mode = RD.CgiParam.Get(FileMode);
        return !!mode ? mode : UrlForm;
    }

    TString GetWebUrl() const {
        return CheckHttp(Strip(RD.CgiParam.Get(WebUrl)));
    }

    TString GetFileLocation() const {
        TString s = Strip(RD.CgiParam.Get(FileLocation));
        return s.empty() ? s : (s.StartsWith("file://") || s.StartsWith('/')) ? CheckFile(s)
                : CheckHttp(s);
    }

    TString GetFileUrl() const {
        return CheckHttp(Strip(RD.CgiParam.Get(FileUrl)));
    }

    TString GetCharset() const {
        return Strip(RD.CgiParam.Get(Charset));
    }

    ECharset GetCharsets(ECharset enc = CODES_UNKNOWN) const {
        TString cs = GetCharset();

        if (!cs.empty()) {
            return CharsetByName(cs.c_str());
        }

        return enc;
    }

    bool GetRandomUrl() const {
        return !!RD.CgiParam.Get(Randurl);
    }

    bool GetShowHtml() const {
        return !!RD.CgiParam.Get(ShowHtml);
    }

    bool GetIgnoreNoindex() const {
        return !!RD.CgiParam.Get(IgnoreNoindex);
    }

    bool IsReady() const {
        return !!Location;
    }

    bool Reply(void*) override {
        try {
            ProcessHeaders();
            RD.Scan();
            const char * crlf = "\r\n";

            Output() << "HTTP/1.0 " << static_cast<int> (HTTP_OK) << " " << HttpCodeStr(HTTP_OK)
                    << crlf;

            Output() << "Content-Type: text/" << (GetRandomUrl() ? "plain" : "html");

            if (GetShowHtml()) {
                InitDoc();
                if (!ValidCharset(RawDoc.ForcedCharset)) {
                    TString dir = !ConfigDir ? ToString(".") : ToString(ConfigDir);
                    THtProcessor proc;
                    proc.Configure((dir + "/htparser.ini").data());
                    THolder<IParsedDocProperties> props(proc.ParseHtml(RawDoc.Html.data(), RawDoc.Html.size()));
                    TRecognizerShell recogn(dir + "/dict.dict");
                    TRecognizer::THints hints;
                    hints.HttpCodepage = RawDoc.HttpCharset;
                    hints.Url = RawDoc.Url.data();
                    recogn.Recognize(proc.GetStorage().Begin(), proc.GetStorage().End(),
                        RawDoc.GuessedCharset, RawDoc.GuessedLanguages, hints);
                }

                if (ValidCharset(RawDoc.GuessedCharset))
                    Output() << "; charset=" << NameByCharset(RawDoc.GuessedCharset) << crlf;
                else
                    Output() << crlf;
            } else {
                Output() << "; charset=utf8" << crlf;
            }


            Output() << crlf;

            if (GetRandomUrl()) {
                Output() << RenderRandomUrl();
            } else if (GetShowHtml()) {
                Output() << RawDoc.Html;
            } else {
                if (!RD.Query().empty()) {
                    InitPage();
                }
                Output() << RenderPage();
            }

        } catch (const yexception& ye) {
            Cerr << "exception in process " << ye.what();
        } catch (const std::exception& e) {
            Cerr << "exception in process " << e.what();
        } catch (...) {
            FinalizePage();
            throw ;
        }

        FinalizePage();
        return true;
    }

    virtual bool HasErrors() const {
        return !Errors.empty();
    }

protected:
    virtual void InitPage() {InitDoc();}
    virtual TString RenderSubForm() const {return "";}
    virtual TString RenderContent() const {return "";}
    virtual void FinalizePage() {}

    virtual TString RenderStyle() const;
    virtual TString RenderFrontScripts() const;
    virtual TString RenderBackScripts() const;

    virtual void InitDoc();

    virtual TString RenderStats() const;
    virtual TString RenderPage() const;
    TString RenderSvnVersionInfo() const;
    virtual TString RenderForm() const;
    virtual TString RenderTitle() const;
    virtual TString RenderErrors() const;

    virtual TString RenderRandomUrl() const;

protected:

    static TString CheckHttp(TString url) {
        static const TString HTTPS_PREF = "https://";
        TStringBuf scheme = "http://";
        if (url.StartsWith(HTTPS_PREF))
            scheme = HTTPS_PREF;
        return CheckScheme(url, TString{scheme});
    }

    static TString CheckFile(TString url, bool stripFile = false) {
        return CheckScheme(url, "file://", stripFile);
    }

    static TString CheckScheme(TString url, TString scheme, bool stripScheme = false) {
        if(url.empty()) {
            return url;
        }
        return (url.StartsWith(scheme) ? url : scheme + url).substr(stripScheme ? scheme.size() : 0);
    }

    static bool GetValidFileName(TString& url) {
        if (url.StartsWith("file://")) {
            url = url.substr(strlen("file://"));
        }

        return true;
    }

};

template<typename TRequest>
class TServer: public THttpServer, public THttpServer::ICallBack {
    TString ConfigDir;
    bool Production;

    THttpServer::TOptions CreateOptions(ui16 port, ui32 nThreads) {
        THttpServer::TOptions options;
        options.Port = port;
        options.nThreads = nThreads;
        return options;
    }
public:
    TServer(const ui16 port, const ui32 nthreads, TString configDir, bool production)
    : THttpServer(this, CreateOptions(port, nthreads)) , ConfigDir(configDir), Production(production) {
    }

    void AddLog(const TString& l) {
        Clog << l << Endl;
    }
protected:
    TClientRequest* CreateClient() override {
        TRequest * req = new TRequest(ConfigDir);

        if(Production)
            req->DisableMode(FileForm);

        return req;
    }
};

static void Usage(const char * me, int defport, int defnThreads, TString defconfigDir) {
    Cerr << me << " [-n nThreads] [-p port] [-c configDir] [-s]" << Endl;
    Cerr << "\t" << "-n nThreads: n threads to run [" << defnThreads << "]" << Endl;
    Cerr << "\t" << "-p port: port for handling requests [" << defport << "]" << Endl;
    Cerr << "\t" << "-c configDir: dir with htparser.ini, dict.dict and 2ld.list";
    Cerr << " [" << defconfigDir << "]" << Endl;
    Cerr << "\t" << "-s : production mode, only web documents are enabled" << Endl;
}

static inline void GetOpts(int argc, const char** argv, int& port, int& nthreads, TString& confdir, bool& production) {
    int defport = port;
    int defnthreads = nthreads;
    TString defconfdir = confdir;

    if(argc > 1 && strcmp(argv[1], "--help") == 0) {
        Usage(argv[0], defport, defnthreads, defconfdir);
        exit(0);
    }

    Opt opts(argc, argv, "p:n:c:s");
    int optlet;

    while (EOF != (optlet = opts.Get())) {
        switch (optlet) {
            case 'p': {
                port = FromString<int> (opts.GetArg());
                break;
            }case 'n': {
                nthreads = FromString<int> (opts.GetArg());
                break;
            }case 'c': {
                confdir = opts.GetArg();
                break;
            }case 's': {
                production = true;
                break;
            }default: {
                Usage(argv[0], defport, defnthreads, defconfdir);
                exit(0);
            }
        }
    }
}

template<typename TRequest>
static void StartServer(int argc, const char** argv, int defport, int defnThreads, TString defconfigDir) {
    int port = defport;
    int nThreads = defnThreads;
    TString configDir = defconfigDir;
    bool production = false;

    GetOpts(argc, argv, port, nThreads, configDir, production);

    signal(SIGPIPE, SIG_IGN);

    Clog << "Creating server" << Endl;

    TServer<TRequest> serv(port, nThreads, configDir, production);

    Clog << "Port: " <<port << " NThreads: " << nThreads << " ConfigDir: " << configDir << Endl;

    if(production) {
        Clog << "Production mode" << Endl;
    }

    serv.AddLog("Starting server");
    serv.Start();
    serv.AddLog("Waiting for requests");

    serv.Wait();

    serv.AddLog("Turning off");
    sleep(1);
}

inline TString RenderCheckbox(TStringBuf label, TStringBuf name, bool value) {
    return Sprintf("<label>%s <input type=checkbox name=%s value=true %s ></label>", label.data(), name.data(), (value ? "checked" : ""));
}

inline TString RenderRadio(TStringBuf label, TStringBuf name, ui32 value, bool checked) {
    return Sprintf("<label><input type=radio name=%s value=%u %s >%s</label>", name.data(), value, (checked ? "checked" : ""), label.data());
}

inline TString RenderSelect(TStringBuf label, TStringBuf name,
                           TStringBuf option0, int value0,
                           TStringBuf option1, int value1,
                           TStringBuf option2, int value2,
                           TStringBuf option3, int value3,
                           TStringBuf option4, int value4,
                           int checked) {
    return Sprintf("<label>%s"
                    "<select name=%s>"
                        "<option value=%u %s >%s</option>"
                        "<option value=%u %s >%s</option>"
                        "<option value=%u %s >%s</option>"
                        "<option value=%u %s >%s</option>"
                        "<option value=%u %s >%s</option>"
                    "</select>"
                    "</label>", label.data(), name.data(),
                    value0, (checked == value0 ? "selected" : ""), option0.data(),
                    value1, (checked == value1 ? "selected" : ""), option1.data(),
                    value2, (checked == value2 ? "selected" : ""), option2.data(),
                    value3, (checked == value3 ? "selected" : ""), option3.data(),
                    value4, (checked == value4 ? "selected" : ""), option4.data()
                    );
}

inline TString RenderInput(TStringBuf label, TStringBuf name, TStringBuf value) {
    return Sprintf("<label>%s <input name=%s value=\"%s\"></label>", label.data(), name.data(), EncodeHtmlPcdata(value.data()).data());
}

inline TString RenderInput(TStringBuf name, TStringBuf value) {
    return Sprintf("<input name=%s value=\"%s\">", name.data(), EncodeHtmlPcdata(value.data()).data());
}

}
