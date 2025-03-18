#include <kernel/search_types/search_types.h>
#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/indexfile/rdkeyit.h>
#include <kernel/keyinv/indexfile/searchfile.h>
#include <kernel/keyinv/indexfile/seqreader.h>
#include <kernel/keyinv/invkeypos/keynames.h>
#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <util/folder/dirut.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/segmented_string_pool.h>

#include <time.h>

using namespace NIndexerCore;

const int WB_Shift = WORD_LEVEL_Bits + BREAK_LEVEL_Bits;

static
ui32 GetMaxDocHandle(const IKeysAndPositions& yndex)
{
    ui32 DocMax = 0xffffffff;

    TPosIterator<> it(yndex, YDX_MAX_FREQ_KEY, RH_DEFAULT);

    while (it.Valid()) {
        DocMax = it.Doc();
        it.Next();
    }

    if (DocMax == 0xffffffff)
        ythrow yexception() << "empty index";
    return DocMax;
}

class TAnchorScanner
{
private:
    struct TAnchorIndex {
        ui32 Count;
        ui32 Offset;
        const char* DocUrl;
        TAnchorIndex() : Count(0), Offset(0xffffffff), DocUrl(nullptr) {
        }
        bool IsHole() const {
            return Offset == 0xffffffff;
        }
    };
    struct TAnchorZone {
        TAnchorZone() : OffsetInWordVector(0xffffffff), pos(0), len(0) {
        }
        bool operator <(const TAnchorZone& other) const {
            return pos < other.pos;
        }
        bool operator <(ui32 wb) const {
            return pos < wb;
        }
        ui32 OffsetInWordVector;
        ui32 pos;
        ui32 len;
    };

    //Needed because MSVC STL requires symmetric less operator in debug mode
    friend bool operator <(ui32 w, const TAnchorZone& a);

    struct TWordInAnchor {
        const char* Forma;
        TWordInAnchor() : Forma(nullptr) {
        }
    };

    TVector<TAnchorIndex> Offsets;
    TVector<TAnchorZone> Ancs;
    TVector<TWordInAnchor> Words;
    segmented_string_pool UsefulKeys;
public:
    void Init(const char* index) {
        TYndex4Searching yndex;
        yndex.InitSearch(index);
        size_t DocMax = GetMaxDocHandle(yndex) + 1;
        fprintf(stderr, "Start to analyse up to %" PRISZT " documents.\n", DocMax);
        Offsets.resize(DocMax);
    }

    TAnchorScanner() {}

    void CountLink(ui32 doc) {
        assert(doc < Offsets.size());
        TAnchorIndex& idx = Offsets[doc];
        ++idx.Count;
    }

    void AllocateAncs() {
        ui32 Offset = 0;
        for (size_t doc = 0; doc < Offsets.size(); doc++) {
            TAnchorIndex& idx = Offsets[doc];
            if (idx.Count > 0) {
                idx.Offset = Offset;
                Offset += idx.Count;
                idx.Count = 0;
            }
        }
        fprintf(stderr, "Prepare for %u anchors.\n", Offset);
        Ancs.resize(Offset);
    }

    void AddZoneBegin(ui32 doc, ui32 word_n_break) {
        assert(doc < Offsets.size());
        TAnchorIndex& idx = Offsets[doc];
        assert(!idx.IsHole());
        TAnchorZone* rec_begin = &Ancs[idx.Offset];
        TAnchorZone *now = rec_begin + idx.Count;
        now->pos = word_n_break;
        ++idx.Count;
    }

    void Sort() {
        for (size_t doc = 0; doc < Offsets.size(); doc++) {
            TAnchorIndex& idx = Offsets[doc];
            if (idx.Count > 1) {
                TAnchorZone* b = &Ancs[idx.Offset];
                TAnchorZone* e = b + idx.Count;
                std::sort(b, e);
            }
        }
    }

