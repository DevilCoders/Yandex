#include <string.h>

#include <util/generic/algorithm.h>
#include <util/system/maxlen.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <utility>

#include <kernel/urlnorm/urlnorm.h>

typedef unsigned char uchar;

#define MAXPARMS   17
#define MAXDIRS    15
#define MAXCOLZ    5
#define MINDIRCHK  (MAXCOLZ+1)

#define FNV64INIT  ULL(14695981039346656037)
#define FNV64PRIME ULL(1099511628211)
#define FNV32PRIME ULL(16777619)
#define MORDAHASH  (FNV64INIT * FNV64PRIME ^ (uchar)'/')

//void PR(char t, const char *b, const char *e) {
//    fprintf(stderr, "%c{%.*s}\n", t, e-b, b);
//}

struct Parm {
    char *Beg, *Eq, *End;

    struct Less {
        bool operator()(const Parm &x, const Parm &y) {
            return stricmp(x.Beg, y.Beg) < 0;
        }
    };
};


%%{

machine UrlHashCommon;

action REJECT {
    return -2;
}

action BEGPARM {
    Parms[ParmsNo].Beg = fpc;
    Parms[ParmsNo].Eq = NULL;
}

action BEGEQ {
    Parms[ParmsNo].Eq = fpc - 1;
}

action ENDPARM {
    if (ParmsNo == MAXPARMS)
        return -3;
    if (!Parms[ParmsNo].Eq)
        Parms[ParmsNo].Eq = fpc;
    Parms[ParmsNo++].End = fpc;
}

delim    =  '?'+;
str      = [^=?]*;
any_val  = ('=' [^?]*)?;
url_val  = '=' 'http://' [^?]*;
num_val  = '=' [\-.0-9A-Fa-f]{10,} [^?]*;
long_val = '=' [^?]{10,};
slash_val = '=' [^/?]* '/' [^?]*;

#=================================================
#==== Scripts ====================================
#=================================================
badscript = ( [^?]* (
    'postings.cgi'i     |
    'login.php'i        |
    'member2.php'i      |
    'sendtofriend.php'i |
    'postings.php'i     |
    'newthread.php'i    |
    'newtopic.php'i     |
    'posting.php'i      |
    'privmsg.php'i      |
    'newreply.php'i     |
    'editpost.php'i     |
    'private.php'i      |
    'ubbmisc.cgi'i
) [34]? ) %REJECT;

#=================================================
#==== Parameters =================================
#=================================================
ubb = 'ubb='i (
    'newtopic'i     |
    'reply'i        |
    'close_topic'i  |
    'transfer'i     |
    'delete_topic'i |
    'get_ip'i
);

ikonb = 'act='i (
    'msg'i  |
    'post'i |
    'poll'i
);

dirlist = [NMSDnmsd] '=' [ADad];

# these params are going to become non-fake very soon
oldfakeparm = (
    's'i              long_val |
    'oid'i             any_val |
    str 'rand'i        any_val |
    'rand'i str        any_val |
    'ids'i             any_val |
    'user'i str        any_val |
    'authid'i          any_val |
    'ref'i             any_val
);

# these narrower set of params will replace above params soon
newfakeparm = (
    's'i               num_val |
    'oid'i             num_val |
    'rand'i            any_val |
    'random'i          any_val |
    'ids'i             num_val |
    'authid'i          num_val  |
    'ref'i             slash_val
);

fakeparm = (
    dirlist                    |
    str 'sid'i        long_val |
    's_id'i            any_val |
    'ses'i             any_val |
    str 'session'i str any_val |
    str 'sessid'i  str any_val |
    str 'sess'i        any_val |
    'nc'i              num_val |
    'no_cache'i        any_val |
    'nocache'i         any_val |
    'expandsection'i   any_val |
    str 'rnd'i         any_val |
    'rnd'i str         any_val |
    'test'i            any_val |
    'staz'i            any_val |
    'tophits'i         any_val |
    'kk'i              any_val |

    str 'basket'i      num_val |
    'thsh'i            any_val |
    'reid'i            any_val |
    'pairslogin'i      any_val |
    'cart_id'i         any_val |
    'back_url'i        any_val |
    'referer'i         any_val |
    'redirect'i        any_val |
    'href'i            any_val
);

badparm = (
    ubb      |
    ikonb
) %REJECT;

#=================================================
#==== Grammar ====================================
#=================================================
anyparm = any+ & [^=?]* ('=' %BEGEQ)? [^?]*;

goodscript = (
    [^?]+ - badscript
);

script = badscript | goodscript;

}%%

