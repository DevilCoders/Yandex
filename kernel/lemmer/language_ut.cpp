#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/stream/output.h>
#include <library/cpp/charset/wide.h>

#include <library/cpp/testing/unittest/registar.h>

#include <kernel/lemmer/core/language.h>

#include <kernel/lemmer/dictlib/grammar_index.h>

namespace NLanguageTest {
    // Auxiliary mapping of character categories to names
    class TCaseFlagMap: public TMap<TCharCategory, TString> {
    public:
        TCaseFlagMap() {
            (*this)[CC_ALPHA] = "ALPHA";
            (*this)[CC_NMTOKEN] = "NMTOKEN";
            (*this)[CC_NUMBER] = "NUMBER";
            (*this)[CC_NUTOKEN] = "NUTOKEN";
            (*this)[CC_ASCII] = "ASCII";
            (*this)[CC_NONASCII] = "NONASCII";
            (*this)[CC_TITLECASE] = "TITLECASE";
            (*this)[CC_UPPERCASE] = "UPPERCASE";
            (*this)[CC_LOWERCASE] = "LOWERCASE";
            (*this)[CC_MIXEDCASE] = "MIXEDCASE";
            (*this)[CC_COMPOUND] = "COMPOUND";
            (*this)[CC_HAS_DIACRITIC] = "HAS_DIACRITIC";
            (*this)[CC_DIFFERENT_ALPHABET] = "DIFFERENT_ALPHABET";
        }

        const char* Get(TCharCategory cc) const {
            const_iterator it = find(cc);
            if (it == end())
                return nullptr;
            return (*it).second.c_str();
        }
    };
    const TCaseFlagMap CaseFlags;

    const char* const RussianText = "В день рыбака в Йошкар-Оле начали собирать новый черно-белый телевизор ТЕМП";
    const char* RussianReferenceResult[] = {
        "#1 [1:1] в+ALPHA+NMTOKEN+NONASCII+TITLECASE+UPPERCASE: (Russian) в+PR{}",
        "#2 [1:1] день+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) день+S,m,inan{acc,sg|nom,sg}",
        "#3 [1:1] рыбака+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) рыбак+S,m,anim{acc,sg|gen,sg}",
        "#4 [1:1] в+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) в+PR{}",
        "#5 [1:1] йошкар+ALPHA+NMTOKEN+NONASCII+TITLECASE: (Russian bastard) йошкар+S,persn,m,anim{nom,sg}",
        "#5 [1:2] йошкар-оле+ALPHA+NMTOKEN+NONASCII+TITLECASE+COMPOUND: (Russian) йошкар-ола+S,geo,f,inan{abl,sg|dat,sg}",
        "#5 [2:2] оле+ALPHA+NMTOKEN+NONASCII+TITLECASE: (Russian) ола+S,f,inan{abl,sg|dat,sg}",
        "#5 [2:2] оле+ALPHA+NMTOKEN+NONASCII+TITLECASE: (Russian) оле+S,persn,m,anim{abl,pl|abl,sg|acc,pl|acc,sg|dat,pl|dat,sg|gen,pl|gen,sg|ins,pl|ins,sg|nom,pl|nom,sg}",
        "#5 [2:2] оле+ALPHA+NMTOKEN+NONASCII+TITLECASE: (Russian) оля+S,persn,f,anim{abl,sg|dat,sg}",
        "#6 [1:1] начали+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) начинать+V,tran{praet,pl,indic,pf}",
        "#7 [1:1] собирать+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) собирать+V{inf,ipf,tran}",
        "#8 [1:1] новый+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) новый+A{acc,sg,plen,m,inan|nom,sg,plen,m}",
        "#9 [1:1] черно+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) черно+ADV{}",
        "#9 [1:1] черно+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) черный+A{sg,brev,n}",
        "#9 [1:2] черно-белый+ALPHA+NMTOKEN+NONASCII+LOWERCASE+COMPOUND: (Russian) черно-белый+A{acc,sg,plen,m,inan|nom,sg,plen,m}",
        "#9 [2:2] белый+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) белый+A{acc,sg,plen,m,inan|nom,sg,plen,m}",
        "#9 [2:2] белый+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) белый+S,famn,m,anim{nom,sg}",
        "#10 [1:1] телевизор+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Russian) телевизор+S,m,inan{acc,sg|nom,sg}",
        "#11 [1:1] темп+ALPHA+NMTOKEN+NONASCII+UPPERCASE: (Russian) темп+S,m,inan{acc,sg|nom,sg}",
        ""};