    void AddZoneEnd(ui32 doc, ui32 word_n_break, ui32 wordcount) {
        assert(doc < Offsets.size());
        TAnchorIndex& idx = Offsets[doc];
        if (!idx.IsHole()) {
            assert(idx.Count);
            TAnchorZone* rec_begin = &Ancs[idx.Offset];
            for (ui32 i = 0; i < idx.Count; i++) {
                TAnchorZone* rec = rec_begin + i;
                if (rec->pos < word_n_break)
                    continue;
                else if (rec->pos == word_n_break) {
                    rec->len = wordcount;
                    break;
                } else
                    break;
            }
        }
    }

    void AllocateWords() {
        ui32 Offset = 0;
        for (size_t i = 0; i < Ancs.size(); i++) {
            TAnchorZone& anc = Ancs[i];
            if (anc.len > 0) {
                anc.OffsetInWordVector = Offset;
                Offset += anc.len;
            }
        }
        fprintf(stderr, "Prepare for %u words.\n", Offset);
        Words.resize(Offset);
    }

    char* AddFormaText(const char* Forma) {
        return UsefulKeys.append(Forma);
    }

    void AddWord(ui32 aid, ui32 word_n_break, const char* forma) {
        assert(aid < Ancs.size());
        TAnchorZone& anc = Ancs[aid];
        assert(anc.OffsetInWordVector != 0xffffffff);
        TWordInAnchor* word = &Words[anc.OffsetInWordVector];
        ui32 num = word_n_break - anc.pos;
        assert(num < anc.len);
        word += num;
        assert(word->Forma == nullptr || strcmp(word->Forma, forma) == 0); // the same forma has two or more lemms
        word->Forma = forma;
    }

    ui32 Find(ui32 doc, ui32 word_n_break) {
        assert(!(doc & ~DOC_LEVEL_Max));
        assert(doc < Offsets.size());
        TAnchorIndex& idx = Offsets[doc];
        if (idx.IsHole())
            return (ui32)-1;

        TAnchorZone *b = &Ancs[idx.Offset];
        TAnchorZone *e = b + idx.Count;
        TAnchorZone *f = std::lower_bound(b, e, word_n_break);
        if (f != e && f->pos == word_n_break) {
            if (f->len == 0)
                return (ui32)-1;
        } else {
            assert(f == e || f->pos > word_n_break);
            if (f == b)
                return (ui32)-1;
            --f;
            assert(f->pos < word_n_break);
            if (word_n_break - f->pos >= f->len)
                return (ui32)-1;
        }
        return idx.Offset + (f - b);
    }

    void AddDocUrls(const char* index) {
        TSequentYandReader UrlReader;
        UrlReader.Init(index, "#url=\"");
        while (UrlReader.Valid()) {
            TSequentPosIterator it(UrlReader);
            while (it.Valid()) {
                ui32 doc = it.Doc();
                assert(doc < Offsets.size());
                if (!Offsets[doc].IsHole()) {
                    const char* Url = UrlReader.CurKey().Text + 6;
                    Offsets[doc].DocUrl = UsefulKeys.append(Url);
                }
                ++it;
            }
            UrlReader.Next();
        }
    }

    void PrintAnchor(ui32 doc, ui32 aid) {
        TAnchorIndex& idx = Offsets[doc];
        assert(!idx.IsHole());
        assert(idx.DocUrl != nullptr);
        printf("%s\t", idx.DocUrl);
        TAnchorZone &anc = Ancs[aid];
        TWordInAnchor* bw = &Words[anc.OffsetInWordVector];
        TWordInAnchor* ew = bw + anc.len;
        bool wasPoint = false;
        for (TWordInAnchor* w = bw; w < ew; ++w) {
            if (w->Forma) {
                wasPoint = false;
                if (w > bw)
                    printf(" ");
                printf("%s", w->Forma);
            } else {
                if (!wasPoint) {
                    printf(".");
                    wasPoint = true;
                }
            }
        }
    }
};

