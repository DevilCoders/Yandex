#include "accesslog.h"

#include "unsorted.h"

#include <library/cpp/getopt/opt.h>

#include <util/datetime/base.h>
#include <library/cpp/digest/old_crc/crc.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

#include <cctype>
#include <ctime>

TAccessLogLoader::TAccessLogLoader()
    : Format_("%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\" \"%{Host}i,%p\" \"%{X-Forwarded-For}i\" \"%{Cookie}i\" %{%s}t \"%T\" %D %C")
    , FixAuthCookie_(false)
{
}

TAccessLogLoader::~TAccessLogLoader() {
}

TString TAccessLogLoader::Opts() {
    return "f:h:v:WY";
}

void TAccessLogLoader::Usage() const {
    Cerr << "   -f apache log format string" << Endl
         << "   -h host to connect to" << Endl
         << "   -v default virtual host" << Endl
         << "   -Y apply some Yandex hacks(for example, modify L cookie)" << Endl
         << "   -W use realhost-prefixed log format" << Endl;
}

bool TAccessLogLoader::HandleOpt(const TOption* option) {
    switch (option->key) {
        case 'f': {
            if (!option->opt) {
                ythrow yexception() << "-f require argument";
            }

            Format_ = option->opt;

            return true;
        }

        case 'h': {
            if (!option->opt) {
                ythrow yexception() << "-h expects an argument(host)";
            }

            Host_ = option->opt;

            return true;
        }

        case 'W': {
            Format_ = "%H " + Format_;

            return true;
        }

        case 'Y': {
            FixAuthCookie_ = true;

            return true;
        }

        case 'v': {
            if (!option->opt) {
                ythrow yexception() << "-v expects an argument(virtual host)";
            }

            VirtualHost_ = option->opt;

            return true;
        }

        default:
            break;
    }

    return false;
}

/*
 * TODO
 */

class TFixAuthCookie {
public:
    TFixAuthCookie(TString& s)
        : v(s)
    {
        vcurrent = v.begin();
        vend = v.end();
        atr_begin = nullptr;
        atr_end = nullptr;

        while (Next()) {
            if (atr_end - atr_begin == 1 && (*atr_begin == 'L' || *atr_begin == 'l')) {
                *atr_begin = 'F';
            }
        }
    }

private:
    TString& v;
    char* vcurrent;
    const char* vend;

    char* atr_begin;
    const char* atr_end;

    static inline bool IsSpecial(char x) {
        switch (x) {
            case '(':
            case ')':
            case '<':
            case '>':
            case '@':
            case ',':
            case ';':
            case ':':
            case '\\':
            case '"':
            case '/':
            case '[':
            case ']':
            case '?':
            case '=':
            case '{':
            case '}':
            case ' ':
            case '\t':
               return true;

            default:
                return false;
        }
    }

    static inline void SkipSpaces(char*& current, const char* end) {
        while (current != end && std::isspace(*current)) {
            ++current;
        }
    }

    static inline void SkipToken(char*& current, const char* end) {
        while (current != end && !(iscntrl(*current) || IsSpecial(*current))) {
            ++current;
        }
    }

    static inline void SkipCookie(char*& current, const char* end) {
        while (current != end && *current != ';') {
            ++current;
        }
        if (current != end) {
            current++;
        }
    }

    bool Next() {
        char*& curr = vcurrent;
        const char* end = vend;

        for (;;) {
            // Looking for cookies...
            bool normal_cookie = true;

            // Skipping whitespace until we got the beginning of cookie...
            SkipSpaces(curr, end);
            if (curr == end) {
                break;
            }

            // Retrieving cookie name...
            atr_begin = curr;
            SkipToken(curr, end);
            atr_end = curr;

            // We failed to get cookie name...
            if (atr_begin == atr_end) {
                normal_cookie = false;
            }

            if (normal_cookie) {
                // Skipping whitespace until we got the equality sign...
                SkipSpaces(curr, end);
                if (curr == end) {
                    break;
                }

                normal_cookie = *curr == '=';
            }

            // Skipping to the next cookie...
            SkipCookie(curr, end);
            if (normal_cookie) {
                return true;
            }

            Cerr << "malformed cookie found" << Endl;
        }

        atr_begin = nullptr;
        atr_end = nullptr;

        return false;
    }
};

struct TRecord {
    struct TFormatOpts {
        bool fix_auth_cookie;
    };