    const char* const EnglishText = "But he-who-must-not-be-named can fly";
    const char* EnglishReferenceResult[] = {
        "#1 [1:1] but+ALPHA+NMTOKEN+ASCII+TITLECASE: (English) but+ADV{}",
        "#1 [1:1] but+ALPHA+NMTOKEN+ASCII+TITLECASE: (English) but+PR{}",
        "#2 [1:1] he+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) he+SPRO,irreg{nom}",
        "#2 [2:2] who+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) who+SPRO{nom}",
        "#2 [3:3] must+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) must+S{nom,sg}",
        "#2 [3:3] must+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) must+A{}",
        "#2 [3:3] must+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) must+V{praes,indic|inf}",
        "#2 [4:4] not+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) not+ADV{}",
        "#2 [5:5] be+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) be+V,irreg{inf}",
        "#2 [6:6] named+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) name+V{praet,indic|praet,partcp}",
        "#2 [6:6] named+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) named+A{}",
        "#3 [1:1] can+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) can+S{nom,sg}",
        "#3 [1:1] can+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) can+V,irreg{praes,indic|inf}",
        "#3 [1:1] can+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) can+V{praes,indic|inf}",
        "#4 [1:1] fly+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) fly+S{nom,sg}",
        "#4 [1:1] fly+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) fly+V{praes,indic|inf}",
        "#4 [1:1] fly+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) fly+A{}",
        ""};

    const char* const UkrainianText = "Обов'язково мий руки перед їдою інакше захворієш";
    const char* UkrainianReferenceResult[] = {
        "#1 [1:1] обов+ALPHA+NMTOKEN+NONASCII+TITLECASE: (Ukrainian foundling) обов+",
        "#1 [1:2] обов'язково+ALPHA+NMTOKEN+NONASCII+TITLECASE+COMPOUND: (Ukrainian) обов'язково+ADV{}",
        "#1 [2:2] язково+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian bastard) язково+ADV{}",
        "#2 [1:1] мий+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) мити+V,ipf{sg,imper,2p}",
        "#3 [1:1] руки+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) рука+S,f{acc,pl|gen,sg|voc,pl|nom,pl}",
        "#4 [1:1] перед+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) перед+PR{}",
        "#5 [1:1] їдою+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) їда+S,sg,f{ins}",
        "#6 [1:1] інакше+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) інакше+ADV{}",
        "#6 [1:1] інакше+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) інакший+A{acc,sg,n|nom,sg,n}",
        "#7 [1:1] захворієш+ALPHA+NMTOKEN+NONASCII+LOWERCASE: (Ukrainian) захворіти+V,pf{inpraes,sg,2p}",
        ""};

    const char* const PolishText = "W ogóle nie wiem dlaczego trzeba coś tutaj napisać";
    const char* PolishReferenceResult[] = {
        "#1 [1:1] w+ALPHA+NMTOKEN+ASCII+TITLECASE+UPPERCASE: (Polish) w+PR{}",
        "#2 [1:1] ogóle+ALPHA+NMTOKEN+LOWERCASE+HAS_DIACRITIC: (Polish) ogół+S,m,inan{voc,sg|abl,sg}",
        "#3 [1:1] nie+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) niego+S,sg{acc}",
        "#3 [1:1] nie+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) nich+S,pl{acc}",
        "#3 [1:1] nie+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) nie+PART{}",
        "#4 [1:1] wiem+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) wiedzieć+V,ipf{praes,sg,indic,1p}",
        "#5 [1:1] dlaczego+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) dlaczego+PART{}",
        "#6 [1:1] trzeba+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) trzeba+V{}",
        "#7 [1:1] coś+ALPHA+NMTOKEN+LOWERCASE+HAS_DIACRITIC: (Polish) coś+SPRO,sg{nom|acc}",
        "#8 [1:1] tutaj+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) tutaj+PART{}",
        "#9 [1:1] napisać+ALPHA+NMTOKEN+LOWERCASE+HAS_DIACRITIC: (Polish) napisać+V,pf{inf}",
        ""};

