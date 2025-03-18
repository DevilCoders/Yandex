#include <kernel/search_types/search_types.h>
#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/hitlist/subindex.h>
#include <kernel/keyinv/indexfile/indexfile.h>
#include <kernel/keyinv/indexfile/seqreader.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <library/cpp/svnversion/svnversion.h>

#include <library/cpp/charset/codepage.h>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/string/printf.h>
#include <util/system/compat.h>

#include <cstdio>

enum EPrintPosType {
    PPT_NO,
    PPT_POS,
    PPT_HEX_POS,
    PPT_SORT_POS,
    PPT_DOCLEN_POS,
    PPT_DOC,
    PPT_FORM_POS
};

static void MakeUtfKey(const char* key, char* buffer, size_t bufferSize) {
    if (bufferSize < 2) {
        ythrow yexception() << "Buffer too small";
    }

    const TUtf16String w = UTF8ToWide(key);
    TFormToKeyConvertor c(buffer, bufferSize);
    c.Convert(w.c_str(), w.size());
}

static bool IsValidStr(const char *str) {
    while (*str) {
        if (*str == '\n' || *str == '\t')
            return false;
        ++str;
    }
    return true;
}

static bool IsValidKey(const char *str) {
    if (strlen(str) >= MAXKEY_BUF) {
        fprintf(stderr, "%s: wrong key\n", str);
        return false;
    }
    TKeyLemmaInfo lemma;
    char szForms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    int nForms = DecodeKey(str, &lemma, szForms);
    if (nForms < 0) {
        fprintf(stderr, "%s: wrong key\n", str);
        return false;
    }
    if (nForms == 0)
        return IsValidStr(str);
    if (!IsValidStr(lemma.szPrefix) || !IsValidStr(lemma.szLemma))
        return false;
    for (int k = 0; k < nForms; ++k) {
        if (!IsValidStr(szForms[k]))
            return false;
    }
    return true;
}

const int N_MAXDECODED_KEY_LENGTH = (MAXKEY_BUF + 3) * (N_MAX_FORMS_PER_KISHKA + 1) + 20;
static TString FormatKey(const char *pszKey, bool printLang, bool printEsc)
{
    char buffer[N_MAXDECODED_KEY_LENGTH] = "";
    TKeyReader reader(pszKey);
    const size_t nForms = reader.GetFormCount();
    if (nForms)
        PrintLemmaWithForms(buffer, reader, printLang);
    else
        strcpy(buffer, pszKey);

    if (printEsc) {
        TString ret;
        for (const char* p = buffer; *p; ++p){
            switch (*p) {
                case '\\':
                    ret += "\\\\";
                    break;
                case '\a':
                    ret += "\\a";
                    break;
                case '\b':
                    ret += "\\b";
                    break;
                case '\f':
                    ret += "\\f";
                    break;
                case '\n':
                    ret += "\\n";
                    break;
                case '\r':
                    ret += "\\r";
                    break;
                case '\t':
                    ret += "\\t";
                    break;
                case '\v':
                    ret += "\\v";
                    break;
                default:
                    ret += *p;
                    break;
            }
        }
        return ret;
    }

    return buffer;
}

class TLazyPrintF
{
private:
    TString SLazy;
    bool Lazy;

    void UnLazy()
    {
        if (Lazy) {
            puts(SLazy.data());
            Lazy = false;
        }
    }

public:
    explicit TLazyPrintF(const TString& sLazy)
        : SLazy(sLazy)
        , Lazy(true)
    {}

    void PrintF(const TString& s)
    {
        FPrintF(stdout, s);
    }

    void FPrintF(FILE* fOut, const TString& s)
    {
        UnLazy();
        fputs(s.data(), fOut);
    }

    bool IsLazy() const
    {
        return Lazy;
    }

    void Append(const TString& s)
    {
        SLazy += s;
    }
};

struct TDiaPrintKeysParams {
    bool PrintKey;
    bool PrintCounter;
    bool PrintLength;
    bool PrintOffset;
    bool PrintLineFeeds;
    bool PrintEscSeqs;
    bool CheckHits;
    bool CheckStrippedHits;
    EPrintPosType PrintPosType;
    bool FilterByDocId;
    ui32 DocId;
    bool PrintValidOnly;
    bool PrintLang;
    bool UseMmap;

