#include "snippet_classifier.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/ptr.h>

Y_UNIT_TEST_SUITE(SnippetClassifierTest) {
    Y_UNIT_TEST(CommercialTest) {
        THolder<ISnippetClassifier> classifier;
        classifier.Reset(CreateSnipClassifier());
        const TStringBuf good_title = "\"Парк культуры\" открывают - Общество - Интерфакс";
        const TStringBuf good_title2 = "...мэр ознакомился с результатами реставрации станции \"Парк культуры\"...";
        const TStringBuf good_snippet =
                "Станция \"Парк культуры\" Кольцевой линии "\
                "столичного метрополитена, закрытая для пассажиров "\
                "с 5 февраля прошлого года, вновь начнет принимать пассажиров "\
                "в ближайшую субботу. Планировалось, что ее откроют в конце "\
                "прошлого года, однако из-за срыва";
        UNIT_ASSERT(!(classifier.Get()->GetTags(good_title) & RT_COMMERCIAL));
        UNIT_ASSERT(!(classifier.Get()->GetTags(good_title2) & RT_COMMERCIAL));
        UNIT_ASSERT(!(classifier.Get()->GetTags(good_snippet) & RT_COMMERCIAL));
        UNIT_ASSERT(classifier.Get()->GetTags("market") & RT_COMMERCIAL);
        UNIT_ASSERT(classifier.Get()->GetTags("купить мопед") & RT_COMMERCIAL);
    }
}