%%{

machine UrlHashOld;
include UrlHashCommon;

goodparm = (
    anyparm - (fakeparm | oldfakeparm | badparm)
) >BEGPARM %ENDPARM;

parm   = goodparm | fakeparm | oldfakeparm | badparm;

main  := script? delim (parm delim)*;
write data noerror;

}%%

class TUrlHashRunnerOld {
public:
    static int Run(char* url, int len, Parm* Parms, int& ParmsNo) {
        int cs = 0;
        char* p = url;
        char* pe = p + len;

        %% write init;
        %% write exec;

        if (cs < UrlHashOld_first_final)
            return -1;

        return 0;
    }
};

%%{

machine UrlHash;
include UrlHashCommon;

goodparm = (
    anyparm - (fakeparm | newfakeparm | badparm)
) >BEGPARM %ENDPARM;

parm   = goodparm | fakeparm | newfakeparm | badparm;

main  := script? delim (parm delim)*;
write data noerror;

}%%

class TUrlHashRunner {
public:
    static int Run(char* url, int len, Parm* Parms, int& ParmsNo) {
        int cs = 0;
        char* p = url;
        char* pe = p + len;

        %% write init;
        %% write exec;

        if (cs < UrlHash_first_final)
            return -1;

        return 0;
    }
};


static inline char d2x(unsigned x) {
    return (char)((x < 10) ? ('0'+x) : ('A'+x-10));
}

void EscapeAppend(TString *norm, unsigned char ch) {
    if (!norm) {
        return;
    }
    if (ch <= ' ' || ch == '%') {
        norm->append('%');
        norm->append( d2x(ch>>4) );
        norm->append( d2x(ch&0xF) );
    }
    else {
        norm->append( ch );
    }
}

template<typename TRunner>
int CalcUrlHash(char* url, int len, ui64& fnv, TString *norm) {
    Parm Parms[MAXPARMS+1];
    int ParmsNo = 0;
    char sep = '?';

    int res = TRunner::Run(url, len, Parms, ParmsNo);
    if (res)
        return res;

    for (int i = 0; i < ParmsNo; i++)
        *Parms[i].Eq = 0;
    StableSort(Parms, Parms + ParmsNo, Parm::Less());
    for (int i = 0; i < ParmsNo; i++) {
        if (i < ParmsNo - 1 && !Parm::Less()(Parms[i], Parms[i+1]))
            continue;
        *Parms[i].Eq = '=';
        if (norm) {
            EscapeAppend(norm, sep);
            sep = '&';
        }
        fnv *= FNV64PRIME;
        for (char *p = Parms[i].Beg; p < Parms[i].End; ++p) {
            fnv = fnv * FNV64PRIME ^ (uchar)*p;
            if (norm) {
                EscapeAppend(norm, *p);
            }
        }
    }
    return 0;
}

unsigned char ParamSeparators[] = {'?', '&', ';', ','};

void ApplySeparators(unsigned char* buf) {
    for (unsigned i = 0; i < sizeof(ParamSeparators); i++)
        buf[(ui8)ParamSeparators[i]] = '?';
}

void FillIdent(unsigned char* buf) {
    for (unsigned i = 0; i < 256; i++)
        buf[i] = i;
}

static const unsigned char *_TolowerNormBuilder() {
    static unsigned char buf[256];
    FillIdent(buf);
    for (unsigned i = 'A'; i <= 'Z'; i++) buf[i] = i + ('a'-'A');
    buf[(ui8)'+'] = '\x20';
    ApplySeparators(buf);
    return buf;
}

static const unsigned char *_NormBuilder() {
    static unsigned char buf[256];
    FillIdent(buf);
    buf[(ui8)'+'] = '\x20';
    ApplySeparators(buf);
    return buf;
}

static const unsigned char *_TolowerEscapeBuilder() {
    static unsigned char buf[256];
    FillIdent(buf);
    for (unsigned i = 'A'; i <= 'Z'; i++) buf[i] = i + ('a'-'A');
    ApplySeparators(buf);
    return buf;
}

