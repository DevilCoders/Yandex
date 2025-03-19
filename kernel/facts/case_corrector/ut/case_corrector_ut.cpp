#include <kernel/facts/case_corrector/case_corrector.h>

#include <library/cpp/testing/unittest/registar.h>

TCompactTrie<char, TString> MakePretty(const TVector<TString>& phrases) {
    TCompactTrieBuilder<char, TString> builder;
    for (const TString& phrase : phrases) {
        TUtf16String temp = UTF8ToWide(phrase);
        temp.to_lower();
        TString lowerPhrase = WideToUTF8(temp);
        TStringBuf phrasePart = lowerPhrase;
        builder.Add(phrasePart, phrase);
        while (phrasePart.RNextTok(" ")) {
            if (phrasePart.length() > 0 && !builder.Find(phrasePart))
                builder.Add(phrasePart, "");
        }
    }
    TBufferOutput out;
    builder.Save(out);
    return TCompactTrie<char, TString>(TBlob::FromBuffer(out.Buffer()));
}

TCompactTrie<char, TString> MakeDecapsable(const TVector<TString>& phrases) {
    TCompactTrieBuilder<char, TString> builder;
    for (const TString& phrase : phrases) {
        TUtf16String temp = UTF8ToWide(phrase);
        temp.to_upper();
        TString lowerPhrase = WideToUTF8(temp);
        TStringBuf phrasePart = lowerPhrase;
        builder.Add(phrasePart, phrase);
        while (phrasePart.RNextTok(" ")) {
            if (phrasePart.length() > 0 && !builder.Find(phrasePart))
                builder.Add(phrasePart, "");
        }
    }
    TBufferOutput out;
    builder.Save(out);
    return TCompactTrie<char, TString>(TBlob::FromBuffer(out.Buffer()));
}

using namespace NFacts;

