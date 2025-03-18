#include <ysite/yandex/common/prepattr.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>       // portable getopt from util
#include <kernel/keyinv/hitlist/positerator.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/invkeypos/keycode.h>

#include <library/cpp/deprecated/autoarray/autoarray.h>
#include <library/cpp/charset/codepage.h>
#include <library/cpp/containers/mh_heap/mh_heap.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/system/filemap.h>

#include <algorithm>
#include <cstdio>
#include <functional>

#include "printkeys.h"
#include <util/string/split.h>

#define BIG_BUF ((size_t)0x100000ul)

#define BAD_KEY_ESCAPE '\x1b'

inline
char *EscapeBadKey(char escaped[/* 2*MAXKEY_LEN */], const char key[/* MAXKEY_LEN */])
{
    const char *iptr = key;
    char *optr = escaped;
    while(*iptr)
        if ((unsigned char)*iptr <= ' ' && (*iptr != '\x01' || *(iptr+1))) {
            *optr++ = BAD_KEY_ESCAPE;
            *optr++ = *iptr++ + ' ';
        } else {
            *optr++ = *iptr++;
        }
    *optr = *iptr;
    return escaped;
}

const int N_MAX_FORMS_COUNTED = 512;
struct SIndexStats {
    SUPERLONG forms[N_MAX_FORMS_COUNTED], formCounts[N_MAX_FORMS_COUNTED];

    SIndexStats() { memset(this, 0, sizeof(*this)); }
    void OutputFormCounts(const char *pszLemma, const std::vector<int> &_counts) {
        if (*pszLemma == 0 || *pszLemma == '!' || *pszLemma == '#')
            return; // count only words
        std::vector<int> counts = _counts;
        std::sort(counts.begin(), counts.end(), std::greater<int>());
        if (counts.size() > (size_t)N_MAX_FORMS_COUNTED)
            counts.resize(N_MAX_FORMS_COUNTED);
        for (size_t i = 0; i < counts.size(); ++i) {
            ++forms[i];
            formCounts[i] += counts[i];
        }
    }
};

static void PrintStats(const SIndexStats &stats) {
    printf("Word forms stats:\n");
    SUPERLONG sumFormCounts = 0;
    for (int i = 0; i < N_MAX_FORMS_COUNTED; ++i)
        sumFormCounts += stats.formCounts[i];
    for (int i = 0; i < N_MAX_FORMS_COUNTED; ++i) {
        printf("%d\t%g\t%g\n", i + 1, ((double)stats.forms[i]) / stats.forms[0], ((double)stats.formCounts[i]) / sumFormCounts);
    }
}

const int N_MAXDECODED_KEY_LENGTH = (MAXKEY_BUF + 3) * (N_MAX_FORMS_PER_KISHKA + 1) + 10;
static int DecodeKeyForms(char *pszBuf, NIndexerCore::TIndexReader& indexReader, char *pszOutputLemma, bool printLang)
{
    const char* const pszKey = indexReader.GetKeyText();
    if (pszOutputLemma)
        strcpy(pszOutputLemma, pszKey);
    TKeyReader reader(pszKey, indexReader.GetVersion() == YNDEX_VERSION_RAW64_HITS);
    const size_t nForms = reader.GetFormCount();
    if (nForms) {
        if (pszOutputLemma) {
            strcpy(pszOutputLemma, reader.GetPrefix());
            strcat(pszOutputLemma, reader.GetLemma());
            if (printLang && reader.GetLang() != LANG_UNK) {
                AppendLanguage(pszOutputLemma, reader);
            }
        }
        PrintLemmaWithForms(pszBuf, reader, printLang);
    } else
        strcpy(pszBuf, pszKey);
    return nForms;
}

