#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>  //sort()
#include <iostream>

#include <util/system/compat.h>
#include <util/stream/output.h>
#include <util/stream/input.h>

#include <library/cpp/tld/tld.h>

#if defined (__WIN32__) || defined(_WIN32)
#   include <malloc.h>
#endif

#include "robottxt.h"

#ifndef MAX_ALLOCA
#define MAX_ALLOCA 8192
#endif

struct my_less1 {
    bool operator()(const char *a, const char *b) const {
        return strcmp(a,b)<0;
    }
};
struct my_less2 {
    bool operator()(const char *a, const char *b) const {
        const char *A = a + strlen(a) - 1, *B = b + strlen(b) - 1;
        for (; A >= a && B >= b; A--, B--) { // note: one test may be eliminated
            if (*A != *B) return *A < *B;
        }
        return A < a && B >= b;
    }
};
inline int str_end_cmp(const char *a, const char *b) {
    const char *A = a + strlen(a) - 1, *B = b + strlen(b) - 1;
    for (; A >= a && B >= b; A--, B--) { // note: one test may be eliminated
        if (*A != *B) return (int)(ui8)*A - (int)(ui8)*B;
    }
    return -(B >= b);
}

RobotTxt::RobotTxt(const char *name) :
    Rules(nullptr),RevRules(nullptr), HostDirective(nullptr),
    NR(0),NRR(0), MaxNR(0),MaxNRR(0), CrawlDelay(0), PackedRules(nullptr)
{
    Agent = nullptr, lAgent = 0;
    SetAgent(name);
}

RobotTxt::~RobotTxt()
{
    Clear(nullptr, nullptr);
    free(Agent);
}

size_t RobotTxt::GetPackedRules(const char *&out) const {
    out = PackedRules;
    return PackedRules? *(ui32*)PackedRules : 0;
}

void RobotTxt::SetAgent(const char *name)
{
    if (name || !Agent) {
        free(Agent);
        Agent = strdup(name ? name : "yandex");
        Agent = strlwr(Agent);
        lAgent = strlen(Agent);
    }
}

void RobotTxt::Sort()
{
    std::sort(Rules, Rules+NR, my_less1());
    std::sort(RevRules, RevRules+NRR, my_less2());
}

int RobotTxt::Disallow(const char* url) const
{
    if (NR < 0) return 1; // full disallow
    int n = 0, k = NR; //log search through sorted array
    while (n < k) {
        int i = (n + k) / 2;
        int j = strncmp(url, Rules[i], strlen(Rules[i]));
        if (!j)
            return 1;
        if (j < 0)
            k = i;
        else
            n = i + 1;
    }
    n = 0, k = NRR; //the same for line ends
    while (n < k) {
        int i = (n + k) / 2;
        int j = str_end_cmp(url, RevRules[i]);
        if (!j)
            return 1;
        if (j < 0)
            k = i;
        else
            n = i + 1;
    }
    return 0;
}

int RobotTxt::Compare(const RobotTxt &r) const
{
    // stupid algorithm n*ln(n), but effective linear (n) code is quite large while speed is not critical here
    int /*rv = 0,*/ i;
    int mc = 0, rc = 0;
    for (i = 0; i < NR; i++) {
        if (!r.Disallow(Rules[i])) {
            mc++;
        }
    }

    for (i = 0; i < r.NR; i++) {
        if (!Disallow(r.Rules[i])) {
            rc++;
        }
    }
    if (mc == 0)
        if (rc == 0) return 0;
        else return -1;
    else
        if (rc == 0) return 1;
    if (mc < rc) return -2;
    return 2;
    //if (rv == 2 && NR < r.NR) rv = -2;
    //return rv;
}

void RobotTxt::Clear(char const* const buf_beg, char const *const buf_end)
{
    int i;
    if (!PackedRules) {
        for (i = 0; i < NR; i++)
            if (Rules[i] < buf_beg || Rules[i] >= buf_end) delete Rules[i];
        for (i = 0; i < NRR; i++)
            if (RevRules[i] < buf_beg || RevRules[i] >= buf_end) delete RevRules[i];
        free(HostDirective);
    }
    free(Rules);
    free(RevRules);
    delete [] PackedRules;
    Rules = nullptr;
    NR = MaxNR = 0;
    RevRules = nullptr;
    NRR = MaxNRR = 0;
    HostDirective = nullptr;
    PackedRules = nullptr;
    CrawlDelay = 0;
}