    inline TRecord(const TString& RealHost, const TString& Host)
        : host(Host)
        , real_host(RealHost)
        , time(0)
        , ms(~0u)
        , port(80)
    {
    }

    inline TString Format(const TFormatOpts& opts) const {
        if (time == 0 || request.empty() || VirtualHost().empty()) {
            ythrow yexception() << "insufficient data";
        }

        TString ret;

        ret += request;
        ret += "\r\n";

        for (THttpHeaders::TConstIterator h = headers.Begin(); h != headers.End(); ++h) {
            TString n = h->Name();
            TString v = h->Value();

            if (opts.fix_auth_cookie && stricmp(n.data(), "cookie") == 0) {
                TFixAuthCookie fixer(v);
            }

            ret += n;
            ret += ": ";
            ret += v;
            ret += "\r\n";
        }

        {
            /*
             * write proper host header
             */

            ret += "Host: ";
            ret += VirtualHost();

            if (port != 80) {
                ret += ":";
                ret += ToString(port);
            }

            ret += "\r\n";
        }

        ret += "\r\n";

        return ret;
    }

    inline TString Host() const {
        if (!real_host.empty()) {
            return real_host;
        }

        return host;
    }

    inline TString VirtualHost() const {
        if (!host.empty()) {
            return host;
        }

        return real_host;
    }

    inline TInstant Time() const noexcept {
        if (ms == ~0u)
            return TInstant::MicroSeconds((ui64)time * (ui64)1000000 + (Hash() % (ui64)1000000) - (ui64)500000);
        else
            return TInstant::MicroSeconds((ui64)time * (ui64)1000000 + ms);
    }

    inline ui64 Hash() const noexcept {
        return crc64(request.data(), request.size(), time);
    }

    THttpHeaders headers;
    TString request;
    TString host;      // host header (and host to connect to for real_host.empty() case)
    TString real_host; // host to connect to
    time_t time;
    ui32 ms;
    ui16 port;
};

class IParserPart {
    public:
        inline IParserPart() noexcept {
        }

        virtual ~IParserPart() {
        }

        virtual void Parse(const TString& s, TRecord& rec) = 0;
};

class TLastPart: public IParserPart {
    public:
        inline TLastPart() noexcept {
        }

        ~TLastPart() override {
        }

        void Parse(const TString& s, TRecord& /*rec*/) override {
            if (!s.empty()) {
                ythrow yexception() << "spurious content at end of line(" <<  s.data() << ")";
            }
        }
};

class TPortPart: public IParserPart {
    public:
        inline TPortPart() noexcept {
        }

        ~TPortPart() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            rec.port = FromString<ui16>(s);
        }
};

class TRequestPart: public IParserPart {
    public:
        inline TRequestPart() noexcept {
        }

        ~TRequestPart() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            rec.request = s;
        }
};

class THeaderPart: public IParserPart {
    public:
        inline THeaderPart(const TString& name)
            : Name_(name)
        {
        }

        ~THeaderPart() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            if (stricmp(Name_.data(), "host") == 0) {
                if (rec.host.empty()) {
                    const size_t pos = s.find(':');

                    if (pos == TString::npos) {
                        rec.host = s;
                    } else {
                        rec.host = s.substr(0, pos);
                        rec.port = FromString<ui16>(s.substr(pos + 1));
                    }
                } else {
                    Cerr << "skip virtual host - it already set" << Endl;
                }
            } else {
                rec.headers.AddHeader(THttpInputHeader(Name_, s));
            }
        }

    private:
        TString Name_;
};

class TRealHostPart: public IParserPart {
    public:
        inline TRealHostPart() {
        }

        ~TRealHostPart() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            rec.real_host = s;
        }
};

class TSkipPart: public IParserPart {
    public:
        inline TSkipPart() noexcept {
        }

        ~TSkipPart() override {
        }

        void Parse(const TString& /*s*/, TRecord& /*rec*/) override {
        }
};

class TTimePart: public IParserPart {
    public:
        inline TTimePart(const TString& format)
            : Format_(format)
        {
        }

        ~TTimePart() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            if (Format_ == "msec") {
                ui64 vl = 0;
                if (!TryFromString(s, vl)) {
                    ythrow yexception() << "can not parse time(" << Format_ << " -> " << s << ")";
                }
                rec.time = vl / 1000000;
                rec.ms = vl % 1000000;
                return;
            }

            struct tm t;

            memset(&t, 0, sizeof(t));