bool operator <(ui32 w, const TAnchorScanner::TAnchorZone& a) {
    return a.pos < w;
}

static TAnchorScanner AnchorScanner;

inline
int load_anchors_positions(const TString& fname, const YxRecord& ys_b, const YxRecord& ys_e)
{
    TBufferedHitIterator hs_beg;
    TBufferedHitIterator hs_end;
    NIndexerCore::TInputFile fss;
    fss.Open(fname.data());
    hs_beg.Init(fss);
    hs_beg.Restart(ys_b.Offset, ys_b.Length, ys_b.Counter);
    if (hs_beg.Valid())
        hs_beg.Next();
    hs_end.Init(fss);
    hs_end.Restart(ys_e.Offset, ys_e.Length, ys_e.Counter);
    if (hs_end.Valid())
        hs_end.Next();

    SUPERLONG prevbeg = 0;
    SUPERLONG prevend = 0;
    ui32 doc = 0xffffffff;

    for(; hs_beg.Valid() && hs_end.Valid(); ++hs_beg, ++hs_end) {

        SUPERLONG wpCurBeg = hs_beg.Current() >> WORD_LEVEL_Shift;
        if (wpCurBeg == prevbeg) {
            doc = 0xffffffff;
            continue;
        }

        if (doc != 0xffffffff) {
            unsigned wordcount = unsigned(prevend - prevbeg);
            if (wordcount > 0) {
                ui32 word_n_break = ui32(prevbeg & ((1 << WB_Shift) - 1));
                AnchorScanner.AddZoneEnd(doc, word_n_break, wordcount);
            }
        }

        SUPERLONG wpCurEnd = hs_end.Current() >> WORD_LEVEL_Shift;
        doc = ui32(wpCurBeg >> WB_Shift);
        if (doc != (wpCurEnd >> WB_Shift))
            ythrow yexception() << "broken zones, " <<  doc;

        prevbeg = wpCurBeg;
        prevend = wpCurEnd;

    }

    if (doc != 0xffffffff) {
        ui32 wordcount = ui32(prevend - prevbeg);
        if (wordcount > 0) {
            ui32 word_n_break = ui32(prevbeg & ((1 << WB_Shift) - 1));
            AnchorScanner.AddZoneEnd(doc, word_n_break, wordcount);
        }
    }


    if (hs_beg.Valid() || hs_end.Valid())
        ythrow yexception() << "broken zones";
    fss.Close();
    return 0;
}

void AddZoneEnd(const char *yName)
{
    TYndex4Searching yndex;
    yndex.InitSearch(yName);

    TRequestContext rc;

    const YxRecord *anchor_b = ExactSearch(&yndex, rc, "(anchor");
    const YxRecord *anchor_e = ExactSearch(&yndex, rc, ")anchor");
    const YxRecord *anchorint_b = ExactSearch(&yndex, rc, "(anchorint");
    const YxRecord *anchorint_e = ExactSearch(&yndex, rc, ")anchorint");
    yndex.CloseSearch();

    fprintf(stderr, "Start the searching for anchor length, anchor: %" PRIi64 ", anchorint: %" PRIi64 ".\n", anchor_b->Counter, anchorint_b->Counter);
    TString fnInv = TString(yName) + "inv";

    if (anchor_b && anchor_e)
        load_anchors_positions(fnInv, *anchor_b, *anchor_e);
    if (anchorint_b && anchorint_e)
        load_anchors_positions(fnInv, *anchorint_b, *anchorint_e);
}

struct SWordProcessor
{
    int nForms;
    char* keytook[N_MAX_FORMS_PER_KISHKA];
    TKeyLemmaInfo keyLemma;
    char szForms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* curkey;