inline void string_down(char *&cur, char *&src, char const* const buf_beg, char const *const buf_end) {
    int len = strlen(src);
    strcpy(cur, src);
    if (src < buf_beg || src >= buf_end) delete [] src;
    src = cur;
    cur += len + 1;
}
inline void string_up(char *&cur, char *&src) {
    int len = strlen(cur);
    src = cur;
    cur += len + 1;
}

void RobotTxt::PackRules(char const* const buf_beg, char const *const buf_end) {
    size_t sz = sizeof(ui32) + 4 * sizeof(i32) + 1;
    if (HostDirective)
        sz += strlen(HostDirective);
    int i;
    for (i = 0; i < NR; i++)
        sz += strlen(Rules[i]) + 1;
    for (i = 0; i < NRR; i++)
        sz += strlen(RevRules[i]) + 1;
    delete [] PackedRules;
    PackedRules = new char[sz];
    char *cur = PackedRules;
    *(ui32*)cur = sz;         cur += sizeof(ui32);
    *(i32*)cur  = -2;         cur += sizeof(i32); // Version
    *(i32*)cur  = CrawlDelay; cur += sizeof(i32);
    *(i32*)cur  = NR;         cur += sizeof(i32);
    *(i32*)cur  = NRR;        cur += sizeof(i32);
    for (i = 0; i < NR; i++)
        string_down(cur, Rules[i], buf_beg, buf_end);
    for (i = 0; i < NRR; i++)
        string_down(cur, RevRules[i], buf_beg, buf_end);
    if (HostDirective) string_down(cur, HostDirective, nullptr, nullptr);
    else *cur++ = 0;
}

void RobotTxt::Load(const char *saved_data) {
    if (!saved_data || saved_data == PackedRules) return;
    Clear(nullptr, nullptr);
    size_t sz = *(ui32*)saved_data;
    PackedRules = new char[sz];
    memcpy(PackedRules, saved_data, sz);
    char *cur = PackedRules;
    cur += sizeof(ui32);
    if (*(i32*)cur == -2) {
        cur += sizeof(i32);
        CrawlDelay = *(i32*)cur; cur += sizeof(i32);
    }
    MaxNR = NR   = *(i32*)cur;   cur += sizeof(i32);
    MaxNRR = NRR = *(i32*)cur;   cur += sizeof(i32);
    Rules =    NR > 0?  (char**)malloc(sizeof(char*) * NR) : nullptr;
    RevRules = NRR > 0? (char**)malloc(sizeof(char*) * NRR) : nullptr;
    int i;
    for (i = 0; i < NR; i++)
        string_up(cur, Rules[i]);
    for (i = 0; i < NRR; i++)
        string_up(cur, RevRules[i]);
    string_up(cur, HostDirective);
    if (!*HostDirective) HostDirective = nullptr;
}

inline void CheckMaxNR(char **&Rules, int NR, int &MaxNR) {
    if (NR >= MaxNR) {
        MaxNR += 50;
        Rules = (char **) realloc(Rules, MaxNR*sizeof(char*));
    }
}

inline char *put_or_allocate(const char *str, int len, char *&buf_start, char const *const buf_end) {
    char *dst;
    if (buf_end - buf_start < len + 1) {
        dst = new char[len + 1];
    } else {
        dst = buf_start;
        buf_start += len + 1;
    }
    memcpy(dst, str, len);
    dst[len] = 0;
    return dst;
}

void RobotTxt::Add(const char* Dir, int len, char *&buf_start, char const *const buf_end)
{
    if (len > 0 && *Dir == '*') {
        CheckMaxNR(RevRules, NRR, MaxNRR);
        RevRules[NRR] = put_or_allocate(Dir + 1, len - 1, buf_start, buf_end);
        NRR++;
        return; //
    }
    if (len > 0 && *Dir != '/') return;
    CheckMaxNR(Rules, NR, MaxNR);
    Rules[NR] = put_or_allocate(Dir, len, buf_start, buf_end);
    NR++;
}

class linereader {
public:
    linereader(FILE *f) :     in(f), flag(0), hascont(0), hascomm(0), unget(0) {}
    linereader(std::istream *f) : ins(f), flag(1), hascont(0), hascomm(0), unget(0) {}
    linereader(IInputStream *f)  : inpS(f), flag(3), hascont(0), hascomm(0), unget(0) {}
    int getline(char **ptr);
    int skipblanks(char **ptr);
    int hascomment() { return hascomm; }

protected:
    int getln(int i = 0);