            if (strptime(s.data(), Format_.data(), &t) == nullptr) {
                ythrow yexception() << "can not parse time(" << Format_ << " -> " << s << ")";
            }

            const time_t T = mktime(&t);

            if (T > rec.time) {
                rec.time = T;
            }
        }

    private:
        TString Format_;
};

class TParser: public IParserPart {
        typedef TSimpleSharedPtr<IParserPart> TPart;
        typedef TVector<TPart> TParts;
        typedef TVector<TString> TTexts;
    public:
        inline TParser(TString format) {
            {
                size_t pos;

                while ((pos = format.find("\\t")) != TString::npos) {
                    format = format.substr(0, pos) + "\t" + format.substr(pos + 2);
                }
            }

            TString::const_iterator it = format.begin();

            while (it != format.end()) {
                it = ReadText(it, format.end());
                it = ReadSpecial(it, format.end());
            }

            if (Parts_.size() != Texts_.size()) {
                ythrow yexception() << "shit happen(" <<  (unsigned)Parts_.size() << ", " <<
                    (unsigned)Texts_.size() << ")";
            }
        }

        ~TParser() override {
        }

        void Parse(const TString& s, TRecord& rec) override {
            TParts::iterator p = Parts_.begin();
            TTexts::iterator t = Texts_.begin();
            size_t pos = 0;

            while (p != Parts_.end()) {
                const TString& txt = *t;

                if (strncmp(s.data() + pos, txt.data(), txt.size()) != 0) {
                    ythrow yexception() << "can not parse line(" << s << ", " << txt << ", " << pos << ")";
                }

                pos += txt.size();

                TString data;

                if (t == Texts_.end() - 1) {
                    data = s.substr(pos);
                } else {
                    const size_t next = s.find(*(t + 1), pos);

                    if (next == TString::npos) {
                        ythrow yexception() << "can not parse line(" << s << ")";
                    }

                    data = s.substr(pos, next - pos);
                    pos = next;
                }

                if (data != "-") {
                    (*p)->Parse(data, rec);
                }

                ++p;
                ++t;
            }
        }

    private:
        inline void Add(TPart part) {
            Parts_.push_back(part);
        }

        inline TString::const_iterator ReadText(TString::const_iterator b, TString::const_iterator e) {
            TString txt;

            while (b != e && *b != '%') {
                txt += *b++;
            }

            Texts_.push_back(txt);

            return b;
        }

        inline TString::const_iterator ReadSpecial(TString::const_iterator b, TString::const_iterator e) {
            if (b == e){
                Add(TPart(new TLastPart()));

                return e;
            }

            TString spec;
            char val;

            ++b;

            if (b == e) {
                ythrow yexception() << "can not parse % token";
            }

            if (*b == '{') {
                while (++b != e && *b != '}') {
                    spec += *b;
                }

                if (b == e) {
                    ythrow yexception() << "can not parse %{ token";
                }

                ++b;

                if (b == e) {
                    ythrow yexception() << "can not parse %{} token";
                }
            }

            val = *b;

            AddSpecial(spec, val);

            return ++b;
        }

        inline void AddSpecial(TString spec, int val) {
            switch (val) {
                case 'i':
                    Add(TPart(new THeaderPart(spec)));
                    break;

                case 't':
                    if (spec.empty()) {
                        spec = "[%d/%b/%Y:%H:%M:%S";
                    }

                    Add(TPart(new TTimePart(spec)));
                    break;

                case 'r':
                    Add(TPart(new TRequestPart()));
                    break;

                case 'p':
                    Add(TPart(new TPortPart()));
                    break;

                case 'H':
                    Add(TPart(new TRealHostPart()));
                    break;

                default:
                    Add(TPart(new TSkipPart()));
                    break;
            }
        }

    private:
        TParts Parts_;
        TTexts Texts_;
};

void TAccessLogLoader::Process(TParams* params) {
    TParser parser(Format_);
    TMonotonicAdapter ma(params);
    TString line;
    size_t num = 0;

    while (params->Input()->ReadLine(line)) {
        ++num;

        try {
            TRecord rec(Host_, VirtualHost_);
            TRecord::TFormatOpts opts = {FixAuthCookie_};

            parser.Parse(line, rec);
            ma.Add(rec.Time(), TDevastateItem(TDuration::Zero(), rec.Host(), rec.port, rec.Format(opts), 0));
        } catch (...) {
            Cerr << CurrentExceptionMessage() << " at line " << num << Endl;
        }
    }

    ma.Flush();
}