    SWordProcessor() {
        AfterKey();
    }
    bool BeforeKey(const char* key) {
        assert(*key != '#' && *key != '(' && *key != ')' && *key != '!');
        curkey = key;
        return true;
    }
    void AfterKey() {
        nForms = -1;
        memset(keytook, 0, sizeof(keytook));
        curkey = nullptr;
    }
    void Do(SUPERLONG pos) {
        SUPERLONG Cur = pos >> WORD_LEVEL_Shift;
        ui32 doc = ui32(Cur >> WB_Shift);
        ui32 word_n_break = ui32(Cur & ((1 << WB_Shift) - 1));

        ui32 aid = AnchorScanner.Find(doc, word_n_break);
        if (aid == (ui32)-1)
            return;
        if (nForms == -1)
            nForms = DecodeKey(curkey, &keyLemma, szForms);
        ui8 nForm = TWordPosition::Form(pos);
        assert(nForm < nForms);
        if (keytook[nForm] == nullptr)
            keytook[nForm] = AnchorScanner.AddFormaText(szForms[nForm]);
        AnchorScanner.AddWord(aid, word_n_break, keytook[nForm]);
    }
};

class TLinkFilter
{
private:
    TVector<char> szBuf;
    THashSet<const char*> urls;
public:
    void LoadSelectedUrls(const TString &szFile) {
        TFile F(szFile, OpenExisting | RdOnly);
        int nFileLength = (int)F.GetLength();
        szBuf.resize(nFileLength + 1);
        F.Read(&szBuf[0], nFileLength);
        szBuf[nFileLength] = 0;
        char *pszLine = &szBuf[0];
        for (char *pszChar = &szBuf[0]; true; ++pszChar) {
            bool bIsEof = *pszChar == 10 || *pszChar == 13;
            if (bIsEof || *pszChar == 0) {
                if (pszChar[-1] == '/')
                    pszChar[-1] = 0; // remove trailing '/' from URL
            }
            if (*pszChar == 0)
                break;
            if (bIsEof) {
                *pszChar = 0;
                if (*pszLine)
                    urls.insert(pszLine);
                pszLine = pszChar + 1;
            }
        }
        if (*pszLine)
            urls.insert(pszLine);
        if (urls.empty())
            ythrow yexception() << "bad selected urls " <<  szFile.c_str();
        fprintf(stderr, "Load %" PRISZT " urls from \"%s\".\n", urls.size(), szFile.data());
    }
    bool UsefulLink(const char *Link) const {
        return (Link != nullptr && (szBuf.empty() || SearchForNormalizeUrl(Link)));
    }
private:
    bool SearchForNormalizeUrl(const char *pSz) const {
        const int N_LARGE_BUF = 10000;
        char szBuf[N_LARGE_BUF];
        if (!NormalizeUrl(szBuf, N_LARGE_BUF, pSz))
            return false;
        return urls.find(szBuf) != urls.end();
    }
};

void PrintLinks(TLinkFilter& LinkFilter, const char *index)
{
    fprintf(stderr, "Start the output to stdout.\n");

    TSequentYandReader LinkReader;
    LinkReader.Init(index, "#link");
    while (LinkReader.Valid()) {
        const char* Key = LinkReader.CurKey().Text;
        const char* Link = nullptr;
        if (strncmp(Key + 5, "=\"", 2) == 0)
            Link = Key + 7;
        else if (strncmp(Key + 5, "int=\"", 5) == 0
       //  || strncmp(Key + 5, "mp3=\"", 5) == 0
         )
            Link = Key + 10;
        if (LinkFilter.UsefulLink(Link)) {
            TSequentPosIterator it(LinkReader);
            while (it.Valid()) {
                SUPERLONG Cur = it.Current() >> WORD_LEVEL_Shift;
                ui32 doc = ui32(Cur >> WB_Shift);
                ui32 word_n_break = ui32(Cur & ((1 << WB_Shift) - 1));

                ui32 aid = AnchorScanner.Find(doc, word_n_break);
                if (aid != (ui32)-1) {
                    printf("%s\t", Link);
                    AnchorScanner.PrintAnchor(doc, aid);
                    printf("\n");
                }
                ++it;
            }
        }
        LinkReader.Next();
    }
}