    const char* const UnknownText = "Grau mein Freund ist alle Theorie doch grün des Lebens goldner Baum";
    const char* UnknownReferenceResult[] = {
        "#1 [1:1] grau+ALPHA+NMTOKEN+ASCII+TITLECASE: (Unknown foundling) grau+",
        "#2 [1:1] mein+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) mein+",
        "#3 [1:1] freund+ALPHA+NMTOKEN+ASCII+TITLECASE: (Unknown foundling) freund+",
        "#4 [1:1] ist+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) ist+",
        "#5 [1:1] alle+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) alle+",
        "#6 [1:1] theorie+ALPHA+NMTOKEN+ASCII+TITLECASE: (Unknown foundling) theorie+",
        "#7 [1:1] doch+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) doch+",
        "#8 [1:1] grün+ALPHA+NMTOKEN+LOWERCASE+HAS_DIACRITIC: (Unknown foundling) gruen+",
        "#9 [1:1] des+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) des+",
        "#10 [1:1] lebens+ALPHA+NMTOKEN+ASCII+TITLECASE: (Unknown foundling) lebens+",
        "#11 [1:1] goldner+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Unknown foundling) goldner+",
        "#12 [1:1] baum+ALPHA+NMTOKEN+ASCII+TITLECASE: (Unknown foundling) baum+",
        ""};

    const char* const RussianUkrainianText = "ТАКСИ";
    const char* RussianUkrainianReferenceResult[] = {
        "#1 [1:1] такси+ALPHA+NMTOKEN+NONASCII+UPPERCASE: (Russian) такси+S,n,inan{abl,pl|abl,sg|acc,pl|acc,sg|dat,pl|dat,sg|gen,pl|gen,sg|ins,pl|ins,sg|nom,pl|nom,sg}",
        "#1 [1:1] такси+ALPHA+NMTOKEN+NONASCII+UPPERCASE: (Ukrainian) такса+S,f{acc,pl|gen,sg|voc,pl|nom,pl}",
        "#1 [1:1] такси+ALPHA+NMTOKEN+NONASCII+UPPERCASE: (Ukrainian) такса+S,f{gen,sg|voc,pl|nom,pl}",
        ""};

    const char* const EnglishPolishText = "ran spice";
    const char* EnglishPolishReferenceResult[] = {
        "#1 [1:1] ran+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) run+V,irreg{praet,indic}",
        "#1 [1:1] ran+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) rana+S,f{gen,pl}",
        "#1 [1:1] ran+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish) rano+S,n{gen,pl}",
        "#2 [1:1] spice+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) spice+V{praes,indic|inf}",
        "#2 [1:1] spice+ALPHA+NMTOKEN+ASCII+LOWERCASE: (English) spice+S{nom,sg}",
        "#2 [1:1] spice+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish bastard) spica+S,f{acc,pl|nom,pl|voc,pl}",
        "#2 [1:1] spice+ALPHA+NMTOKEN+ASCII+LOWERCASE: (Polish bastard) spika+S,f{dat,sg|abl,sg}",
        ""};

