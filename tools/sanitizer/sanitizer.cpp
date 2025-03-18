#include <yweb/webxml/websan.h>
#include <yweb/webxml/webxml.h>

#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/codepage.h>
#include <util/memory/blob.h>
#include <util/string/split.h>

#include <fstream>
#include <stdio.h>

#ifndef _WIN32
#   include <unistd.h>
#endif

using namespace std;

static bool doOne(const char*     file_from,
                  const char*     file_to,
                  const char*     url_base,
                  ECharset cp,
                  TString*         outBodyTag,
                  TString*         outTitle,
                  int             lineLimit)
{
    if(file_from && file_to)
    {
        cerr << "Processing file " << file_from << " to "
             << file_to << " ..." << endl;
    }
    else
    {
        if(file_from)
            cerr << "Processing file " << file_from << "..." << endl;
        else
            cerr << "Processing..." << endl;
    }

    ostream* fout = &cout;

    if(file_to)
        fout = new ofstream(file_to, ios::out|ios::trunc|ios::binary);

    if(fout->bad())
    {
        cerr << "File problems: " << file_to << endl;
        if(file_to)
            delete fout;
        return false;
    }

    TBlob inp = TBlob::FromFile(file_from);
    NHtmlLexer::TObsoleteLexer lx(inp.AsCharPtr(), inp.Size());
    wxSanitizerStream st(lx, hxMakeAttributeChecker(url_base), *fout, cp, lineLimit);

    if(lx.good() && st.good())
    {
        bool ret = st.doIt(true, outBodyTag, outTitle);
        cerr << "...done(" << ret << ")" << endl;
        if(file_to)
            delete fout;
        if(st.isFailed())
        {
            cerr << "!sanitize problems!" << endl;
            ret = false;
        }

        return ret;
    }
    if(file_to)
        delete fout;

    cerr << "File problems" << endl;
    return false;
}

static void workOne(const char*     file_from,
                    const char*     file_to,
                    const char*     url_base,
                    ECharset cp,
                    TString&         tmpFile,
                    int             lineLimit)
{
    if(!tmpFile)
    {
        doOne(file_from, file_to, url_base, cp, nullptr, nullptr, lineLimit);
        return;
    }
    TString bodyTag;
    TString title;
    if(!doOne(file_from, tmpFile.c_str(), url_base, cp,
              &bodyTag, &title, lineLimit))
        return;

    ifstream fin(tmpFile.c_str(), ios::in|ios::binary);
    if(fin.bad())
    {
        cerr << "File problems: " << tmpFile.c_str() << endl;
        return;
    }

    ostream* fout = &cout;

    if(file_to)
        fout = new ofstream(file_to, ios::out|ios::trunc|ios::binary);

    if(fout->bad())
    {
        cerr << "File problems: " << file_to << endl;
        if(file_to)
            delete fout;
        return;
    }

    HXSanitizer::formDocument(fin, bodyTag, title, *fout);

    if(file_to)
    {
        delete fout;
    }
}


static int usage(char** argv)
{
    cerr << "Usage :" << endl;
    cerr << "\t" << argv[0]
         << " [-c codePage] [-t temp_file] [-b url_base] [-n line_limit] in_file > out_file" << endl;
    cerr << "or" << endl;
    cerr << "\t" << argv[0]
         << " [-c codePage] [-t temp_file] [-b url_base] [-n line_limit] -o out_dir <files>" << endl;
    return 1;
}

int main(int argc, char** argv)
{
    ECharset cp = CODES_UNKNOWN;
    TString dir_to = nullptr;
    TString tmpFile = nullptr;
    TString urlBaseValue = nullptr;
    const char* url_base = nullptr;
    int line_limit = 0;
    int opt;
    Opt getOpt(argc, argv, "c:t:b:n:o:");

    while ( (opt = getOpt.Get()) != EOF )
    {
        switch (opt) {
            case 'c':
                cp = CharsetByName(getOpt.Arg);
                if(cp == CODES_UNKNOWN)
                {
                    cerr << "Wrong codepage " << getOpt.Arg << endl;
                    return 1;
                }
                break;
            case 't':
                tmpFile = getOpt.Arg;
                break;
            case 'b':
                urlBaseValue = getOpt.Arg;
                url_base = urlBaseValue.c_str();
                break;
            case 'n':
                line_limit = atoi(getOpt.Arg);
                break;
            case 'o':
                dir_to = getOpt.Arg;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if(!dir_to)
    {
        if(argc - getOpt.Ind !=1)
        {
            usage(argv);
            return 1;
        }

        workOne(argv[getOpt.Ind], nullptr, url_base, cp, tmpFile, line_limit);
        return 0;
    }

    for(int count=0; ; count++)
    {
        if(getOpt.Ind>=argc)
        {
            if(count==0)
            {
                usage(argv);
                return 1;
            }
            break;
        }
        TString fFrom = argv[getOpt.Ind++];
        TVector<TString> flds;
        int idx = Split(fFrom.c_str(), ":/\\", flds);
        TString fTo = dir_to + "/" + flds[idx-1];
        workOne(fFrom.c_str(), fTo.c_str(), url_base, cp, tmpFile, line_limit);
    }

    return 0;
}