int CompareKeyPrefix(const char* fullKey, const PRINTKEYS_OPTIONS& opt) {
    if (opt.remove_prefix) {
        const size_t len = strlen(fullKey);
        {
            char buffer[MAXKEY_BUF];
            if (fullKey[0] == ATTR_PREFIX) { // attribute
                for (const char* p = fullKey + 1; *p != '=' || *p == 0; ++p) { // *p == 0 for ##_DOC_LENS for example
                    if (*p == KEY_PREFIX_DELIM) {
                        ++p;
                        buffer[0] = ATTR_PREFIX;
                        const size_t n = len - (p - fullKey);
                        memcpy(buffer + 1, p, n);
                        buffer[n + 1] = 0;
                        return memcmp(buffer, opt.key_prefix, strlen(opt.key_prefix));
                    }
                }
            } else if (fullKey[0] == OPEN_ZONE_PREFIX || fullKey[0] == CLOSE_ZONE_PREFIX) { // zone
                for (const char* p = fullKey + 1; *p == 0; ++p) {
                    if (*p == KEY_PREFIX_DELIM) {
                        ++p;
                        buffer[0] = ATTR_PREFIX;
                        const size_t n = len - (p - fullKey);
                        memcpy(buffer + 1, p, n);
                        buffer[n + 1] = 0;
                        return memcmp(buffer, opt.key_prefix, strlen(opt.key_prefix));
                    }
                }
            } else { // lemma
                if (fullKey[0] == KEY_PREFIX_DELIM) {
                    strcpy(buffer, fullKey);
                    for (const char* p = buffer; (unsigned char)*p > 0x1F; ++p) {
                        if (*p == KEY_PREFIX_DELIM) {
                            ++p;
                            const size_t n = strlen(buffer) - (p - buffer);
                            memmove(buffer, p, n);
                            buffer[n] = 0;
                            return memcmp(buffer, opt.key_prefix, strlen(opt.key_prefix));
                        }
                    }
                } else {
                    for (const char* p = fullKey; (unsigned char)*p > 0x1F; ++p) {
                        if (*p == KEY_PREFIX_DELIM) {
                            ++p;
                            const size_t n = len - (p - fullKey);
                            memcpy(buffer, p, n);
                            buffer[n] = 0;
                            return memcmp(buffer, opt.key_prefix, strlen(opt.key_prefix));
                        }
                    }
                }
            }
            return memcmp(buffer, opt.key_prefix, strlen(opt.key_prefix));
        }
    }
    return memcmp(fullKey, opt.key_prefix, strlen(opt.key_prefix));
}

static void CollectStats(
    const char* namekey,
    const char* nameinv,
    const PRINTKEYS_OPTIONS &opt)
{
    NIndexerCore::TIndexReader ir(namekey, nameinv, opt.is_full_format);
    SIndexStats stats;
    TPosIterator<> it;
    char szPrevLemma[MAXKEY_BUF] = "";
    std::vector<int> formCounts;

    while (ir.ReadKey()) {
        char szCurLemma[MAXKEY_BUF], szFullKey[N_MAXDECODED_KEY_LENGTH];
        int nForms = DecodeKeyForms(szFullKey, ir, szCurLemma, opt.print_lang);


        if (opt.print_valid_only && !is_valid_key(szFullKey))
            continue;
        if (opt.key_prefix[0]) {
            const int cmp = CompareKeyPrefix(szFullKey, opt);
            if (cmp < 0)
                continue;
            else if (cmp > 0)
                break;
        }

        if (nForms == 0 || strcmp(szPrevLemma, szCurLemma) != 0 ) {
            stats.OutputFormCounts(szPrevLemma, formCounts);
            formCounts.resize(0);
        }
        strcpy(szPrevLemma, szCurLemma);

        if (nForms == 0) {
            formCounts.push_back(ir.GetCount());
            continue;
        }

        ir.InitPosIterator(it);

        ui32 nFormCounts[NFORM_LEVEL_Max + 1];
        memset(nFormCounts, 0, sizeof(nFormCounts));
        for (; it.Valid(); ++it)
        {
            TWordPosition wp = *it;
            ui32 nForm = wp.Form();
            ++nFormCounts[nForm];
        }
        formCounts.insert(formCounts.end(), nFormCounts, nFormCounts + nForms);
    }
    PrintStats(stats);
}

static bool FormatKey(char *str, const char *szFullKey, const PRINTKEYS_OPTIONS &opt, bool *pbContinue, size_t StrLen) {
    if (opt.print_valid_only && !is_valid_key(szFullKey))
        return false;
    if (opt.key_prefix[0]) {
        const int cmp = CompareKeyPrefix(szFullKey, opt);
        if (cmp < 0)
            return false;
        else if (cmp > 0) {
            *pbContinue = false;
            return false;
        }
    }
    if (opt.print_exact_keys) {
        EscapeBadKey(str, szFullKey);
    } else {
        strlcpy(str, szFullKey, StrLen); //strlcpy(str, szFullKey, MAXKEY_BUF);
        bool is_cap = DecaseWord(str, strlen(str)) != 0;
        if (is_cap) {
            char *s;
            if (str[0] != '#') {
                s = str;
            } else {
                s = strchr(str, '=') + 1;
                assert(s != (char*)1 && (*s != '"' || strchr(s, '#') != nullptr)); // only words and '#url' can be capitalized
            }
            if (*s == '!')
                s++;
#ifndef PORTER_STEMMER_CAN_BE_NULL_BUG
            assert(*s);
#else
            if (*s)
#endif
                *s = csYandex.ToUpper(*s);
        }
    }
    return true;
}

