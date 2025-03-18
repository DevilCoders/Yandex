#include <kernel/search_types/search_types.h>
#include <kernel/tarc/disk/searcharc.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/xref/xmap.h>

#include <ysite/yandex/srchmngr/yrequester.h>

#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/hitlist/positerator.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <library/cpp/charset/recyr.hh>
#include <util/folder/dirut.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/system/filemap.h>
#include <util/generic/noncopyable.h>

struct TIndexFiles : TNonCopyable
{
    THolder<TSearchArchive> Archive;
    TXRefMapped2DArray AnchorXRefData;
    TSparsedArray<ui32> *DocRemap;

    TIndexFiles() : DocRemap(nullptr) {}

    ~TIndexFiles() {
        delete DocRemap;
    }

    bool Open(const TString& indexName, bool dontUseArchive)
    {
        if (!AnchorXRefData.Init((indexName + ".refxmap").c_str(), true)) {
            printf("Failed to open xmap\n");
            return false;
        }

        if (!dontUseArchive) {
            THolder<TSearchArchive> archive(new TSearchArchive());
            archive->Open(indexName.c_str());
            if (!archive->IsOpen()) {
                Cerr << "Warning: failed to open archive" << Endl;
            } else {
                archive.Swap(Archive);
            }

            TString indexarr = indexName + "arr";

            if (NFs::Exists(indexarr)) {
                DocRemap = new TSparsedArray<ui32>(DOC_LEVEL_Bits + 1);
                if (!DocRemap->Load(indexarr.c_str())) {
                    printf("failed to open arr file\n");
                    return false;
                }
            }
        }

        return true;
    }
};

static TString GetDocumentUrl(const TIndexFiles &ff, int docId) {
    int arcDocId = ff.DocRemap ? ff.DocRemap->GetAt(docId) : docId;

    const size_t N_NORMAL_URL_BUFSIZE = 10000;
    const char* pszUrl = "";

    TDocArchive DA;
    TDocDescr DocD;

    if (ff.Archive.Get() && !ff.Archive->FindDocBlob(arcDocId, DA)) {
        if (DA.Exists) {
            DocD.UseBlob(DA.GetBlob(), DA.Blobsize);
            if (DocD.IsAvailable())
                pszUrl = DocD.get_url();
        }
    }
    char szNormalUrl[N_NORMAL_URL_BUFSIZE];
    if (!NormalizeUrl(szNormalUrl, N_NORMAL_URL_BUFSIZE, pszUrl))
        strlcpy(szNormalUrl, pszUrl, N_NORMAL_URL_BUFSIZE - 1);

    return szNormalUrl;
}

TString GetFormText(const TKeyLemmaInfo& lemma, const char* form) {
    TString forma = TString::Join(lemma.szPrefix, form);
    forma = Recode(CODES_YANDEX, CODES_WIN, forma.data());

    if (forma.empty()) {
        ythrow yexception() << "Empty key form";
    }

    if (forma.back() == TITLECASECOLLATOR) {
        forma = TString(1, CodePageByCharset(CODES_WIN)->ToUpper(forma[0])) + forma.substr(1, forma.size() - 2);
    }

    return forma;
}