static const unsigned char *_EscapeBuilder() {
    static unsigned char buf[256];
    FillIdent(buf);
    ApplySeparators(buf);
    return buf;
}

static const unsigned char *_HexBuilder() {
    static unsigned char buf[256];
    unsigned i;
    memset(buf, 0, sizeof(buf));
    for (i = '0'; i <= '9'; i++) buf[i] = i - '0';
    for (i = 'A'; i <= 'F'; i++) buf[i] = i + 10 - 'A';
    for (i = 'a'; i <= 'f'; i++) buf[i] = i + 10 - 'a';
    return buf;
}

static const unsigned char
    *_TolowerNorm = _TolowerNormBuilder(),
    *_Norm = _NormBuilder(),
    *_TolowerEscape = _TolowerEscapeBuilder(),
    *_Escape = _EscapeBuilder(),
    *_Hex = _HexBuilder();

struct dirless {
    bool operator()(const char *x, const char *y) {
        return strcmp(x, y) < 0;
    }
};

// copies path to url, lowercasing if needed
// can work with borh NULL-terminated strings (pend is NULL), and ranges (path, pend)

char* UrlToLower(char* url, const char* path, const char* pend, bool caseSensitive) {
    const unsigned char* Norm = caseSensitive ? _Norm : _TolowerNorm;
    char* urlend = url + URL_MAX;
    const char* sptr = path;
    char* dptr = url;
    if (!pend)
        pend = path + 2 * URL_MAX;

    while (dptr < urlend && sptr < pend && (*dptr++ = Norm[(uchar)*sptr++])) {}

    return dptr;
}

char* UrlUnescape(char* url, char* end, bool caseSensitive) {
    char *sptr, *dptr;
    const unsigned char* Escape = caseSensitive ? _Escape : _TolowerEscape;

    dptr = end;
    if ((sptr = strchr(url, '%')) && sptr < end) {
        *dptr++ = 0;
        *dptr++ = 0; // do not break at url = ...%\0
        dptr = sptr;
        while ((*dptr = *sptr++)) {
            if (*dptr == '%') {
                *dptr = Escape[(_Hex[(uchar)*sptr]<<4) + _Hex[(uchar)*(sptr+1)]];
                sptr += 2;
            }
            dptr++;
        }
    } else {
        dptr--; // dptr points at nil in url
    }

    return dptr;
}

