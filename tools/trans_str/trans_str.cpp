#include <cstdio>

#include <util/stream/output.h>
#include <util/string/strip.h>
#include <util/generic/string.h>

#include "trans_str.h"

/********************************************************/
//  Internal definitions
/********************************************************/

class tsBasePattern
{
    protected:
        regex_t mRegex;
        bool    mFinal;
        bool    mOK;
        bool    mDebug;
        int     mNLine;

        bool match  (TString str);
        bool replace(TString&     str,
                     const char* replacement,
                     int*        position);

    public:
        tsBasePattern(const char*          regex,
                      int                  cflags,
                      bool                 final,
                      bool                 debug,
                      int                  nline,
                      tsStringTransformer* transf);
        virtual ~tsBasePattern();

        virtual bool process(TString& str, bool& final) = 0;

        bool    isOK()    { return mOK; }
        bool    isDebug() { return mDebug; }
        bool    isFinal() { return mFinal; }

};

/********************************************************/
class tsReplacePattern: public tsBasePattern
{
    protected:
        TString              mReplacement;
        int                 mRecoursion;

    public:
        tsReplacePattern(const char*          regex,
                         const char*          replacement,
                         int                  recoursion,
                         int                  cflags,
                         bool                 final,
                         bool                 debug,
                         int                  nline,
                         tsStringTransformer* transf);
        ~tsReplacePattern() override;

        bool process(TString& str, bool& final) override;
};

/********************************************************/
class tsSelectPattern: public tsBasePattern
{
        friend class tsStringTransformer;
    protected:
        TVector<tsReplacePattern*> mReplacements;

    public:
        bool process(TString& str, bool& final) override;

        tsSelectPattern(const char*          regex,
                        int                  cflags,
                        bool                 final,
                        bool                 debug,
                        int                  nline,
                        tsStringTransformer* transf);
        ~tsSelectPattern() override;
};

/********************************************************/
/********************************************************/
tsBasePattern::tsBasePattern(const char*          regex,
                             int                  cflags,
                             bool                 final,
                             bool                 debug,
                             int                  nline,
                             tsStringTransformer* transf)
{
    int ret = regcomp(&mRegex, regex, cflags);
    mOK     = !ret;
    mDebug  = debug;
    mNLine  = nline;
    mFinal  = final;

    if(ret!=0)
    {
        char buf[1000];
        sprintf(buf, "Failure on regex compilation: [%s]\n", regex);
        transf->Log(buf);
    }
}

/********************************************************/
tsBasePattern::~tsBasePattern()
{
    regfree(&mRegex);
}

/********************************************************/
bool tsBasePattern::match  (TString str)
{
    if(!mOK)
        return false;
    regmatch_t m;

    int ret = regexec(&mRegex, str.c_str(), 1, &m, 0);

    if(isDebug() && ret == 0)
    {
        Cout << "Pattern on line " << mNLine << " matches:" << Endl;
        Cout << "\t>>" << str << Endl;
        if(isFinal())
            Cout << "***Final match***" << Endl;
    }

    return (ret == 0);
}

/********************************************************/
bool tsBasePattern::replace
(TString& str, const char* replacement, int* position)
{
    if(!mOK)
        return false;

    regmatch_t m;
    int pos = 0;
    if(position)
    {
        if( (int)(str.length()) < (*position) )
            return false;
        pos = *position;
    }


    int ret = regexec(&mRegex, str.c_str() + pos, 1, &m, 0);
    if (ret != 0 || m.rm_so<0)
        return false;

    if(isDebug())
    {
        Cout << "Pattern on line " << mNLine << " replacement:" << Endl;
        Cout << "\t>>>" << str << Endl;
        if(isFinal())
            Cout << "***Final operation***" << Endl;
    }

    TString str1 = str;
    str1 = str1.replace(pos + m.rm_so, m.rm_eo-m.rm_so, replacement);

    if(position)
    {
        *position = (int)(pos + m.rm_so + strlen(replacement));
    }

    if(isDebug())
    {
        Cout << "\t<<<" << str1 << Endl;
    }

    if(str == str1)
    {
        if(isDebug())
            Cout << "\tcycle of no changes happens, broken" << Endl;
        return false;
    }
    str = str1;

    return true;
}

