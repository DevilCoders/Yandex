#include "sax.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TSaxTest) {
    Y_UNIT_TEST(Container) {
        TString config =
            "instance = {"
            "  container1 = {"
            "    opt1 = 1;"
            "    opt2 = 2;"
            "  };"
            "  container2 = {"
            "    { opt1 = 1; };"
            "    { opt2 = 2; };"
            "  };"
            "  container3 = {};"
            "  opt3 = 3;"
            "};";

        TStringInput si(config);
        THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(si);

        size_t counter = 0;

        class TFunc: public NConfig::IConfig::IFunc {
        public:
            explicit TFunc(size_t* counter)
                : Counter_(counter)
            {
            }

        private:
            void DoConsume(const TString& key, NConfig::IConfig::IValue* value) override {
                if (key.StartsWith("container")) {
                    ++*Counter_;
                    UNIT_ASSERT_C(value->IsContainer(), key);
                } else if (key.StartsWith("opt")) {
                    ++*Counter_;
                    UNIT_ASSERT_C(!value->IsContainer(), key);
                }

                value->AsSubConfig()->ForEach(this);
            }

            size_t* const Counter_;
        } f(&counter);

        parsed->ForEach(&f);
        UNIT_ASSERT_EQUAL(counter, 8);
    }

    Y_UNIT_TEST(MaybeAndHolder) {
        TString config =
            "instance = {"
            "  opt1 = 1;"
            "  opt2 = 2;"
            "};";

        TStringInput si(config);
        THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(si);

        class TFunc: public NConfig::IConfig::IFunc {
        public:
            TMaybe<int> opt1;
            THolder<int> opt2;

        private:
            START_PARSE {
                ON_KEY("opt1", opt1) {
                    return;
                }

                ON_KEY("opt2", opt2) {
                    return;
                }
            } END_PARSE;
        } f;

        parsed->ForEach(&f);

        UNIT_ASSERT(f.opt1);
        UNIT_ASSERT_EQUAL(*f.opt1, 1);
        UNIT_ASSERT(f.opt2);
        UNIT_ASSERT_EQUAL(*f.opt2, 2);
    }
}