    TDiaPrintKeysParams()
        : PrintKey(true)
        , PrintCounter(true)
        , PrintLength(true)
        , PrintOffset(true)
        , PrintLineFeeds(false)
        , PrintEscSeqs(false)
        , CheckHits(false)
        , CheckStrippedHits(false)
        , PrintPosType(PPT_NO)
        , FilterByDocId(false)
        , DocId(0)
        , PrintValidOnly(false)
        , PrintLang(false)
        , UseMmap(false)
    {}
};

static int DiaPrintKeys(const char *fnYndex, const char *lowKey, const char *highKey, const TDiaPrintKeysParams& params)
{
    TSequentYandReader syr;
    syr.Init(fnYndex, lowKey, highKey, HITS_MAP_SIZE, MAXKEY_BUF*512, params.UseMmap ? RH_FORCE_MAP : RH_FORCE_ALLOC);

    int r = 0, ret = 0;
    if (params.CheckHits) {
        TYndex4Searching y4s;
        y4s.InitSearch(fnYndex, RH_FORCE_ALLOC);
        TRequestContext ctx;
        i32 dummy = 0;
        char prevTextPointer[MAXKEY_BUF] = "";
        for (ui32 key = 0; key < y4s.KeyCount(); ++key) {
            const YxRecord* now = y4s.EntryByNumber(ctx, key, dummy);
            /*
            TRequestContext ctx2;
            ui32 num = y4s.LowerBound(~word, ctx2);
            if (num != key && num != key + 1) {
                ret = 1;
                fprintf(stderr, "bad fat '%s' %d %d\n", ~word, key, num);
            }
            */
            if (strcmp(now->TextPointer, prevTextPointer) < 0) {
                ret = 1;
                fprintf(stderr, "bad fat '%s' '%s'\n", now->TextPointer, prevTextPointer);
            }
            if (strlen(now->TextPointer) > MAXKEY_LEN) {
                ret = 1;
                fprintf(stderr, "too long key: '%s'\n", now->TextPointer);
            }
            strcpy(prevTextPointer, now->TextPointer);
        }
    }

    i64 curOffset = syr.Valid() ? syr.CurKey().Offset : 0;
    char szPrevKey[MAXKEY_BUF] = "";

    for (; syr.Valid(); syr.Next()) {
        const YxKey &entry = syr.CurKey();
        if (!entry.Text && !params.CheckHits)
            continue;

        if (strcmp(entry.Text, szPrevKey) < 0) {
            fprintf(stderr, "%s: wrong key order, prev key: %s\n",
                    FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                    FormatKey(szPrevKey, false, params.PrintEscSeqs).c_str());
        }
        strlcpy(szPrevKey, entry.Text, MAXKEY_BUF);

        if (params.PrintValidOnly && !IsValidKey(entry.Text))
            continue;
        TString mess;
        if (params.PrintKey)
            mess += FormatKey(entry.Text, params.PrintLang, params.PrintEscSeqs);
        if (params.PrintCounter)
            mess += Sprintf("\t%" PRIi64, (i64)entry.Counter);
        if (params.PrintLength)
            mess += Sprintf("\t%" PRIi64, (i64)entry.Length);
        if (params.PrintOffset)
            mess += Sprintf("\t%" PRIi64, entry.Offset);
        TString keyInfo = mess;
        if (params.PrintPosType == PPT_SORT_POS)
            mess.clear();

        TLazyPrintF lazyPrinter(mess);
        if ((params.PrintPosType == PPT_NO) && !params.CheckHits) {
            if (!params.FilterByDocId)
                lazyPrinter.PrintF("\n");
            continue;
        }

        const bool qLongHits = params.PrintPosType != PPT_DOC && params.PrintPosType != PPT_SORT_POS
            && ((entry.Counter > 3) && params.PrintPosType != PPT_NO || params.PrintLineFeeds);
        if (qLongHits)
            lazyPrinter.Append("\n");

        i64 counter = syr.CurKey().Counter;
        TSequentPosIterator it(syr);
        TSubIndexInfo subIndexInfo = syr.GetSubIndexInfo();

        if (params.CheckHits) {
            TWordPosition wp = *it;
            if (wp.Doc() > DOC_LEVEL_Max) {
                ret = 1;
                lazyPrinter.FPrintF(stderr, Sprintf("%s: first pos too large: %" PRIx64 "\n",
                    FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                    *it));
            }
            if (entry.Offset != curOffset) {
                ret = 1;
                lazyPrinter.FPrintF(stderr, Sprintf("%s: wrong entry offset : %" PRIx64 " != %" PRIx64 "\n",
                    FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                    entry.Offset, curOffset));
                curOffset = entry.Offset;
            }
            curOffset += hitsSize(curOffset, entry.Length, entry.Counter, subIndexInfo);
        }

        int formRemap[16];
        int nFormCount = 0;
        if (params.CheckHits) {
            TKeyLemmaInfo lemma;
            char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
            int formLens[N_MAX_FORMS_PER_KISHKA];
            nFormCount = DecodeKey(entry.Text, &lemma, forms, formLens);

            if(params.CheckStrippedHits) {
                ConvertKeyToStrippedFormat(&lemma, forms, formLens, nFormCount);

                for(int i = 0; i < nFormCount; i++) {
                    formRemap[i] = i;

                    for(int j = i + 1; j < nFormCount; j++)
                        if(strcmp(forms[i], forms[j]) == 0)
                            formRemap[i] = j;
                }
            }
        }

        SUPERLONG prevPos = START_POINT;
        long olddoc = 0;
        i64 curcnt = 0;
        for (; it.Valid(); ++it) {
            ++curcnt;
            if (params.CheckHits) {
                if (nFormCount > 0 && TWordPosition::Form(*it) >= static_cast<ui32>(nFormCount)) {
                    TWordPosition wp = *it;
                    lazyPrinter.PrintF(Sprintf("%s: wrong word form number [%ld.%ld.%ld.%ld.%ld]",
                        FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                        (long)wp.Doc(), (long)wp.Break(),
                        (long)wp.Word(), (long)wp.GetRelevLevel(),
                        (long)wp.Form()));
                }
                if (prevPos == *it) {
                    if (NeedRemoveHitDuplicates(entry.Text)) {
                        ret = 1;
                        lazyPrinter.FPrintF(stderr, Sprintf("'%s': equal hits: %" PRIx64 " == %" PRIx64 "\n",
                            FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                            prevPos, *it));
                    }
                } else if (prevPos > *it) {
                    TWordPosition wp1 = *it, wp2 = prevPos;
                    if (wp1.Doc() != wp2.Doc()
                        || wp1.Break() != BREAK_LEVEL_Max || wp1.Word() != WORD_LEVEL_Max
                        || wp2.Break() != BREAK_LEVEL_Max || wp2.Word() != WORD_LEVEL_Max)
                    {
                        ret = 1;
                        lazyPrinter.FPrintF(stderr, Sprintf("'%s': wrong order: %" PRIx64 " > %" PRIx64 "\n",
                            FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                            prevPos, *it));
                    }
                } else if(params.CheckStrippedHits) {
                    if (NeedRemoveHitDuplicates(entry.Text)) {
                        if((prevPos & ~NFORM_LEVEL_Mask) == (*it & ~NFORM_LEVEL_Mask) && formRemap[TWordPosition::Form(prevPos)] == formRemap[TWordPosition::Form(*it)]) {
                            ret = 1;
                            lazyPrinter.FPrintF(stderr, Sprintf("'%s': equal stripped hits: %" PRIx64 " == %" PRIx64 "\n",
                                FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                prevPos, *it));
                        }
                    }
                }

                prevPos = *it;
            }

            TWordPosition wp = *it;
            ui32 curDocId = 0;

            curDocId = wp.Doc();

            if (!params.FilterByDocId || (curDocId == params.DocId)) {
                switch (params.PrintPosType) {
                case PPT_HEX_POS:
                    lazyPrinter.PrintF(Sprintf("\t%016" PRIx64 "", *it));
                    break;
                case PPT_SORT_POS:
                    lazyPrinter.PrintF(Sprintf("\t%08lx %08lx %08lx %s\n",
                        (long)wp.Doc(), (long)wp.Break(), (long)wp.Word(), keyInfo.data()));
                    break;
                case  PPT_DOCLEN_POS:
                    lazyPrinter.PrintF(Sprintf("\t[%ld.%ld]", (long)wp.Doc(), (long)wp.DocLength()));
                    break;
                case PPT_DOC:
                {
                    long doc = (long)wp.Doc();
                    if (doc == 0 || doc != olddoc) {
                        lazyPrinter.PrintF(Sprintf("\t%ld", (long)wp.Doc()));
                    }
                    olddoc = doc;
                    break;

                }
                case PPT_FORM_POS:
                    lazyPrinter.PrintF(Sprintf("\t[%ld.%ld.%ld.%ld.%ld]",
                        (long)wp.Doc(), (long)wp.Break(), (long)wp.Word(), (long)wp.GetRelevLevel(), (long)wp.Form()));
                    break;
                case PPT_POS:
                    lazyPrinter.PrintF(Sprintf("\t[%ld.%ld.%ld.%ld]",
                        (long)wp.Doc(), (long)wp.Break(), (long)wp.Word(), (long)wp.GetRelevLevel()));
                    break;
                case PPT_NO:
                    break;
                }
                if (qLongHits)
                    lazyPrinter.PrintF(Sprintf("\n"));
            }
        }

        if (params.CheckHits) {
            TWordPosition wp = prevPos;
            if (wp.Doc() > DOC_LEVEL_Max) {
                ret = 1;
                lazyPrinter.FPrintF(stderr, Sprintf("%s: last pos too large: %" PRIx64 "\n",
                    FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                    prevPos));
            }
            if (curcnt != counter) {
                ret = 1;
                lazyPrinter.FPrintF(stderr, Sprintf("%s: wrong hits count: %li != %li\n",
                    FormatKey(entry.Text, false, params.PrintEscSeqs).c_str(),
                                                    curcnt, counter));
            }
        }
        if (params.PrintKey && !qLongHits && (!params.FilterByDocId || !lazyPrinter.IsLazy())
                && (params.PrintPosType != PPT_SORT_POS))
            lazyPrinter.PrintF("\n");
    }
    r = ret || r;
    return r;
}