    union {
        FILE *in;
        std::istream *ins;
        IInputStream *inpS;
    };
    int flag;
    int hascont, hascomm, unget;
    char buffer[FILENAME_MAX+1];

    int nextch() {
        if (flag == 3) { // we need use IInputStream::ReadLine
            char c;
            if (0 == inpS->Load(&c, 1))
                return EOF;
            return c;
        }
        return (flag == 1) ? ins->get() : fgetc(in);
    }
};

int linereader::getln(int i)
{
    int c = 0;

    if (unget) {
        buffer[i++] = unget;
        unget = 0;
    }
    while(
        i < FILENAME_MAX
        && (c = nextch()) != EOF
        && c != '\r'
        && c != '\n'
    )
        buffer[i++] = c;
    buffer[i] = 0;

    switch(c) {
    case '\n':
        hascont = 0;
        break;
    case '\r':
        hascont = 0;
        if ((unget = nextch()) == '\n')
            unget = 0;
        break;
    case EOF:
        hascont = 0;
        if (!i) return 0;
        break;
    default:
        hascont = 1;
    }
    return 1;
}

int linereader::getline(char **ptr)
{
    *ptr = buffer;

    // skip rest of previous line
    while(hascont)
        getln();

    if (!getln())
        return 0;

    char *pc = strchr(buffer, '#');
    if (pc) {
        *pc = 0;
        hascomm = 1;
    }
    else
        hascomm = 0;
    return 1;
}


int linereader::skipblanks(char **ptr)
{
    memmove(buffer, *ptr, strlen(*ptr)+1);
    char *pc;

    while (!*(pc = buffer + strspn(buffer, " \t")) && hascont)
        getln();

    if (*pc == '#') {
        hascomm = 1;
        *pc = 0;
        *ptr = pc;
        return 1;
    }
    else if (!hascont) {
        *ptr = pc;
        return 1;
    }
    else {
        int i = strlen(pc);
        memmove(buffer, pc, i);
        getln(i);
        *ptr = buffer;
        return 1;
    }
}

void RobotTxt::Parse(FILE *in) {
    linereader lr(in);
    Parse(lr, nullptr, 0);
}

void RobotTxt::Parse(linereader &lr, const char *hostname) {
    const char *p = hostname? strchr(hostname, ':') : nullptr;
    int port = 80;
    if (p) {
        port = atoi(p + 1);
        if (!port) port = 80; // fix
        char *h = (char*)alloca(p - hostname + 1);
        memcpy(h, hostname, p - hostname);
        h[p - hostname] = 0;
        hostname = h;
    }
    Parse(lr, hostname, port);
}

void RobotTxt::Parse(std::istream *ins, const char *hostname) {
    linereader lr(ins);
    Parse(lr, hostname);
}

void RobotTxt::Parse(std::istream *ins, const char *hostname, int port) {
    linereader lr(ins);
    if (strchr(hostname, ':'))
        abort();
    Parse(lr, hostname, port);
}

void RobotTxt::Parse(IInputStream *ins, const char *hostname) {
    linereader lr(ins);
    Parse(lr, hostname);
}

void RobotTxt::Parse(IInputStream *ins, const char *hostname, int port) {
    linereader lr(ins);
    if (strchr(hostname, ':'))
        abort();
    Parse(lr, hostname, port);
}