template <class TIterator>
void TypeHits(TIterator *pIterator, const PRINTKEYS_OPTIONS &opt, const i32 count = 0, const char* key = nullptr)
{
    i32 n = 0;
    TIterator &it = *pIterator;
    if (opt.doc_id && key)
        it.SkipTo(TWordPosition(*opt.doc_id, 0, 0).SuperLong());
    for (; it.Valid(); ++it, ++n)
    {
        TWordPosition wp = *it;
        if (opt.doc_id && key) {
            if (wp.Doc() > *opt.doc_id)
                break;
            printf("%s", key);
        }
        switch (opt.print_pos) {
            case PRINTPOS_NONE:
                printf("\n");
                break;
            case PRINTPOS_WORDPOS:
                printf("\t[%ld.%ld.%ld.%ld]\n",
                    (long)wp.Doc(), (long)wp.Break(),
                    (long)wp.Word(), (long)wp.GetRelevLevel());
                break;
            case PRINTPOS_WORDPOS_NFORM:
                printf("\t[%ld.%ld.%ld.%ld.%ld]\n",
                    (long)wp.Doc(), (long)wp.Break(),
                    (long)wp.Word(), (long)wp.GetRelevLevel(),
                    (long)wp.Form());
                break;
            case PRINTPOS_DOCID:
                printf("\t%ld\n", (long)(TWordPosition(*it).Doc()));
                break;
            case PRINTPOS_DOCLENG:
                printf("\t%ld\t%ld\n", (long)wp.Doc(), (long)wp.DocLength());
                break;
            case PRINTPOS_HEX:
                printf("\t%016" PRIx64 "\n", *it);
                break;
        }
    }
    if (count)
        Y_ASSERT(n == count);
}

static void PrintPositions(
    const char* namekey,
    const char* nameinv,
    const PRINTKEYS_OPTIONS &opt)
{
    NIndexerCore::TIndexReader ir(namekey, nameinv, opt.is_full_format);
    TPosIterator<> it;
    // inv-file reading here...
    ir.InitPosIterator(it, opt.hit_offset, opt.hit_length, opt.hit_count);
    TypeHits(&it, opt);
}


static void PrintKeysOfDoc(
    const char* namekey,
    const char* nameinv,
    const PRINTKEYS_OPTIONS &opt)
{
    NIndexerCore::TIndexReader ir(namekey, nameinv, opt.is_full_format);
    TPosIterator<> it;
    // inv-file reading here...
    bool bContinue = true;
    while (ir.ReadKey() && bContinue) {
        ir.InitPosIterator(it); //, opt.hit_offset, opt.hit_length, opt.hit_count);
        char str[N_MAXDECODED_KEY_LENGTH * 2 + 1];

        char szCurLemma[MAXKEY_BUF], szFullKey[N_MAXDECODED_KEY_LENGTH];
        DecodeKeyForms(szFullKey, ir, szCurLemma, opt.print_lang);
        if (!FormatKey(str, szFullKey, opt, &bContinue, N_MAXDECODED_KEY_LENGTH * 2))
            continue;
        TypeHits(&it, opt, 0, WideToUTF8(CharToWide(str, csYandex)).data());
    }
}

static void PrintKeys(
    const char* namekey,
    const char* nameinv,
    const PRINTKEYS_OPTIONS &opt)
{
    NIndexerCore::TIndexReader ir(namekey, nameinv, opt.is_full_format);
    TPosIterator<> it;

    const char *format32 = opt.use_tab ? "\t%" PRId32 : " %" PRId32;
    const char *format64 = opt.use_tab ? "\t%" PRId64 : " %" PRId64;

    bool bContinue = true;
    while (bContinue && ir.ReadKey()) {
        char str[N_MAXDECODED_KEY_LENGTH * 2 + 1];

        char szCurLemma[MAXKEY_BUF], szFullKey[N_MAXDECODED_KEY_LENGTH];
        DecodeKeyForms(szFullKey, ir, szCurLemma, opt.print_lang);

        if (!FormatKey(str, szFullKey, opt, &bContinue, N_MAXDECODED_KEY_LENGTH * 2))
            continue;

        fputs(str, stdout);

        if (opt.is_full_format == IYndexStorage::FINAL_FORMAT)
            printf(format32, ir.GetCount());
        printf(format32, ir.GetLength());
        if (opt.print_offset)
            printf(format64, ir.GetOffset());
        fputs("\n", stdout);

        if (opt.print_pos == PRINTPOS_NONE)
            continue;

        // inv-file reading here...
        ir.InitPosIterator(it);
        TypeHits(&it, opt);
    }
}