    class TLanguageTest: public TTestBase {
        UNIT_TEST_SUITE(TLanguageTest);
        UNIT_TEST(TestRussian);
        UNIT_TEST(TestEnglish);
        UNIT_TEST(TestUkrainian);
        UNIT_TEST(TestPolish);
        UNIT_TEST(TestUnknown);
        UNIT_TEST(TestDistinctLanguages);
        UNIT_TEST(TestIntersectingLanguages);
        UNIT_TEST(TestLanguageAttributes);
        UNIT_TEST(TestClassifyLanguage);
        UNIT_TEST(TestEmptyRequest);
        UNIT_TEST_SUITE_END();

    public:
        void TestRussian() {
            UNIT_ASSERT(CheckMorphology(LANG_RUS, RussianText, RussianReferenceResult));
        }
        void TestEnglish() {
            UNIT_ASSERT(CheckMorphology(LANG_ENG, EnglishText, EnglishReferenceResult));
        }
        void TestUkrainian() {
            UNIT_ASSERT(CheckMorphology(LANG_UKR, UkrainianText, UkrainianReferenceResult));
        }
        void TestPolish() {
            UNIT_ASSERT(CheckMorphology(LANG_POL, PolishText, PolishReferenceResult));
        }
        void TestUnknown() {
            UNIT_ASSERT(CheckMorphology(LANG_UNK, UnknownText, UnknownReferenceResult));
        }