void RobotTxt::Parse(linereader &reader, const char *Host, int Port)
{
    static const char agent[] = "user-agent:";
    static const char disallow[] = "disallow:";
    static const char host[] = "Host:";
    static const char crawldelay[] = "Crawl-delay:";
    static const int agentlen = strlen(agent);
    static const int disallowlen = strlen(disallow);
    static const int hostlen = strlen(host);
    static const int crawldelaylen = strlen(crawldelay);
    int wasast = 0, wasmy = 0, my = 0, inrecord = 0, inrules = 0;
    int n, k;
    char *buf;
    bool badhost = false, goodhost = false;
    char alloca_buf[MAX_ALLOCA];
    char *alloca_buf_cur = alloca_buf, *alloca_buf_end = alloca_buf + MAX_ALLOCA;

    Clear(nullptr, nullptr);
    while((reader.getline(&buf))) {
        // detect blank lines
        if (!*buf || *buf == ' ' || *buf == '\t') {
            reader.skipblanks(&buf);
            if (!reader.hascomment() && *buf == 0)
                my = inrecord = inrules = 0;
            continue;
        }
        // search for myagent
        if (!inrules && !strnicmp(agent, buf, agentlen)) {
            inrecord = 1;
            buf += agentlen;
            if (my && wasmy)
                continue;
            reader.skipblanks(&buf);
            buf = strlwr(buf);
            if (!wasmy && !wasast && *buf == '*')
                wasast = my = 1;
            else if (strstr(buf, Agent) != nullptr) {
                wasmy = my = 1;
                if (wasast == 1) {
                    Clear(alloca_buf, alloca_buf_end);
                    alloca_buf_cur = alloca_buf;
                    badhost = false, goodhost = false;
                    wasast = 2;
                }
            }
            continue;
        }
        // add rules
        if (inrecord && !strnicmp(disallow, buf, disallowlen)) {
            inrules = 1;
            if (!my)
                continue;
            buf += disallowlen;
            reader.skipblanks(&buf);
            if (*buf)
                Add(buf,strcspn(buf," \t"), alloca_buf_cur, alloca_buf_end);
            else {
                Clear(alloca_buf, alloca_buf_end);
                alloca_buf_cur = alloca_buf;
                badhost = false, goodhost = false;
            }
        // host directive
        } else if (inrecord && !strnicmp(host, buf, hostlen)) {
            inrules = 1;
            if (!my)
                continue;
            buf += hostlen;
            reader.skipblanks(&buf);
            if (*buf) {
                char *p = buf + strcspn(buf," \t#:/");
                int port = 80;
                if (*p == ':') {
                    *p++ = 0;
                    port = strtol(p, &p, 10);
                }
                if (!port || *p == '/') //port zero is not allowed, / is not allowed
                    continue;
                char *s = !*p? p : p + strspn(p, " \t");
                if (*s && *s != '#')
                    continue;
                *p = 0;
                strlwr(buf);
                if (*buf == '-' || *buf == '.' || !*buf)
                    continue;
                char *lastdot = nullptr;
                for (p = buf; *p; p++) { //RFC check
                    if (!isalnum((ui8)*p) && *p != '-' && *p != '.' || (*p == '-' && (p[1] == '.' || p[-1] == '.' || !p[1])) || (*p == '.' && p[1] == '.'))
                        break;
                    if (*p == '.')
                        lastdot = p;
                }
                if (*p || (lastdot && !NTld::IsTld(lastdot + 1)))
                    continue;
                if (Host && *Host) {
                    badhost = stricmp(Host, buf) || Port != port;
                    if (!badhost)
                        goodhost = true;
                }
                if (!HostDirective)
                    strcpy(HostDirective = new char[strlen(buf) + 1], buf);
            }
        // crawl-delay directive
        } else if (inrecord && !strnicmp(crawldelay, buf, crawldelaylen)) {
            CrawlDelay = atoi(buf + crawldelaylen);
            if (CrawlDelay < 0) CrawlDelay = 0;
        }
    }
    bool disallow_all = badhost && !goodhost;
    Sort();
    if (NR > 0 && **Rules == '/' && !Rules[0][1] || NRR > 0 && !**RevRules)
        disallow_all = true;

    if (disallow_all) {
        char *h = HostDirective; HostDirective = nullptr; //save
        Clear(alloca_buf, alloca_buf_end);
        HostDirective = h; //restore
        NR = -1;
    }

    if (NR > 1) {
        for (n = 1, k = 0; n < NR; n++) {
            if (!strncmp(Rules[n], Rules[k], strlen(Rules[k]))) {
                if (Rules[n] < alloca_buf || Rules[n] >= alloca_buf_end) delete[] Rules[n];
            } else {
                Rules[++k] = Rules[n];
            }
        }
        NR = k + 1;
    }
    if (NRR > 1) {
        for (n = 1, k = 0; n < NRR; n++) {
            if (!str_end_cmp(RevRules[n], RevRules[k])) {
                if (RevRules[n] < alloca_buf || RevRules[n] >= alloca_buf_end) delete[] RevRules[n];
            } else {
                RevRules[++k] = RevRules[n];
            }
        }
        NRR = k + 1;
    }
    PackRules(alloca_buf, alloca_buf_end);
    alloca_buf_cur = alloca_buf;
}
