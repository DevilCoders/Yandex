#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <library/cpp/digest/old_crc/crc.h>

#include "sax.h"

Y_UNIT_TEST_SUITE(TXmlSaxTest) {
    class TCb: public NXml::ISaxHandler {
    public:
        inline TCb(IOutputStream* out)
            : Out_(*out)
        {
        }

        void OnStartDocument() override {
            Out_ << "start\n";
        }

        void OnEndDocument() override {
            Out_ << "end\n";
        }

        bool GetEntity(const TStringBuf& entity, TString& expansion) override {
            if (entity == "bad") {
                return false;
            }

            expansion = TString("entity[") + entity + "]";
            return true;
        }

        void OnStartElement(const char* name, const char** attrs) override {
            Out_ << "se " << name << "\n";

            while (attrs && *attrs) {
                Out_ << "attr " << *attrs << "\n";
                ++attrs;
            }
        }

        void OnEndElement(const char* name) override {
            Out_ << "ee " << name << "\n";
        }

        void OnText(const char* text, size_t len) override {
            Out_ << "text " << TString(text, len) << "\n";
        }

    private:
        IOutputStream& Out_;
    };

    Y_UNIT_TEST(TestParse) {
        TStringStream ss;
        TCb cb(&ss);
        TString xml = "<a><el1></el1><el2>text1</el2><el3><el1></el1>text3</el3><el4 qw=\"qwqw\">text5</el4></a>";
        TStringInput si(xml);

        NXml::ParseXml(&si, &cb);

        UNIT_ASSERT_EQUAL(Crc<ui64>(ss.Str().data(), ss.Str().size()), ULL(6968380102647173389));
    }

    Y_UNIT_TEST(TestEntities) {
        TStringStream ss;
        TCb cb(&ss);
        TString xml = "<test attr=\"&hellip;\">&magic;</test>";
        TStringInput si(xml);

        NXml::ParseXml(&si, &cb);
        UNIT_ASSERT_EQUAL(Crc<ui64>(ss.Str().data(), ss.Str().size()), ULL(9779707346090159793));
    }

    Y_UNIT_TEST(TestEntitiesError) {
        TStringStream ss;
        TCb cb(&ss);
        TString xml = "<test>&bad;</test>";
        TStringInput si(xml);
        bool wasError = false;

        try {
            NXml::ParseXml(&si, &cb);
        } catch (...) {
            wasError = true;
        }
        UNIT_ASSERT(wasError);
    }

    Y_UNIT_TEST(TestError) {
        TStringStream ss;
        TCb cb(&ss);
        TString xml = "<a><el1></el1><el2>text1</el2><el3><el1></el1>text3</el3><el4 qw=\"qwqw\">text5</el4></a>extra";
        TStringInput si(xml);
        bool wasError = false;

        try {
            NXml::ParseXml(&si, &cb);
        } catch (...) {
            wasError = true;
        }

        UNIT_ASSERT(wasError);
        UNIT_ASSERT_EQUAL(Crc<ui64>(ss.Str().data(), ss.Str().size()), ULL(4160225226096208826));
    }
}