//////////////////////////////////////////////////////////////////
//                           main()                             //
//////////////////////////////////////////////////////////////////
void usage() {
    fprintf(stderr, "USAGE: diaprintkeys [-w -x -c -L -O] [-h highkey] [-F docId] yndex lowkey\n");
    fprintf(stderr, "\t -w, -f print word positions (-f for forms)\n");
    fprintf(stderr, "\t -l print 'doc. length' word positions\n");
    fprintf(stderr, "\t -n print linefeeds\n");
    fprintf(stderr, "\t -x print hex word positions\n");
    fprintf(stderr, "\t -s print word positions for later sort\n");
    fprintf(stderr, "\t -E print esaped control symbols, backslash and others\n");
    fprintf(stderr, "\t -g print language\n");
    fprintf(stderr, "\t -c check hits\n");
    fprintf(stderr, "\t -C check hits only (lowkey unnecessary)\n");
    fprintf(stderr, "\t -R check for duplicate hits after key stripping (to be used with -c or -C)\n");
    fprintf(stderr, "\t -F docId print just for docId\n");
    fprintf(stderr, "\t -O don't print offset\n");
    fprintf(stderr, "\t -u treat key as unicode\n");
    fprintf(stderr, "\t -i print index information (lowkey unnecessary)\n");
    fprintf(stderr, "\t -m use mmap to save memory\n");
}