template<typename TRunner>
int UrlHashValImpl(ui64 &fnv, const char* path, bool caseSensitive, TString* norm) {
    char url[URL_MAX+12], *sptr, *dptr, *ptr;
    char* urlend = url + URL_MAX;

    dptr = UrlToLower(url, path, NULL, caseSensitive);

    // if url is too long
    if (dptr == urlend) {
        return 1;
    }

    dptr = UrlUnescape(url, dptr, caseSensitive);

    if ((sptr = strchr(url, '?')))
        *sptr = 0;
    else
        sptr = dptr;

    // check max slashes
    int slashes = 0;
    char *dirs[MAXDIRS], *lastdir;
    for (ptr = url; (ptr = strchr(ptr, '/')); ptr = dirs[slashes++] = ptr + 1)
        if (slashes == MAXDIRS) {
            fnv = 0;
            return 1;
        }
    if (slashes < 1) {
        fnv = FNV64INIT;
        return 0;
    }
    lastdir = dirs[slashes-1];

    // compute dir FNV
    for (ptr = url, fnv = FNV64INIT; ptr < sptr; ++ptr) {
        if (norm) {
            EscapeAppend(norm, *ptr);
        }
        fnv = fnv * FNV64PRIME ^ (uchar)*ptr;
    }

    // check duplicated dirs
    if (slashes >= MINDIRCHK) {
        int i, col = 0;
        for (i = 0; i < slashes; i++)
            *(dirs[i]-1) = 0;
        Sort(dirs, dirs + slashes, dirless());
        for (i = 1; i < slashes; i++)
            if (!dirless()(dirs[i - 1], dirs[i]))
                col++;
        if (col >= MAXCOLZ) {
            fnv = 0;
            return 2;
        }
    }

    // compute script FNV
    if (sptr != dptr) {
        *sptr = *dptr = '?';
        if (CalcUrlHash<TRunner>(lastdir, dptr + 1 - lastdir, fnv, norm)) {
            fnv = 0;
            return 3;
        }
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4307) /* interger overflow in the MORDAHASH definition */
#endif
    if (fnv == MORDAHASH) {
        fnv = ULL(1);
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    return 0;
}

int UrlHashValOld(ui64 &fnv, const char* path, bool caseSensitive, TString* norm) {
    return UrlHashValImpl<TUrlHashRunnerOld>(fnv, path, caseSensitive, norm);
}

int UrlHashVal(ui64 &fnv, const char* path, bool caseSensitive, TString* norm) {
    return UrlHashValImpl<TUrlHashRunner>(fnv, path, caseSensitive, norm);
}

int UrlHashValCaseInsensitive(ui64& fnv, const char* path, TString* norm) {
   return UrlHashValImpl<TUrlHashRunner>(fnv, path, false, norm);
}

////////////////////////////////////////////////
/// \brief Calculates normalized form of URL
///
/// This function transforms URL for the purpose
/// of computing unique URLID by stripping parameters
/// that are not significant (sessionids) and sorts
/// dynamic arguments alphanumerically.
int UrlNormId(const char *url, TString &urlid) {

    // skip http:// and https:// for sure
    if (strlen(url) < 8) {
        return -1;
    }

    // find path separator
    const char *path = strchr(url + 8, '/');
    if (path == NULL) {
        return -1;
    }

    // put hostname in place
    urlid.append(url, path-url);

    ui64 fnv = 0;
    if (UrlHashVal(fnv, path, true, &urlid)) {
        urlid.clear();
        return -1;
    }

    return 0;
}

%%{
# this machine checks if this param was fake before and now
machine CheckFake;
include UrlHashCommon;

action OLDFAKE {
    oldFake = true;
}

action NOTOLDFAKE {
    oldFake = false;
}

action NEWFAKE {
    newFake = true;
}

action NOTNEWFAKE {
    newFake = false;
}

main := oldfakeparm %~OLDFAKE %*NOTOLDFAKE | newfakeparm %~NEWFAKE %*NOTNEWFAKE;

write data noerror;

}%%

bool ShouldRemoveParam(char* p, char* pe) {
    bool oldFake = false, newFake = false;
    int cs = 0;

    %% write init;
    %% write exec;

    return oldFake && !newFake;
}

void SplitQueryByDelims(char* query, TVector<char*>& offsets) {
    for (size_t i = 0; i < sizeof(ParamSeparators); i++) {
        uchar delim = ParamSeparators[i];
        char* p = query - 1;
        while ((p = strchr(p + 1, delim)))
            offsets.push_back(p);
        char edelim[4] = "%00";
        edelim[1] = tolower(d2x(delim >> 4));
        edelim[2] = tolower(d2x(delim & 0xF));
        p = query - 1;
        while ((p = strcasestr(p + 1, edelim)))
            offsets.push_back(p);
    }
    offsets.push_back(query + strlen(query));
    std::sort(offsets.begin(), offsets.end());
}

typedef std::pair<char*, char*> TRange;
typedef TVector<TRange> TRangeVector;

void FindNonFakeParams(TVector<char*>& offsets, TRangeVector& delRanges) {
    for (TVector<char*>::iterator it = offsets.begin(); it != offsets.end(); ++it) {
        TVector<char*>::iterator next = it + 1;
        if (next == offsets.end())
            break;
        char* begin = *it;
        size_t delimLen = (*begin == '%') ? 3 : 1;
        begin += delimLen;
        char* end = *next;
        char param[URL_MAX];
        char* pend = UrlToLower(param, begin, end, false);
        pend = UrlUnescape(param, pend, false);
        if ((end - begin) - 1 == (pend - param)) {
            pend += 1; // this is needed because of a bug in UrlUnescape, fixing the bug will cause some hashes to change
        }
        if (ShouldRemoveParam(param, pend))
            delRanges.push_back(TRange(begin - delimLen, end));
    }
}

void DeleteRanges(char* path, size_t& plen, TRangeVector& delRanges) {
    for (TRangeVector::reverse_iterator it = delRanges.rbegin(); it != delRanges.rend(); ++it) {
        char* begin = (*it).first;
        char* end = (*it).second;
        memmove(begin, end, path + plen - end + 1);
        plen -= (end - begin);
    }
}