        void TestDistinctLanguages() {
            // Languages that have distinct character set should not be affected by presence of other languages in the language mask
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_ENG, LANG_POL, LANG_RUS), RussianText, RussianReferenceResult));
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_ENG, LANG_RUS, LANG_UKR), EnglishText, EnglishReferenceResult));
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_ENG, LANG_POL, LANG_UKR), UkrainianText, UkrainianReferenceResult));
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_POL, LANG_RUS, LANG_UKR), PolishText, PolishReferenceResult));
        }

        void TestIntersectingLanguages() {
            // Languages with intersecting character sert could get a disjunction of analyses
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_RUS, LANG_UKR), RussianUkrainianText, RussianUkrainianReferenceResult));
            UNIT_ASSERT(CheckMorphology(TLangMask(LANG_ENG, LANG_POL), EnglishPolishText, EnglishPolishReferenceResult));
        }

        void TestLanguageAttributes() {
            // Check that all methods of accessing the language give consistent results
            const TLanguage* lang_Unknown = NLemmer::GetLanguageById(LANG_UNK);
            UNIT_ASSERT(lang_Unknown != nullptr);
            UNIT_ASSERT(lang_Unknown == NLemmer::GetLanguageByName("Unknown"));
            UNIT_ASSERT(lang_Unknown == NLemmer::GetLanguageByName("mis"));

            const TLanguage* lang_Russian = NLemmer::GetLanguageById(LANG_RUS);
            UNIT_ASSERT(lang_Russian != nullptr);
            UNIT_ASSERT(lang_Russian == NLemmer::GetLanguageByName("russian"));
            UNIT_ASSERT(lang_Russian == NLemmer::GetLanguageByName("ru"));
            UNIT_ASSERT(lang_Russian == NLemmer::GetLanguageByName("rus"));
            UNIT_ASSERT(lang_Russian == NLemmer::GetLanguageByName("ru-RU"));

            const TLanguage* lang_English = NLemmer::GetLanguageById(LANG_ENG);
            UNIT_ASSERT(lang_English != nullptr);
            UNIT_ASSERT(lang_English == NLemmer::GetLanguageByName("ENGLISH"));
            UNIT_ASSERT(lang_English == NLemmer::GetLanguageByName("en"));
            UNIT_ASSERT(lang_English == NLemmer::GetLanguageByName("eng"));
            UNIT_ASSERT(lang_English == NLemmer::GetLanguageByName("en-US"));
            UNIT_ASSERT(lang_English == NLemmer::GetLanguageByName("en-GB"));

            const TLanguage* lang_Ukrainian = NLemmer::GetLanguageById(LANG_UKR);
            UNIT_ASSERT(lang_Ukrainian != nullptr);
            UNIT_ASSERT(lang_Ukrainian == NLemmer::GetLanguageByName("UKrainian"));
            UNIT_ASSERT(lang_Ukrainian == NLemmer::GetLanguageByName("uk"));
            UNIT_ASSERT(lang_Ukrainian == NLemmer::GetLanguageByName("ukr"));
            UNIT_ASSERT(lang_Ukrainian == NLemmer::GetLanguageByName("uk-UA"));

            const TLanguage* lang_Polish = NLemmer::GetLanguageById(LANG_POL);
            UNIT_ASSERT(lang_Polish != nullptr);
            UNIT_ASSERT(lang_Polish == NLemmer::GetLanguageByName("poLIsh"));
            UNIT_ASSERT(lang_Polish == NLemmer::GetLanguageByName("pl"));
            UNIT_ASSERT(lang_Polish == NLemmer::GetLanguageByName("pol"));
        }

        void TestClassifyLanguage() {
            const char* canon = "canon";
            const char* sapop = "сапоп";

            TUtf16String can = UTF8ToWide(canon);
            TUtf16String sap = UTF8ToWide(sapop);

            TLangMask id_lw = NLemmer::ClassifyLanguage(can.data(), can.size());
            TLangMask id_lwz = NLemmer::ClassifyLanguage(can.data(), can.size() + 1);
            TLangMask id_cw = NLemmer::ClassifyLanguage(sap.data(), sap.size());

            UNIT_ASSERT(id_lw.any());
            UNIT_ASSERT(id_lw.Test(LANG_ENG));

            UNIT_ASSERT(id_cw.any());
            UNIT_ASSERT(id_cw.Test(LANG_RUS));

            UNIT_ASSERT(id_lwz.none());

            UNIT_ASSERT((id_lw & id_cw).none());
        }

        void TestEmptyRequest() {
            TWLemmaArray lemmas;
            const TUtf16String emptyString;
            NLemmer::TAnalyzeWordOpt opt;
            NLemmer::AnalyzeWord(emptyString.c_str(), 0, lemmas, TLangMask(LANG_RUS), nullptr, opt);
            UNIT_ASSERT_EQUAL(lemmas.size(), 0);
        }

    protected:
        struct LessIgnoreSpace {
            bool operator()(const TString& st1, const TString& st2) const {
                const char* const spaces = " \t\r\n";
                const char* s1 = st1.c_str();
                const char* s2 = st2.c_str();
                // Run through the two lines and compares everything but spaces
                while (*s1 && *s2) {
                    if (strchr(spaces, *s1)) {
                        if (!strchr(spaces, *s2))
                            return true;
                        s1 += strspn(s1, spaces);
                        s2 += strspn(s2, spaces);
                    } else if (strchr(spaces, *s2))
                        return false;
                    if (*s1 != *s2)
                        return *s1 < *s2;
                    ++s1, ++s2;
                }
                s1 += strspn(s1, spaces);
                s2 += strspn(s2, spaces);
                return !*s1 && *s2;
            }
        };

        typedef TSet<TString, LessIgnoreSpace> TResSet;

        bool CheckMorphology(TLangMask langmask, const char* text, const char* reference[]) {
            const char* const word_separators = " \t\r\n";
            const char* const token_separators = "'-";
            const TUtf16String tokenSeparatorsUtf16 = UTF8ToWide(token_separators);

            NLemmer::TAnalyzeWordOpt opt;
            // Tokenize the text by spaces
            size_t len = 0;
            int i = 0;
            TVector<TResSet> result;
            while (true) {
                // Separate the word
                text += len;
                text += strspn(text, word_separators);
                len = strcspn(text, word_separators);
                if (!len)
                    break;
                i++;

                TWLemmaArray lemmas;
                TUtf16String buf = UTF8ToWide(TStringBuf(text, len));
                if (strcspn(text, token_separators) >= len) {
                    // single-token
                    NLemmer::AnalyzeWord(buf.data(), buf.size(), lemmas, langmask, nullptr, opt);
                } else {
                    // multi-token
                    TWideToken word;
                    word.Token = buf.data();
                    word.Leng = buf.size();
                    ;

                    // Parse the word into tokens - just split by dashes and apostrophes
                    size_t offset = 0;
                    for (size_t pos = 0; pos <= buf.size(); ++pos) {
                        bool isBreak = pos == buf.size();
                        for (wchar16 c : tokenSeparatorsUtf16) {
                            if (buf[pos] == c) {
                                isBreak = true;
                                break;
                            }
                        }
                        if (isBreak) {
                            word.SubTokens.push_back(offset, pos - offset);
                            offset = pos + 1;
                        }
                    }
                    NLemmer::AnalyzeWord(word, lemmas, langmask, nullptr, opt);
                }
                result.resize(i);
                PrintLemmas(result[i - 1], lemmas);
            }
            return CompareRes(result, reference);
        }

        void printbad(const TResSet& res, const char** ref) {
            Cerr << Endl << "EXPECTED:" << Endl;
            assert(**ref == '#');
            int num = atoi(*ref + 1);
            while (**ref) {
                const char* s = *ref;
                assert(*s == '#');
                ++s;
                if (atoi(s) != num)
                    break;
                while (isdigit((unsigned char)*s))
                    ++s;
                while (isspace((unsigned char)*s))
                    ++s;
                Cerr << "\t" << s << Endl;
                ++ref;
            }

            Cerr << "RECEIVED:" << Endl;
            for (const auto& re : res) {
                Cerr << "\t" << re << Endl;
            }
        }

        bool CompareRes(const TVector<TResSet>& res, const char** reference) {
            int old_num = 0;
            size_t cnt = 0;
            const char** beg = reference;
            while (**reference) {
                const char* s = *reference;
                assert(*s == '#');
                ++s;
                int num = atoi(s);
                if (num != old_num) {
                    if (old_num > 0 && res[old_num - 1].size() != cnt) {
                        printbad(res[old_num - 1], beg);
                        return false;
                    }
                    old_num = num;
                    cnt = 0;
                    beg = reference;
                }
                while (isdigit((unsigned char)*s))
                    ++s;
                while (isspace((unsigned char)*s))
                    ++s;
                ++cnt;
                if (res[num - 1].find(TString(s)) == res[num - 1].end()) {
                    printbad(res[num - 1], beg);
                    return false;
                }
                ++reference;
            }
            return true;
        }

        // Produce a string representation of the morphological analysis, for reference
        void PrintLemmas(TResSet& result, const TWLemmaArray& lemmas) {
            for (size_t i = 0; i < lemmas.size(); i++) {
                TStringStream out;
                const TYandexLemma& lemma = lemmas[i];

                size_t first = lemma.GetTokenPos() + 1;
                size_t last = lemma.GetTokenPos() + lemma.GetTokenSpan();

                out << "[" << (int)first << ":" << (int)last << "] ";

                out << lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength();
                for (int k = 0; k < 32; k++) {
                    TCharCategory flag = (1 << k);
                    if (lemma.GetCaseFlags() & flag) {
                        const char* flagname = CaseFlags.Get(flag);
                        UNIT_ASSERT(flagname != nullptr);
                        out << "+" << flagname;
                    }
                }
                out << ": (" << FullNameByLanguage(lemma.GetLanguage());
                if (lemma.GetQuality() & TYandexLemma::QFoundling)
                    out << " foundling";
                else if (lemma.IsBastard())
                    out << " bastard";
                out << ") ";
                out << lemma.GetText() << "+";
                if (lemma.GetStemGram())
                    out << sprint_grammar(lemma.GetStemGram());
                if (lemma.FlexGramNum()) {
                    out << "{";
                    for (size_t k = 0; k < lemma.FlexGramNum(); k++) {
                        if (k)
                            out << "|";
                        out << sprint_grammar(lemma.GetFlexGram()[k]);
                    }
                    out << "}";
                }
                result.insert(out.Str());
            }
        }
    };

    UNIT_TEST_SUITE_REGISTRATION(TLanguageTest);
}