void PrintIndexInfo(const char* prefix) {
    NIndexerCore::TInputIndexFile indexFile(prefix, IYndexStorage::FINAL_FORMAT);
    Cout << "Index info:\n";
    Cout << "\tFormat: FINAL_FORMAT\n";
    Cout << "\tVersion: " << NIndexerCore::PrintIndexVersion(indexFile.GetVersion()) << "\n";
    Cout << "\tNumber Of Keys: " << indexFile.GetNumberOfKeys() << "\n";
    Cout << "\tNumber Of Blocks: " << abs(indexFile.GetNumberOfBlocks()) << "\n";
    Cout << "\tSize of FAT: " << indexFile.GetSizeOfFAT() << "\n";
    Cout << "\tSubindex: " << (indexFile.GetNumberOfBlocks() < 0 ? "YES" : "NO") << "\n";
}

int wrapped_main(int argc, char *argv[]) {
    TDiaPrintKeysParams params;
    char highKeyBuf[MAXKEY_BUF*2];
    highKeyBuf[0] = 0;

    bool checkHitsOnly= false;
    bool showDocsOnly = false;
    bool isUtf = false;
    bool dumpSize = true;
    bool printIndexInfo = false;
    int optlet;
    PRINT_VERSION;
    Opt opt(argc, argv, "SCRcwfxsglnvdh:F:OuiEm");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'm':
            params.UseMmap = true;
            break;
        case 'S':
            dumpSize = false;
            break;
        case 'C':
            checkHitsOnly = true;
            break;
        case 'd':
            showDocsOnly = true;
            break;
        case 'c':
            params.PrintValidOnly = false;
            params.CheckHits = true;
            break;
        case 'R':
            params.CheckStrippedHits = true;
            break;
        case 'w':
            params.PrintPosType = PPT_POS;
            break;
        case 'f':
            params.PrintPosType = PPT_FORM_POS;
            break;
        case 'x':
            params.PrintPosType = PPT_HEX_POS;
            break;
        case 's':
            params.PrintPosType = PPT_SORT_POS;
            break;
        case 'E':
            params.PrintEscSeqs = true;
            break;
        case 'g':
            params.PrintLang = true;
            break;
        case 'l':
            params.PrintPosType = PPT_DOCLEN_POS;
            break;
        case 'n':
            params.PrintLineFeeds = true;
            break;
        case 'h':
            strlcpy(highKeyBuf, opt.Arg, MAXKEY_BUF);
            break;
        case 'v':
            params.PrintValidOnly = true;
            break;
        case 'F':
            params.FilterByDocId = true;
            params.DocId = atoi(opt.Arg);
            break;
        case 'O':
            params.PrintOffset = false;
            break;
        case 'u':
            isUtf = true;
            break;
        case 'i':
            printIndexInfo = true;
            break;
        default:
            usage();
            return 1;
        }
    }

    if (argc - opt.Ind != 2 - (checkHitsOnly || printIndexInfo)) {
        usage();
        return 1;
    }

    if (printIndexInfo) {
        PrintIndexInfo(argv[opt.Ind]);
        return 0;
    }

    const char* lowKey = "";
    if (checkHitsOnly) {
        params.CheckHits = true;
        params.PrintValidOnly = false;
        params.PrintKey = params.PrintCounter = params.PrintLength = params.PrintOffset = false;
        params.PrintPosType = PPT_NO;
        highKeyBuf[0] = 0;
    } else if (showDocsOnly) {
        params.PrintCounter = params.PrintLength = params.PrintOffset = false;
        params.PrintPosType = PPT_DOC;
        lowKey = argv[opt.Ind+1];
    } else {
        lowKey = argv[opt.Ind+1];
    }

    if (!dumpSize) {
        params.PrintLength = params.PrintOffset = false;
    }

    char utfLowKey[MAXKEY_BUF * 2 + 2];
    if (isUtf && *lowKey) {
        MakeUtfKey(lowKey, utfLowKey, MAXKEY_BUF * 2 + 2);
        lowKey = utfLowKey;
    }

    char utfHighKey[MAXKEY_BUF * 2 + 2];
    char* highKey = highKeyBuf;
    if (!highKey[0]) {
        strncat(highKey, lowKey, MAXKEY_BUF);
        memset(highKey+strlen(highKey), '\xff', MAXKEY_BUF-1);
    } else if (isUtf) {
        MakeUtfKey(highKey, utfHighKey, MAXKEY_BUF * 2 + 2);
        highKey = utfHighKey;
    }

    int ret = DiaPrintKeys(argv[opt.Ind], lowKey, highKey, params);

    if (checkHitsOnly && !ret)
        fprintf(stderr, "OK\n");

    return ret;
}

int main(int argc, char *argv[]) {
    try {
        return wrapped_main(argc, argv);
    } catch (...) {
        Cerr << "Exception: " << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