static void SinglePass(const TString& indexName,
                       const THashMap<int, TString> &owners,
                       const TIndexFiles &ff, int startDoc, int docCount, int *maxDoc)
{
    TVector< TVector< TVector<int> > > linkText;
    TVector<TString> forms;

    NIndexerCore::TIndexReader ir((indexName + ".refkey").c_str(), (indexName + ".refinv").c_str(), IYndexStorage::FINAL_FORMAT);

    while (ir.ReadKey()) {
        if (*ir.GetKeyText() == '#') {
            continue;
        }

        TKeyLemmaInfo lemma;
        char szForms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
        int nForms = DecodeKey(ir.GetKeyText(), &lemma, szForms);

        if (nForms <= 0) {
            continue;
        }

        int keyForms[N_MAX_FORMS_PER_KISHKA];
        memset(keyForms, -1, sizeof(keyForms));

        TPosIterator<> it;
        ir.InitPosIterator(it);
        it.SkipTo(TWordPosition(startDoc, 0).SuperLong());

        for (; it.Valid(); ++it) {
            TWordPosition wp = *it;

            int docId = wp.Doc();
            if (docId < startDoc) {
                continue;
            }

            *maxDoc = std::max(*maxDoc, docId);
            if (docId >= startDoc + docCount) {
                break;
            }

            int posForm = wp.Form();
            if (posForm >= nForms) {
                ythrow yexception() << "Wrong form number";
            }

            if (keyForms[posForm] == -1) {
                keyForms[posForm] = (int)forms.size();
                TString formText = GetFormText(lemma, szForms[posForm]);
                forms.push_back(formText);
            }
            int forma = keyForms[posForm];
            assert(forma != -1);

            docId -= startDoc;
            if (docId >= (int)linkText.size())
                linkText.resize(docId + 1);
            TVector< TVector<int> > &dstDoc = linkText[docId];

            int linkId = wp.Break();
            if (linkId >= (int)dstDoc.size())
                dstDoc.resize(linkId + 1);
            TVector<int> &dst = dstDoc[linkId];

            int wordId = wp.Word();
            if (wordId >= (int)dst.size())
                dst.resize(wordId + 1, -1);
            int &dstForma = dst[wordId];

            if (dstForma != -1) {
                const TString& currentForm = forms[dstForma];
                const TString& newForm = forms[forma];
                if (newForm != currentForm) {
                    TString lowNew = ToLower(newForm, *CodePageByCharset(CODES_WIN));
                    TString lowCur = ToLower(currentForm, *CodePageByCharset(CODES_WIN));
                    if (lowNew.find(lowCur) != TString::npos) {
                        dstForma = forma;
                    } else if (lowCur.find(lowNew) == TString::npos) {
                        TString errorText = Sprintf("There are more than one form for one link word: '%s' vs '%s'", currentForm.data(), newForm.data());
                        ythrow yexception() << errorText;
                    }
                }
            }
            dstForma = forma;
        }
    }

    for (size_t docId = 0; docId < linkText.size(); ++docId) {
        TVector< TVector<int> > &docLinks = linkText[docId];
        for (size_t linkId = 1; linkId < docLinks.size(); ++linkId) {
            const TXMapInfo &xmap = (ff.AnchorXRefData.GetAt(docId + startDoc, linkId - 1));

            printf("%ld\t%ld\t", static_cast<long>(docId + startDoc), static_cast<long>(linkId));
            printf("%s\t", GetDocumentUrl(ff, docId + startDoc).c_str());

            THashMap<int, TString>::const_iterator ownit = owners.find(xmap.LinkInfo.OwnerFromId);
            if (ownit == owners.end())
                printf("\t");
            else
                printf("%s\t", ownit->second.c_str());

            const TVector<int>& linkWords = docLinks[linkId];
            for (size_t i = 1; i < linkWords.size(); ++i) {
                if (i != 1) {
                    printf(" ");
                }
                int formaId = linkWords[i];
                if (formaId == -1) {
                    // There are may be no key for appropriate link index entry.
                    // For example 'http' and 'www' keys are filtered from links that looks like URL.
                    printf("*");

                } else {
                    printf("%s", forms[formaId].data());
                }
            }

            printf("\t0");
            printf("\t%d", static_cast<int>(xmap.LinkInfo.HasGoodlink));
            printf("\t%d", static_cast<int>(xmap.LinkInfo.NewCommercialWeight));
            printf("\t%d", static_cast<int>(xmap.LinkInfo.NewCommercialLength));
            printf("\t%d", static_cast<int>(xmap.LinkInfo.Date));
            printf("\n");
        }
    }
}

void PrintUsage() {
    Cerr << "printxref [-a] [-d docid] index" << Endl;
    Cerr << "-a - don't use archive for finding document URL" << Endl;
    Cerr << "-d - print xref only for selected document" << Endl;
}

int main(int argc, char *argv[])
{
    bool dontUseArchive = false;
    ui32 docId = static_cast<ui32>(-1);

    int optlet;
    Opt opt(argc, argv, "ad:");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
            case 'a':
                dontUseArchive = true;
                break;

            case 'd':
                docId = FromString<ui32>(opt.Arg);
                break;

            default:
                PrintUsage();
                return 1;
        }
    }

    if (argc - opt.Ind != 1) {
        PrintUsage();
        return 1;
    }

    TString indexName = argv[opt.Ind];

    TIndexFiles ff;
    if (!ff.Open(indexName, dontUseArchive))
        return -1;

    THashMap<int, TString> owners;
    if (NFs::Exists("owners.txt")) {
        TFileInput f("owners.txt");
        TString line;
        while (f.ReadLine(line)) {
            char* buf = line.begin();
            char *p = strchr(buf, ' ');
            if (p) {
                *p = 0;
                ++p;
                owners[atoi(buf)] = p;
            }
        }
    }

    int maxDoc = 1;
    const int N_STEP = 50000;
    if (docId != static_cast<ui32>(-1)) {
        SinglePass(indexName, owners, ff, docId, 1, &maxDoc);

    } else {
        for (int startDoc = 0; startDoc < maxDoc; startDoc += N_STEP) {
            SinglePass(indexName, owners, ff, startDoc, N_STEP, &maxDoc);
        }
    }
    return 0;
}