static void PrintLemma(const char* pszLemma, bool* pbContinue, i32 totalCount, i32 totalLength,
    i64 firstOffset, TVector< TPosIterator<> >& iters, const PRINTKEYS_OPTIONS& opt)
{
    char str[2*MAXKEY_BUF+1];
    const char *format32 = opt.use_tab ? "\t%" PRId32 : " %" PRId32;
    const char *format64 = opt.use_tab ? "\t%" PRId64 : " %" PRId64;

    if (FormatKey(str, pszLemma, opt, pbContinue, 2*MAXKEY_BUF)) {
        fputs(str, stdout);
        if (opt.is_full_format == IYndexStorage::FINAL_FORMAT)
            printf(format32, totalCount);
        printf(format32, totalLength);
        if (opt.print_offset)
            printf(format64, firstOffset);
        fputs("\n", stdout);

        if (opt.print_pos != PRINTPOS_NONE) {
            const int nSize = iters.size();
            if (nSize == 1) {
                TypeHits(&iters[0], opt, totalCount);
            } else {
                autoarray< TPosIterator<>* > pIters(nSize);
                for (int i = 0; i < nSize; ++i) {
                    pIters[i] = &iters[i];
                }
                MultHitHeap< TPosIterator<> > it(&pIters[0], nSize);
                it.Restart();
                TypeHits(&it, opt);
            }
        }
    }
}

void PrintKeysByLemma(const char* namekey, const char* nameinv, const PRINTKEYS_OPTIONS &opt)
{
    NIndexerCore::TIndexReader ir(namekey, nameinv, opt.is_full_format);

    char szPrevLemma[MAXKEY_BUF] = "";
    i32 totalCount = 0;
    i32 totalLength = 0;
    i64 firstOffset = 0;
    TVector< TPosIterator<> > iters;

    bool bContinue = true;
    while (bContinue && ir.ReadKey()) {
        char szCurLemma[MAXKEY_BUF], szFullKey[N_MAXDECODED_KEY_LENGTH];
        DecodeKeyForms(szFullKey, ir, szCurLemma, opt.print_lang);

        if (opt.print_valid_only && !is_valid_key(szFullKey))
            continue;

        if (strcmp(szPrevLemma, szCurLemma) != 0 && !iters.empty()) {
            PrintLemma(szPrevLemma, &bContinue, totalCount, totalLength, firstOffset, iters, opt);
            totalCount = 0;
            totalLength = 0;
            firstOffset = 0;
            iters.clear();
        }

        totalCount += ir.GetCount();
        totalLength += ir.GetLength();
        if (iters.empty())
            firstOffset = ir.GetOffset();

        iters.push_back(TPosIterator<>());
        ir.InitPosIterator(iters.back());

        strcpy(szPrevLemma, szCurLemma);

    }
    if (!iters.empty())
        PrintLemma(szPrevLemma, &bContinue, totalCount, totalLength, firstOffset, iters, opt);
}

static void die()
{
    fprintf(stderr, "Copyright (c) OOO \"Yandex\". All rights reserved.\n");
    fprintf(stderr, "Call software@yandex-team.ru for support.\n\n");
    fprintf(stderr, "printkeys - prints keys and word positions from key/inv\n");
    fprintf(stderr, "Usage: printkeys [-0|1] [-p] [-e] [-w|x|l|i|f] [-v] [-m] [-c] [-o output] [-k key] indexkey indexinv\n");
    fprintf(stderr, "\t-0 vs. -1 'intermediate' vs. 'full' (default) format (service keys)\n");
    fprintf(stderr, "\t-p positions file offset\n");
    fprintf(stderr, "\t-e exact escaped keys\n");
    fprintf(stderr, "\t-w word positions\n");
    fprintf(stderr, "\t-f word positions with nform\n");
    fprintf(stderr, "\t-x hexadecimal word positions\n");
    fprintf(stderr, "\t-l word positions as document lengths\n");
    fprintf(stderr, "\t-i doc indentificator\n");
    fprintf(stderr, "\t-v valid keys only\n");
    fprintf(stderr, "\t-s collect word form stats\n");
    fprintf(stderr, "\t-m ignored\n");
    fprintf(stderr, "\t-r remove prefix from keys before comparison for option 'k' (beta)\n");
    fprintf(stderr, "\t-k\"key\" print keys with prefix \"key\"\n");
    fprintf(stderr, "\t-c do not merge by lemma\n");
    fprintf(stderr, "\t-t use tab as delimeter\n");
    fprintf(stderr, "\t-z print key language\n");
    fprintf(stderr, "\t-K\"count,length,offset\" print word positions at given offset\n");
    fprintf(stderr, "\t-D DOC_ID print keys and word positions of given document\n");
    exit(1);
}