void CountLinks(TLinkFilter& LinkFilter, const char *index)
{
    TSequentYandReader LinkReader;
    LinkReader.Init(index, "#link");
    while (LinkReader.Valid()) {
        const char* Key = LinkReader.CurKey().Text;
        const char* Link = nullptr;
        if (strncmp(Key + 5, "=\"", 2) == 0)
            Link = Key + 7;
        else if (strncmp(Key + 5, "int=\"", 5) == 0
       //  || strncmp(Key + 5, "mp3=\"", 5) == 0
         )
            Link = Key + 10;
        if (LinkFilter.UsefulLink(Link)) {
            TSequentPosIterator it(LinkReader);
            while (it.Valid()) {
                SUPERLONG Cur = it.Current() >> WORD_LEVEL_Shift;
                ui32 doc = ui32(Cur >> WB_Shift);
                AnchorScanner.CountLink(doc);
                ++it;
            }
        }
        LinkReader.Next();
    }
}

void AddZoneBegin(TLinkFilter& LinkFilter, const char *index)
{
    TSequentYandReader LinkReader;
    LinkReader.Init(index, "#link");
    while (LinkReader.Valid()) {
        const char* Key = LinkReader.CurKey().Text;
        const char* Link = nullptr;
        if (strncmp(Key + 5, "=\"", 2) == 0)
            Link = Key + 7;
        else if (strncmp(Key + 5, "int=\"", 5) == 0
       //  || strncmp(Key + 5, "mp3=\"", 5) == 0
         )
            Link = Key + 10;
        if (LinkFilter.UsefulLink(Link)) {
            TSequentPosIterator it(LinkReader);
            while (it.Valid()) {
                SUPERLONG Cur = it.Current() >> WORD_LEVEL_Shift;
                ui32 doc = ui32(Cur >> WB_Shift);
                ui32 word_n_break = ui32(Cur & ((1 << WB_Shift) - 1));
                AnchorScanner.AddZoneBegin(doc, word_n_break);
                ++it;
            }
        }
        LinkReader.Next();
    }
}

static void die()
{
    fprintf(stderr, "printlinks - prints links and anchors from key/inv\n");
    fprintf(stderr, "Usage: printlinks -i selecturls indexprefix\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    TString IndexPrefix, SelectUrls;

    int optlet;
    class Opt opt (argc, argv, "i:");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'i':
            SelectUrls = opt.Arg;
            break;
        case '?':
        default:
            die();
            break;
        }
    }
    if (argc - opt.Ind < 1)
        die();
    IndexPrefix = argv[opt.Ind];

    if (!IndexPrefix)
        die();

    try {
        time_t t = time(nullptr);
        fprintf(stderr, "Task was started at %s", ctime(&t));
        TLinkFilter LinkFilter;
        if (SelectUrls.size())
            LinkFilter.LoadSelectedUrls(SelectUrls);

        AnchorScanner.Init(IndexPrefix.data());
        CountLinks(LinkFilter, IndexPrefix.data());
        AnchorScanner.AllocateAncs();
        AddZoneBegin(LinkFilter, IndexPrefix.data());
        AnchorScanner.Sort();
        AddZoneEnd(IndexPrefix.data());

        AnchorScanner.AddDocUrls(IndexPrefix.data());
        AnchorScanner.AllocateWords();
        SWordProcessor wp;
        ScanSeqKeys(IndexPrefix.data(), ")zzzzzzzzzzzzzzzzzzzzzzzz", wp);
        PrintLinks(LinkFilter, IndexPrefix.data());
        t = time(nullptr);
        fprintf(stderr, "Task was finished at %s", ctime(&t));
    } catch (std::exception& e) {
        fprintf(stderr, "%s", e.what());
        return 1;
    }
    return 0;
}