/********************************************************/
/********************************************************/
tsReplacePattern::tsReplacePattern
    (const char*          regex,
     const char*          replacement,
     int                  recoursion,
     int                  cflags,
     bool                 final,
     bool                 debug,
     int                  nline,
     tsStringTransformer* transf):
        tsBasePattern(regex, cflags, final, debug, nline, transf),
        mReplacement (replacement),
        mRecoursion  (recoursion)
{
}

/********************************************************/
tsReplacePattern::~tsReplacePattern()
{
}

/********************************************************/
bool tsReplacePattern::process(TString& str, bool& final)
{
    if(!isOK() || final)
        return false;


    bool ret0 = false;
    bool ret;
    int  position = 0;
    int* pos = nullptr;

    if(mRecoursion == 1)
        pos = &position;

    do
    {
        ret = replace(str, mReplacement.c_str(), pos);
        ret0 |= ret;
    } while(ret && mRecoursion);

    final = isFinal();
    return ret0;
}

/********************************************************/
/********************************************************/
tsSelectPattern::tsSelectPattern
    (const char*          regex,
     int                  cflags,
     bool                 final,
     bool                 debug,
     int                  nline,
     tsStringTransformer* transf):
        tsBasePattern(regex, cflags|REG_NOSUB, final, debug, nline, transf),
        mReplacements()
{
}

/********************************************************/
tsSelectPattern::~tsSelectPattern()
{
    for(unsigned i=0; i<mReplacements.size(); i++)
        delete mReplacements[i];
}

/********************************************************/
bool tsSelectPattern::process(TString& str, bool& final)
{
    if(final || !match(str))
        return false;

    bool ret = false;
    for(unsigned i=0; !final && i<mReplacements.size(); i++)
    {
        ret |= mReplacements[i]->process(str, final);
    }
    final |= isFinal();
    return ret;
}

/********************************************************/
/********************************************************/
tsStringTransformer::tsStringTransformer():
    mPatterns(),
    mOK      (true)
{
}

/********************************************************/
tsStringTransformer::~tsStringTransformer()
{
    for(unsigned i=0; i<mPatterns.size(); i++)
        delete mPatterns[i];
}

/********************************************************/
void tsStringTransformer::Log(const char* msg)
{
    Cerr << msg;
}

/********************************************************/
bool tsStringTransformer::process(TString& str)
{
    if(!mOK)
        return false;

    bool ret   = false;
    bool final = false;
    for(unsigned i=0; i<mPatterns.size() && !final; i++)
    {
        ret |= mPatterns[i]->process(str, final);
    }
    return ret;
}

/********************************************************/
void tsStringTransformer::badLine
    (const char* fName,
     int nLine,
     const char* rep)
{
    char buf[1000];
    if(rep)
        snprintf(buf, 1000, "tsStringTransformer: bad line in file %s line %d\n\t%s\n",
                fName, nLine, rep);
    else
        snprintf(buf, 1000, "tsStringTransformer: bad line in file %s line %d\n      ",
                fName, nLine     );
    Log(buf);
    mOK = false;
}

/********************************************************/
void tsStringTransformer::addSelect
    (const char* opt,
     int         cflags,
     bool        final,
     const char* fName,
     int         nLine,
     bool        debug)
{
    if(mPatterns.size()>0)
    {
        if(mPatterns.back()->mReplacements.size()==0 &&
           !mPatterns.back()->isFinal())
        {
            badLine(fName, nLine,
                    "No replacement pattern for previous selection pattern");
            return;
        }
    }

    tsSelectPattern* sp =
        new tsSelectPattern(opt, cflags, final, debug, nLine, this);

    if(sp->isOK())
    {
        mPatterns.push_back(sp);
    }
    else
    {
        badLine(fName, nLine, "Bad pattern");
        delete sp;
    }
}