Y_UNIT_TEST_SUITE(TCaseCorrector) {
    Y_UNIT_TEST(EmptyString) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "ИП"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "");
    }

    Y_UNIT_TEST(NamingCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Компания 'РОГА И КОПЫТА'.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Компания 'Рога и копыта'.");
    }

    Y_UNIT_TEST(TwoNamingCapsInSentence) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Компания 'РОГА И КОПЫТА' конкурирует с компанией 'И КОПЫТА, И РОГА'.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Компания 'Рога и копыта' конкурирует с компанией 'И копыта, и рога'.");
    }

    Y_UNIT_TEST(DifferentNamingQoutes) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Компания 'РОГА И КОПЫТА', компания \"РОГА И КОПЫТА\", компания «РОГА И КОПЫТА».";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Компания 'Рога и копыта', компания \"Рога и копыта\", компания «Рога и копыта».");
    }

    Y_UNIT_TEST(SentenceCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ЗАТЯЖКУ ОСЛАБЛЯЮТ РАВНОМЕРНО – ПО ОДНОМУ ОБОРОТУ КЛЮЧА НА КАЖДЫЙ.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Затяжку ослабляют равномерно – по одному обороту ключа на каждый.");
    }

    Y_UNIT_TEST(TwoSentencesCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "СОДЕРЖАНИЯ ОБЩЕГО ИМУЩЕСТВА В МНОГОКВАРТИРНОМ ДОМЕ. И ПРАВИЛ ИЗМЕНЕНИЯ РАЗМЕРА ПЛАТЫ ЗА СОДЕРЖАНИЕ И РЕМОНТ.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Содержания общего имущества в многоквартирном доме. И правил изменения размера платы за содержание и ремонт.");
    }

    Y_UNIT_TEST(CapsedPartsOfSentence) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Производство по административным ДЕЛАМ О ПРИОСТАНОВЛЕНИИ деятельности ИЛИ ЛИКВИДАЦИИ политической партии. Ее регионального ОТДЕЛЕНИЯ ИЛИ иного СТРУКТУРНОГО ПОДРАЗДЕЛЕНИЯ, ДРУГОГО ОБЩЕСТВЕННОГО объединения, религиозной И ИНОЙ НЕКОММЕРЧЕСКОЙ ОРГАНИЗАЦИ.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Производство по административным делам о приостановлении деятельности или ликвидации политической партии. Ее регионального отделения или иного структурного подразделения, другого общественного объединения, религиозной и иной некоммерческой организаци.");
    }

    Y_UNIT_TEST(SingleCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Приклеить её обратно клеем ПВА.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Приклеить её обратно клеем ПВА.");
    }

    Y_UNIT_TEST(CapsSeparatedByPunctuation) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Несколько иначе обстоит вопрос с индустриальными смазками ВМГЗ (ИАТБ), И3ОА, ИФЕА ~ ИПОА.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Несколько иначе обстоит вопрос с индустриальными смазками ВМГЗ (ИАТБ), И3ОА, ИФЕА ~ ИПОА.");
    }

    Y_UNIT_TEST(AsciiCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "AMD NVIDIA.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "AMD NVIDIA.");
    }

    Y_UNIT_TEST(AsciiInCapsedSentence) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ЭТО ПЕРЕБОЛЕВШИЕ COVID-19 В ТЕЧЕНИЕ ПОСЛЕДНЕГО ГОДА.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Это переболевшие COVID-19 в течение последнего года.");
    }

    Y_UNIT_TEST(SingleCapsWithNumber) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Он позволяет установить предварительный диагноз «COVID-19».";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Он позволяет установить предварительный диагноз «COVID-19».");
    }

    Y_UNIT_TEST(CapsSentences) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "РФ", "TSV"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "КРУГОМ ВРАГИ. AMD NVIDIA. КРУГОМ-ВРАГИ.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Кругом враги. AMD NVIDIA. КРУГОМ-ВРАГИ.");
    }

    Y_UNIT_TEST(AcronymCaps) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "ИП"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ОоО Вектор. Ип Пупкин!";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "ООО Вектор. ИП Пупкин!");
    }

    Y_UNIT_TEST(AcronymInCaspedSentence) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"США"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "В ГОРОДЕ НА СЕВЕРЕ США.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "В городе на севере США.");
    }

    Y_UNIT_TEST(LongPrettyWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Банк России", "Российской Федерации"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Банк россии (Центральный банк российской федерации) — особый публично-правовой институт.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Банк России (Центральный банк Российской Федерации) — особый публично-правовой институт.");
    }

    Y_UNIT_TEST(NotMatchedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Times New Roman"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Используйте шрифт times new andrew.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Используйте шрифт times new andrew.");
    }

    Y_UNIT_TEST(LongAndShortPrettyWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Российской Федерации", "Российской"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Банк России (Центральный банк российской федерации) находится на улице российской.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Банк России (Центральный банк Российской Федерации) находится на улице Российской.");
    }

    Y_UNIT_TEST(NestedPrettyWords) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"делами Президента Республики Беларусь", "Республики Беларусь", "Беларусь", "Центр Развития", "Центр Развития и Поддержки Предпринимателей Санкт-Петербурга", "Санкт-Петербурга"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Управление делами президента республики беларусь. Открыт центр развития и поддержки предпринимателей санкт-петербурга.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Управление делами Президента Республики Беларусь. Открыт Центр Развития и Поддержки Предпринимателей Санкт-Петербурга.");
    }

    Y_UNIT_TEST(NamingCapsAndPrettify) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Костромской области"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ЗАКРЫТОЕ АКЦИОНЕРНОЕ ОБЩЕСТВО \"БЕЗОПАСНЫЕ ДОРОГИ КОСТРОМСКОЙ ОБЛАСТИ\".";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Закрытое акционерное общество \"Безопасные дороги Костромской области\".");
    }

    Y_UNIT_TEST(DecapsePhrase) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "РФ", "TSV"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"Общество"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Производитель - ОБЩЕСТВО \"Вектор\"!";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Производитель - Общество \"Вектор\"!");
    }

    Y_UNIT_TEST(DecapseFirstWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "РФ", "TSV"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"код"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "КОД +7 - это российский номер.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Код +7 - это российский номер.");
    }

    Y_UNIT_TEST(DecapseNPhrases) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ООО", "РФ", "TSV"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"нельзя", "код", "обязательно", "инструкция"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "НЕЛЬЗЯ писать свой КОД, ОБЯЗАТЕЛЬНО используйте готовый, вам поможет ИНСТРУКЦИЯ.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Нельзя писать свой код, обязательно используйте готовый, вам поможет инструкция.");
    }

    Y_UNIT_TEST(OhterSpaceCharacters) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "London  is  the\tcapital of\nGreat  Britain!!!1111";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "London is the capital of Great Britain!!!1111");
    }

    Y_UNIT_TEST(CutSpareSpaces) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "London    is  the    capital of     Great  Britain!!!1111";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "London is the capital of Great Britain!!!1111");
    }

    Y_UNIT_TEST(CutUnstrippedSpaces) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "london is the capital of great britain. ";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "London is the capital of great britain.");
    }

    Y_UNIT_TEST(FirstLetter) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ооо Вектор.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Ооо Вектор.");
    }

    Y_UNIT_TEST(CutEmptySentence) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = ". London is the capital of great britain.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "London is the capital of great britain.");
    }

    Y_UNIT_TEST(AllInOne) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"АО", "ИНН"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"вид"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = ". Полное наименование: Ао \"ЖУКОВСКИЙ ИНТЕРНЕШНЛ ЭЙРПОРТ КАРГО\". ИнН: 5040144174. ВИД деятельности (по ОКВЭД): 52.23.11 - ДЕЯТЕЛЬНОСТЬ АЭРОПОРТОВАЯ. Форма собственности: 34 - Совместная частная и иностранная собственность...";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Полное наименование: АО \"Жуковский интернешнл эйрпорт карго\". ИНН: 5040144174. Вид деятельности (по ОКВЭД): 52.23.11 - деятельность аэропортовая. Форма собственности: 34 - Совместная частная и иностранная собственность...");
    }

    Y_UNIT_TEST(AllInSomeSentences) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"АО", "ИНН"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"внимание"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = ". Полное наименование: Ао \"ЖУКОВСКИЙ ИНТЕРНЕШНЛ ЭЙРПОРТ КАРГО\". ИнН: 5040144174. Вид деятельности (по ОКВЭД): 52.23.11 - ДЕЯТЕЛЬНОСТЬ АЭРОПОРТОВАЯ. Форма собственности: 34 - Совместная частная и иностранная собственность... Ангустамин. ВНИМАНИЕ:НЕ ЯВЛЯЕТСЯ ЛЕКАРСТВЕННЫМ СРЕДСТВОМ. Рекомендуется:В качестве биологически активной добавки к пище - дополнительного источника йода.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Полное наименование: АО \"Жуковский интернешнл эйрпорт карго\". ИНН: 5040144174. Вид деятельности (по ОКВЭД): 52.23.11 - деятельность аэропортовая. Форма собственности: 34 - Совместная частная и иностранная собственность... Ангустамин. Внимание:не является лекарственным средством. Рекомендуется:В качестве биологически активной добавки к пище - дополнительного источника йода.");
    }

    Y_UNIT_TEST(RegularFact) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"ГосУслуг»"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Как посмотреть результаты теста на covid19 на ГосУслуги: 1) Открываем портал gosuslugi.ru и входим на сайт под своим паролем. 2) В верхнем меню нажимаем на раздел «УСЛУГИ». 3) Открывается страница «Каталог ГосУслуг», на ней ищем раздел «МОЕ ЗДОРОВЬЕ». 4) На открывшейся странице ищем «Сведения о результатах исследований и иммунизации COVID-19».";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Как посмотреть результаты теста на covid19 на ГосУслуги: 1) Открываем портал gosuslugi.ru и входим на сайт под своим паролем. 2) В верхнем меню нажимаем на раздел «Услуги». 3) Открывается страница «Каталог ГосУслуг», на ней ищем раздел «Мое здоровье». 4) На открывшейся странице ищем «Сведения о результатах исследований и иммунизации COVID-19».");
    }

    Y_UNIT_TEST(LongText) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Сибирь"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ИНФОРМАЦИОННАЯ БЕЗОПАСНОСТЬ - способность государства защитить все сферы общественной жизни,сознание и психику граждан от негативного информационного воздействия,обеспечить субъекты политического и военного руководства данными для успешной модернизации общества и армии,недопустить утечки закрытой,общественно ценной информации и сохранить постоянную готовность к информационному противоборству внутри страны и на мировой арене,способствовать достижению социальной стабильности и согласия в обществе. Основным видом деятельности ООО \"АЙКХОФФ СИБИРЬ\" является \"Ремонт машин и оборудования\". Организация также зарегистрирована в таких категориях ОКВЭД как \"Торговля оптовая эксплуатационными материалами и принадлежностями машин\", \"Образование дополнительное детей и взрослых прочее, не включенное в другие группировки\", \"Торговля оптовая прочими машинами, приборами, аппаратурой и оборудованием общепромышленного и специального назначения\", \"Торговля оптовая прочими машинами и оборудованием\".";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Информационная безопасность - способность государства защитить все сферы общественной жизни,сознание и психику граждан от негативного информационного воздействия,обеспечить субъекты политического и военного руководства данными для успешной модернизации общества и армии,недопустить утечки закрытой,общественно ценной информации и сохранить постоянную готовность к информационному противоборству внутри страны и на мировой арене,способствовать достижению социальной стабильности и согласия в обществе. Основным видом деятельности ООО \"Айкхофф Сибирь\" является \"Ремонт машин и оборудования\". Организация также зарегистрирована в таких категориях ОКВЭД как \"Торговля оптовая эксплуатационными материалами и принадлежностями машин\", \"Образование дополнительное детей и взрослых прочее, не включенное в другие группировки\", \"Торговля оптовая прочими машинами, приборами, аппаратурой и оборудованием общепромышленного и специального назначения\", \"Торговля оптовая прочими машинами и оборудованием\".");
    }

    Y_UNIT_TEST(OnlyHighlightingTokens) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[\x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[\x07]");
    }

    Y_UNIT_TEST(OneHighlightedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[Преступление\x07].";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Преступление\x07].");
    }

    Y_UNIT_TEST(OneHighlightedWordWithoutPunctuation) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[Жестокость\x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Жестокость\x07]");
    }

    Y_UNIT_TEST(OneHighlightedWordInPhrase) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Мосто-мозжечковый угол (ММУ) – \x07[область\x07], расположенная у основания черепа в месте смыкания варолиева моста, продолговатого мозга и мозжечка.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Мосто-мозжечковый угол (ММУ) – \x07[область\x07], расположенная у основания черепа в месте смыкания варолиева моста, продолговатого мозга и мозжечка.");
    }

    Y_UNIT_TEST(ManyHighlightedWordsInPhrase) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[Технологическая\x07] \x07[карта\x07] и \x07[презентация\x07] \x07[урока\x07] по...";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Технологическая\x07] \x07[карта\x07] и \x07[презентация\x07] \x07[урока\x07] по...");
    }

    Y_UNIT_TEST(FirstHighlightedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[Беродуал\x07] - инструкция по применению, дозы, побочные действия, противопоказания...";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Беродуал\x07] - инструкция по применению, дозы, побочные действия, противопоказания...");
    }

    Y_UNIT_TEST(LastHighlightedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Могут ли постельные клопы поселиться на \x07[голове\x07]?";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Могут ли постельные клопы поселиться на \x07[голове\x07]?");
    }

    Y_UNIT_TEST(OneHighlightedCapsedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"наказание"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[НАКАЗАНИЕ\x07].";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Наказание\x07].");
    }

    Y_UNIT_TEST(OneHighlightedCapsedWordWithoutPunctuation) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"благочестие"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[БЛАГОЧЕСТИЕ\x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Благочестие\x07]");
    }

    Y_UNIT_TEST(OneHighlightedCapsedWordInPhrase) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Ивангороде"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Авито | Объявления в \x07[ИВАНГОРОДЕ\x07]: недвижимость, транспорт, работа, услуги, вещи...";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Авито | Объявления в \x07[Ивангороде\x07]: недвижимость, транспорт, работа, услуги, вещи...");
    }

    Y_UNIT_TEST(FirstHighlightedCapsedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"взаимодействие"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[ВЗАИМОДЕЙСТВИЕ\x07] аллельных генов.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Взаимодействие\x07] аллельных генов.");
    }

    Y_UNIT_TEST(LastHighlightedCapsedWord) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({"обучение"});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Налоговый вычет за \x07[ОБУЧЕНИЕ\x07].";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Налоговый вычет за \x07[обучение\x07].");
    }

    Y_UNIT_TEST(TwoHighlightedCapsedWords) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Вера", "Титова"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[ВЕРА\x07] \x07[ТИТОВА\x07]: биография, творчество, карьера, личная жизнь Кино";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Вера\x07] \x07[Титова\x07]: биография, творчество, карьера, личная жизнь Кино");
    }

    Y_UNIT_TEST(AllHighlightedCapsedWords) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"России"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[БИНАРНЫЕ\x07] \x07[ОПЦИОНЫ\x07] \x07[-\x07] \x07[ЛУЧШИЕ\x07] \x07[БРОКЕРЫ\x07] \x07[В\x07] \x07[РОССИИ\x07] \x07[2021\x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Бинарные\x07] \x07[опционы\x07] \x07[-\x07] \x07[лучшие\x07] \x07[брокеры\x07] \x07[в\x07] \x07[России\x07] \x07[2021\x07]");
    }

    Y_UNIT_TEST(CapsedHighlightedQuotedPhrase) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\"\x07[ТЕОРИЯ\x07] \x07[АВТОМАТИЧЕСКОГО\x07] УПРАВЛЕНИЯ\".";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\"\x07[Теория\x07] \x07[автоматического\x07] управления\".");
    }

    Y_UNIT_TEST(ManyHighlightedPhrases) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Великого", "Новгорода"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "НОВОСТИ \x07[ВЕЛИКОГО\x07] \x07[НОВГОРОДА\x07]. \x07[ГЛАВНЫЕ\x07] \x07[НОВОСТИ\x07] \x07[ВЕЛИКОГО\x07] \x07[НОВГОРОДА\x07] \x07[СЕГОДНЯ\x07]...";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Новости \x07[Великого\x07] \x07[Новгорода\x07]. \x07[Главные\x07] \x07[новости\x07] \x07[Великого\x07] \x07[Новгорода\x07] \x07[сегодня\x07]...");
    }

    Y_UNIT_TEST(HighlightedAndUnhighlightedCapsedWords) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Валькирию"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "ШИКАРНЕЙШИЙ ПЕРСОНАЖ!! ПОЛНЫЙ \x07[ОТЗЫВ\x07] НА \x07[ВАЛЬКИРИЮ\x07]... - YouTube.";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Шикарнейший персонаж!! Полный \x07[отзыв\x07] на \x07[Валькирию\x07]... - YouTube.");
    }

    Y_UNIT_TEST(TokenizationProblem) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"AnTuTu", "Samsung", "Galaxy", " ", "S7", "S"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[antutu\x07] samsung Galaxy \x07[Tab\x07] \x07[s\x07]\x07[7\x07]+ LTE результаты теста";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[AnTuTu\x07] Samsung Galaxy \x07[Tab\x07] \x07[S\x07]\x07[7\x07]+ LTE результаты теста");
    }

    Y_UNIT_TEST(ExactlyLastWordHiglighted) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"Москве"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "\x07[Принтеры\x07], копиры, МФУ \x07[Epson\x07]. Купить в \x07[МОСКВЕ\x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "\x07[Принтеры\x07], копиры, МФУ \x07[Epson\x07]. Купить в \x07[Москве\x07]");
    }

    Y_UNIT_TEST(SpaceBeforeHighlightingTokenInTheSentenceEnd) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"будильников"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "И мертвого поднимем — 20 самых креативных \x07[БУДИЛЬНИКОВ \x07]";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "И мертвого поднимем — 20 самых креативных \x07[будильников\x07]");
    }

    Y_UNIT_TEST(SpaceAfterHighlightingTokenInTheSentenceEnd) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"судимости"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Как ускорить получение \x07[справки\x07] об \x07[отсутствии\x07] \x07[ СУДИМОСТИ\x07] ";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Как ускорить получение \x07[справки\x07] об \x07[отсутствии\x07] \x07[судимости\x07]");
    }

    Y_UNIT_TEST(SpacesBeforeAndAfterHighlightingTokensInTheSentenceEnd) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"полиса"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Калькулятор \x07[ОСАГО\x07] Альфа \x07[страхование\x07] - расчет стоимости \x07[ ПОЛИСА \x07] ";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Калькулятор \x07[ОСАГО\x07] Альфа \x07[страхование\x07] - расчет стоимости \x07[полиса\x07]");
    }

    Y_UNIT_TEST(SpacesBetweenHighlightingTokensBrokenHighlighting) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({"оклады"});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Военнослужащим и силовикам РФ повысят \x07 [ОКЛАДЫ \x07 ] на 3%";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Военнослужащим и силовикам РФ повысят \x07[оклады \x07]на 3%");
    }

    Y_UNIT_TEST(SpacesAfterBetweenBeforeHighlightingTokensBrokenHighlighting) {
        TCompactTrie<char, TString> prettyTrie = MakePretty({});
        TCompactTrie<char, TString> decapsableTrie = MakeDecapsable({});
        NFacts::TCaseCorrector caseCorrector(decapsableTrie, prettyTrie);

        TString text = "Игра 2022 года. Превью \x07 [ Elden \x07 ] \x07 [ Ring \x07 ] ";
        caseCorrector.Process(text);
        UNIT_ASSERT_EQUAL(text, "Игра 2022 года. Превью \x07[Elden \x07]\x07[Ring\x07]");
    }
}
