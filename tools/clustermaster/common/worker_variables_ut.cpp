#include "worker_variables.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(WorkerVariablesTest) {
    Y_UNIT_TEST_DECLARE(Conditions);
}

class TTestWorkerVariables: public TWorkerVariables {
public:
    using TWorkerVariables::Map;
    Y_UNIT_TEST_FRIEND(WorkerVariablesTest, Conditions);
};

Y_UNIT_TEST_SUITE_IMPLEMENTATION(WorkerVariablesTest) {
    Y_UNIT_TEST(Conditions) {
        TTestWorkerVariables vars;

        vars.Set(TTestWorkerVariables::TMapType::value_type("ONE", "", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("TWO", "two", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("THREE", "one two three", 0));

        // Exists/not exists
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("ONE")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("TWO")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("THREE")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("FOUR")));

        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!ONE")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!TWO")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!THREE")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!FOUR")));

        // Has value
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!FOUR~foo")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("FOUR~foo")));

        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!ONE~foo")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("ONE~foo")));

        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!TWO~foo")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("TWO~two")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("TWO~foo")));

        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("THREE~one")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("THREE~two")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("THREE~three")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~four")));

        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!THREE~one")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!THREE~two")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!THREE~three")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~four")));

        // Check that it doesn't match parts of words
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~on")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~ne")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~t")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~o")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~thr")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!THREE~ree")));

        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~on")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~ne")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~t")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~o")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~thr")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~ree")));

        // Equals value
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("TWO=two")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!TWO=two")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("TWO=foo")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!TWO=foo")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("FOUR=foo")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!FOUR=foo")));

        // Value from variable
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("TWO=$TWO")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!TWO=$TWO")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("THREE~$TWO")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("!THREE~$TWO")));

        // Disjunction
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!ONE|!TWO|!FOUR|!THREE")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("TWO|!FOUR")));
        UNIT_ASSERT(vars.CheckCondition(TCondition::Parse("!TWO=$TWO|THREE~$ONE")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("TWO~one|!TWO=two")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE~four|THREE=|THREE~five")));
        UNIT_ASSERT(!vars.CheckCondition(TCondition::Parse("THREE=$FOUR|THREE=$TWO|THREE=$ONE")));
    }

    Y_UNIT_TEST(Substitution) {
        TTestWorkerVariables vars;

        vars.Set(TTestWorkerVariables::TMapType::value_type("oN_e", "", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("oN_e_", "-", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("1Tw0", "two", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("__three_", "$oN_e", 0));
        vars.Set(TTestWorkerVariables::TMapType::value_type("-", "LOL", 0));

        TString s;

        #define MY_UNIT_ASSERT(A) do { UNIT_ASSERT_NO_EXCEPTION(A); UNIT_ASSERT(A); } while (false)

        s = "$__three_$oN_e_$1Tw0$$oN_e";
        vars.Substitute(s);
        MY_UNIT_ASSERT(s == "$oN_e-two$");

        s = "$oN_e$1Tw0$$__three_$.";
        vars.Substitute(s);
        MY_UNIT_ASSERT(s == "two$$oN_e$.");

        s = "oN_e 1Tw0$$$/__three_";
        vars.Substitute(s);
        MY_UNIT_ASSERT(s == "oN_e 1Tw0$$$/__three_");

        s = "$1Tw0$on_e|three";
        UNIT_ASSERT_EXCEPTION(vars.Substitute(s), TTestWorkerVariables::TNoVariable);

        s = "$s-";
        UNIT_ASSERT_EXCEPTION(vars.Substitute(s), TTestWorkerVariables::TNoVariable);

        s = "asd$$oN_e$$__$";
        UNIT_ASSERT_EXCEPTION(vars.Substitute(s), TTestWorkerVariables::TNoVariable);

        #undef MY_UNIT_ASSERT
    }
}