int main(int argc, char** argv)
{
    PRINTKEYS_OPTIONS options =
    {
        false,         // 'print_valid_only' is off
        IYndexStorage::FINAL_FORMAT, // 'is_full_format' is on
        PRINTPOS_NONE, // 'print_pos' is off
        false,         // 'print_offset' is off
        false,         // 'print_exact_keys' is off
        false,         // 'use_mapping' is off
        {'\x00'},      // 'key_prefix' is zero string
        false,         // collect_stats
        false,         // use_tab
        false,         // print_lang
        false,         // remove_prefix
        false,         // print_hitlist
        0,             // hit_count
        0,             // hit_length
        0,             // hit_offset
        TMaybe<ui32>(),// document identifier
    };

    bool bMergeByLemma = true;
    int optlet;
    class Opt opt (argc, argv, "01wlxivmprteo:dk:K:fsczD:");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case '0':
            options.is_full_format = IYndexStorage::PORTION_FORMAT;
            break;
        case '1':
            options.is_full_format = IYndexStorage::FINAL_FORMAT;
            break;
        case 'o':
            if (freopen(opt.Arg, "w", stdout) == nullptr) {
                perror(opt.Arg);
                exit(1);
            }
            break;
        case 'e':
            options.print_exact_keys = true;
            break;
        case 'w':
            options.print_pos = PRINTPOS_WORDPOS;
            break;
        case 'i':
            options.print_pos = PRINTPOS_DOCID;
            options.print_valid_only = false;
            break;
        case 'f':
            options.print_pos = PRINTPOS_WORDPOS_NFORM;
            break;
        case 'l':
            options.print_pos = PRINTPOS_DOCLENG;
            break;
        case 'x':
            options.print_pos = PRINTPOS_HEX;
            break;
        case 'p':
            options.print_offset = true;
            break;
        case 't':
            options.use_tab = true;
            break;
        case 'k':
            strlcpy(options.key_prefix, opt.Arg, MAXKEY_BUF);
            break;
        case 'K':
            {
                TVector<TString> fields;
                StringSplitter(opt.Arg).Split(' ').SkipEmpty().Collect(&fields);
                Y_VERIFY(fields.size() == 3, "Not enough arguments for hit entry");
                options.print_hitlist = true;
                options.hit_count  = FromString<ui32>(fields[0]);
                options.hit_length = FromString<ui32>(fields[1]);
                options.hit_offset = FromString<ui64>(fields[2]);
            }
            break;
        case 'r':
            options.remove_prefix = true;
            break;
        case 'v':
            options.print_valid_only = true;
            break;
        case 'm':
            break;
        case 's':
            options.collect_stats = true;
            break;
        case 'c':
            bMergeByLemma = false;
            break;
        case 'z':
            options.print_lang = true;
            break;
        case 'D':
            options.doc_id = FromString<ui32>(opt.Arg);
            break;
        case '?':
        default:
            die();
            break;
        }
    }
    if (argc - opt.Ind < 2)
        die();

    if (!isatty(fileno(stdout)))
        setvbuf(stdout, nullptr, _IOFBF, BIG_BUF);

    try {
        if (options.doc_id)
            PrintKeysOfDoc(argv[opt.Ind], argv[opt.Ind+1], options);
        else if (options.print_hitlist)
            PrintPositions(argv[opt.Ind], argv[opt.Ind+1], options);
        else if (options.collect_stats)
            CollectStats(argv[opt.Ind], argv[opt.Ind+1], options);
        else if (bMergeByLemma)
            PrintKeysByLemma(argv[opt.Ind], argv[opt.Ind+1], options);
        else
            PrintKeys(argv[opt.Ind], argv[opt.Ind+1], options);
    } catch (std::exception& e) {
        fprintf(stderr, "%s", e.what());
        return 1;
    }
    return 0;
}
