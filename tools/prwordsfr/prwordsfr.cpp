/***************************************************************************
 ***************************************************************************
 * рТПЗТБННБ РЕЮБФБЕФ УРЙУПЛ УМПЧ Й ЛПМЙЮЕУФЧП ДПЛХНЕОФПЧ,
 * Ч ЛПФПТЩИ ПОЙ ЧУФТЕЮБАФУС
 ***************************************************************************
 * Usage: $0 <yandex> <doc_ids_list_fname> [fl_print_null_fr=0 [words_list_fname]]
 * yandex             - РТЕЖЙЛУ ЙНЕО ЙОДЕЛУОЩИ ЖБКМПЧ (workindex/index)
 * doc_ids_list_fname - ЙНС ЖБКМБ УП УРЙУЛПН ИЕОДМПЧ РТПЧЕТСЕНЩИ ДПЛХНЕОФПЧ
 * fl_print_null_fr=1   РЕЮБФБФШ ОХМЕЧХА ЮБУФПФХ (РП ХНПМЮБОЙА ОЕ РЕЮБФБЕФУС)
 * words_list_fname   - ЙНС ЖБКМБ УП УРЙУЛПН РТПЧЕТСЕНЩИ УМПЧ (ЕУМЙ ОЕ
 *                      ЪБДБО, РТПЧЕТСАФУС ЧУЕ УМПЧБ ЙЪ ЙОДЕЛУБ)
 ***************************************************************************/

#include <kernel/search_types/search_types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <map>
#include <set>

#include <util/system/compat.h>
#include <library/cpp/containers/atomizer/atomizer.h>

#include <library/cpp/wordpos/wordpos.h>
#include <kernel/keyinv/indexfile/seqreader.h>

using namespace std;

atomizer<> wordlist;
int nword;
int fl_print_null_fr = 0; // РЕЮБФБФШ ОХМЕЧХА ЮБУФПФХ
int fl_doc_info = 0; // РЕЮБФБФШ ЙОЖПТНБГЙА РП ПФДЕМШОПУФЙ ДМС ЛБЦДПЗП ДПЛХНЕОФБ

int load_wordlist(const char* wordlist_fname) {
    FILE *Wordlist = fopen(wordlist_fname, "r");
    if (Wordlist == nullptr)
        err(1, "%s", wordlist_fname);
    char buf[4096];
    while (fgets(buf, sizeof(buf), Wordlist)) {
        char *s = strchr(buf, '\n');
        if (s != nullptr)
            *s = 0;
        nword++;
        wordlist.string_to_atom(buf);
    }
    fclose(Wordlist);
    return 0;
}

set<ui32> docset;

int load_docset(const char *fn) {
    FILE *Docset = fopen(fn, "r");
    if (Docset == nullptr)
        err(1, "docset %s", fn);
    ui32 d;
    while (fscanf(Docset, "%d", &d) == 1) {
        docset.insert(d);
    }
    fclose(Docset);
    return 0;
}

struct Int {
    int a;
    Int() : a(0) {}
};

map<ui32, Int> fr;
char curkey[MAXKEY_BUF];
ui16 *maxfr;

static void dump_curkey() {
    int df = 0;
    if (curkey[0] == 0)
        return;
    for (map<ui32, Int>::iterator i = fr.begin(); i != fr.end(); i++) {
        if (docset.count((*i).first) == 0)
            continue;
        if ((*i).second.a) {
            if (fl_doc_info)
                printf("%d %s:%d\n", (*i).first, curkey, (*i).second.a);
            df++;
        }
    }
    if (df != 0 || fl_print_null_fr)
        printf("%s\t%d\n", curkey, df);
}

void print_df(const char *fnYndex) {
    TSequentYandReader syr;
    char highkey[MAXKEY_BUF];
    memset(highkey, 0xff, MAXKEY_BUF-1);
    highkey[MAXKEY_BUF-1] = 0;
    syr.Init(fnYndex, "0", highkey);
    for (; syr.Valid(); syr.Next()) {
        const YxKey &entry = syr.CurKey();
        if (!entry.Text)
            continue;
        if (strpbrk(entry.Text, " \t\n"))
            continue;
        int l = strcspn(entry.Text, "\1");
        if (strncmp(curkey, entry.Text, l)) {
            if (wordlist.empty() || wordlist.find_atom(curkey))
                dump_curkey();
            //memset(fr.begin(), 0, fr.size() * sizeof(fr[0]));
            fr.clear();
            strncpy(curkey, entry.Text, l);
            curkey[l] = 0;
        }
        if (!wordlist.empty() && !wordlist.find_atom(curkey))
            continue;
        for (TSequentPosIterator it(syr); it.Valid(); ++it) {
            TWordPosition wp = *it;
            fr[wp.Doc()].a++;
        }
    }
    if (wordlist.empty() || wordlist.find_atom(curkey))
        dump_curkey();
}


int main(int argc, char *argv[]) {
    if (argc < 3)
        errx(1, "Usage: %s <yandex> <doc_ids_list_fname> [fl_print_null_fr={no|yes} [words_list_fname]]\n", argv[0]);

    load_docset(argv[2]);

    if (argc > 3 && strncmp(argv[3], "yes", 3) == 0)
        fl_print_null_fr = 1;

    if (argc > 4)
        load_wordlist(argv[4]);

    if (argc > 5 && strncmp(argv[5], "yes", 3) == 0)
        fl_doc_info = 1;

    print_df(argv[1]);

    return 0;
}