/********************************************************/
void tsStringTransformer::addReplace
    (const char* opt,
     const char* replacement,
     int         recoursion,
     int         cflags,
     bool        final,
     const char* fName,
     int         nLine,
     bool        debug)
{
    if(mPatterns.size()==0)
    {
        badLine(fName, nLine,
                "No selection pattern is set");
        return;
    }

    tsReplacePattern* rp = new tsReplacePattern
        (opt, replacement, recoursion, cflags, final, debug, nLine, this);

    if(rp->isOK())
    {
        mPatterns.back()->mReplacements.push_back(rp);
    }
    else
    {
        badLine(fName, nLine, "Bad pattern");
        delete rp;
    }

}

/********************************************************/
void tsStringTransformer::finishUp(const char* fName, int nLine)
{
    if(mPatterns.size()==0)
    {
        Log("tsStringTransformer Warining: no selection pattern is set\n");
        return;
    }

    if(mPatterns.size()>0)
    {
        if(mPatterns.back()->mReplacements.size()==0 &&
           !mPatterns.back()->isFinal())
        {
            badLine(fName, nLine,
                    "No replacement pattern for previous selection pattern");
            return;
        }
    }
}

/********************************************************/
bool tsStringTransformer::split
(const char* orig,
 char*       buf,
 char**      sections,
 int         min_sections,
 int         max_sections)
{
    int n = 0;
    char* b = buf;
    const char* c = orig;
    sections[0] = buf;

    for(; *c && n<max_sections; c++)
    {
        if(*c=='\\' && c[1]=='@')
        {
            *(b++)=*(++c);
            continue;
        }
        if(*c=='@')
        {
            *(b++)=0;
            sections[++n]=b;
        }
        else
        {
            *(b++) = *c;
        }
    }
    *b = 0; n++;
    if(n < min_sections || *c)
        return false;

    for(; n>max_sections; n++)
        buf[n]=0;
    return true;
}

/********************************************************/
void tsStringTransformer::mkopts
    (char*      buf,
     int&       recoursion,
     int&       cflags,
     bool&      final)
{
    cflags = 0;
    final      = false;
    recoursion = 0;
    if(buf)
    {
        if(strchr(buf, 'i'))
            cflags |= REG_ICASE;
        if(strchr(buf, 'x'))
            cflags |= REG_EXTENDED;
        if(strchr(buf, 'f'))
            cflags |= REG_NOSUB;
        if(strchr(buf, 'n'))
            cflags |= REG_NEWLINE;
        if(strchr(buf, 's'))
            recoursion = 1;
        if(strchr(buf, 'r'))
            recoursion = 2;
        if(strchr(buf, '!'))
            final = true;
    }
}

/********************************************************/
bool tsStringTransformer::prepare(const char* fName, bool debug)
{
    if(!mOK)
        return false;

    FILE* f = fopen(fName, "r");
    if(f == nullptr)
    {
        mOK = false;
        char buf[1000];
        sprintf(buf, "tsStringTransformer: bad settings file %s\n",
                fName);
        return false;
    }

    char  buf[0x1000];
    char* sects[4];
    int   cflags;
    int   recoursion;
    bool  final;
    int   nline;
    for(nline = 1; isOK() && fgets(buf, 0x1000, f); nline++)
    {
        for(char* c = buf; *c; c++)
        {
            if(strchr("\t\r\n", *c))
                *c=' ';
        }

        TString ln(buf);
        ln = StripInPlace(ln);
        if(ln[0]=='#' || ln[0]==0)
            continue;

        if(ln[0]=='m'
           && ln[1]=='@')
        {
            if(!split(ln.c_str(), buf, sects, 2, 3))
            {
                badLine(fName, nline, "Wrong instruction sections");
                continue;
            }
            mkopts(sects[2], recoursion, cflags, final);
            addSelect(sects[1], cflags, final, fName, nline, debug);
            continue;
        }

        if(ln[0]=='s' &&
           ln[1]=='@')
        {
            if(!split(ln.c_str(), buf, sects, 3, 4))
            {
                badLine(fName, nline, "Wrong instruction sections");
                continue;
            }
            mkopts(sects[3], recoursion, cflags, final);
            addReplace(sects[1], sects[2], recoursion, cflags, final,
                       fName, nline, debug);
            continue;
        }

        badLine(fName, nline, "Unrecognized instruction");
    }

    if(isOK())
        finishUp(fName, nline);

    return isOK();
}

/********************************************************/
/********************************************************/
