#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/is_definition.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/qtree/query.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/charset/wide.h>

namespace NSnippets {

static const TUtf16String FIRST_SENT = u"Гистере́зис (греч. ὑστέρησις — отставание,запаздывание) — свойство систем...";
static const TUtf16String SECOND_SENT = u"Для гистерезиса характерно явление «насыщения», а также неодинаковость траекторий между крайними...";

static const TUtf16String SIMPLE_DEFINITION = UTF8ToWide("Гистерезис - свойство систем (физических, биологических и т. д.), "
    "мгновенный отклик которых на приложенные к ним воздействия зависит в том числе и от их текущего состо...");
static const TUtf16String DEFINITION_WITH_BRACKETS = UTF8ToWide("Гистерезис (греч. ὑστέρησις — отставание,запаздывание) — свойство "
    "(физических, биологических и т. д.), мгновенный отклик которых на приложенные к ним воздействия зависит "
    "в том числе и от их текущего состояния...");
static const TUtf16String LONG_DEFINITION = UTF8ToWide("Гистерезис гистерезис - это свойство систем (физических, биологических и т. д.),"
    " мгновенный отклик которых на приложенные к ним воздействия зависит в том числе и от их текущего состо...");
static const TUtf16String VERY_LONG_DEFINITION_WITH_BRACKETS = UTF8ToWide("Гистерезис Гистерезис (греч. ὑστέρησις — отставание,запаздывание) "
    " гистерезис гистерезис гистерезис - это свойство систем (физических, биологических и т. д.), "
    "мгновенный отклик которых на приложенные к ним воздействия зависит в том числе и от их текущего состо...");
static const TUtf16String NOT_DEFINITION = UTF8ToWide("Гистерезис. Свойство систем (физических, биологических и т. д.), "
    "мгновенный отклик которых на приложенные к ним воздействия зависит в том числе и от их текущего состо...");
static const TUtf16String WRONG_DEFINITION = UTF8ToWide("Анизотропность - это свойство систем (физических, биологических и т. д.), "
    "мгновенный отклик которых на приложенные к ним воздействия зависит в том числе и от их текущего состо...");


Y_UNIT_TEST_SUITE(TIsDefinitionTests) {
    Y_UNIT_TEST(TestTextIsDefinition) {
        const TConfig cfg;
        const TRichTreePtr qtree = CreateRichTree(u"что такое гистерезис", TCreateTreeOptions());
        const TQueryy query(qtree->Root.Get(), cfg);
        TRetainedSentsMatchInfo rsmi;
        rsmi.SetView({FIRST_SENT, SECOND_SENT}, TRetainedSentsMatchInfo::TParams(cfg, query));
        const TSentsMatchInfo& smi = *rsmi.GetSentsMatchInfo();

        UNIT_ASSERT(LooksLikeDefinition(smi, 0));
        UNIT_ASSERT(!LooksLikeDefinition(smi, 1));
    }

    Y_UNIT_TEST(TestIsDefinition) {
        const TConfig cfg;
        const TRichTreePtr qtree = CreateRichTree(u"что такое гистерезис", TCreateTreeOptions());
        const TQueryy query(qtree->Root.Get(), cfg);

        UNIT_ASSERT(LooksLikeDefinition(query, SIMPLE_DEFINITION, cfg));
        UNIT_ASSERT(LooksLikeDefinition(query, LONG_DEFINITION, cfg));

        UNIT_ASSERT(LooksLikeDefinition(query, DEFINITION_WITH_BRACKETS, cfg));
        UNIT_ASSERT(LooksLikeDefinition(query, VERY_LONG_DEFINITION_WITH_BRACKETS, cfg));

        UNIT_ASSERT(!LooksLikeDefinition(query, NOT_DEFINITION, cfg));
        UNIT_ASSERT(!LooksLikeDefinition(query, WRONG_DEFINITION, cfg));
    }
}
} // namespace NSnippets
