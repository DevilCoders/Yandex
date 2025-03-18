#include <tools/domschemec/ut/test.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/yexception.h>
#include <util/generic/maybe.h>


#define LOG_ERROR_MESSAGE(code, type) \
    try { \
        code; \
    } catch (type&) { \
        Cdbg << "MESSAGE[" << __LOCATION__ << "]: " << CurrentExceptionMessage() << Endl; \
    }

#define UNIT_ASSERT_EXCEPTION_AND_LOG(code, type) \
    UNIT_ASSERT_EXCEPTION(code, type); \
    LOG_ERROR_MESSAGE(code, type);

enum class EValidatePolicy {
    Strict,
    Relaxed
};

template <NDomSchemeRuntime::TValidateInfo::ESeverity Severity>
class TValidateException : public yexception {};

using TValidateError = TValidateException<NDomSchemeRuntime::TValidateInfo::ESeverity::Error>;
using TValidateWarning = TValidateException<NDomSchemeRuntime::TValidateInfo::ESeverity::Warning>;

template <NDomSchemeRuntime::TValidateInfo::ESeverity Severity>
inline void ThrowBySeverity(const NSc::TValue& value, TStringBuf path, TStringBuf error) {
    ythrow TValidateException<Severity>{}
         << "failed to validate JSON state: '" << value << "'"
         << "\n" "<validation error> JSON[\"" << path << "\"] " << error;
}

template <template<typename> class SchemeType>
inline void RunValidate(
    TStringBuf text,
    EValidatePolicy policy,
    TMaybe<NDomSchemeRuntime::TValidateInfo::ESeverity> selectSeverity = {})
{
    NSc::TValue value = NSc::TValue::FromJson(text);
    SchemeType<TSchemeTraits> scheme{&value};

    TMaybe<NDomSchemeRuntime::TValidateInfo::ESeverity> errorSeverity;
    TString errorPath;
    TString errorText;

    scheme->Validate(
        "",
        EValidatePolicy::Strict == policy,
        [&scheme, &errorSeverity, &errorPath, &errorText, &selectSeverity](
            const TString& path,
            const TString& err,
            NDomSchemeRuntime::TValidateInfo info)
        {
            if (NDomSchemeRuntime::TValidateInfo::ESeverity::Error == info.Severity
                && (!selectSeverity.Defined() || selectSeverity.GetRef() == info.Severity))
            {
                ThrowBySeverity<NDomSchemeRuntime::TValidateInfo::ESeverity::Error>(
                    *scheme.GetRawValue(),
                    (path ? path.data() : "/"),
                    err);
            } else if (!errorSeverity.Defined()) {
                errorSeverity = info.Severity;
                errorPath = TString{path ? path.data() : "/"};
                errorText = err;
            }
        });

    if (errorSeverity.Defined()
        && (!selectSeverity.Defined() || selectSeverity == errorSeverity))
    {
        Y_VERIFY(errorSeverity.GetRef() == NDomSchemeRuntime::TValidateInfo::ESeverity::Warning);
        ThrowBySeverity<NDomSchemeRuntime::TValidateInfo::ESeverity::Warning>(
            *scheme.GetRawValue(),
            errorPath,
            errorText);
    }
}

Y_UNIT_TEST_SUITE(SchemeStructTest) {
    Y_UNIT_TEST(TestEmptyStruct) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TEmpty>("{}", EValidatePolicy::Strict));

        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TEmpty>("{}", EValidatePolicy::Relaxed));

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TEmpty>("{a:b}", EValidatePolicy::Strict),
            TValidateError);

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TEmpty>("{a:b}", EValidatePolicy::Relaxed));

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TEmptyStrict>("{a:b}", EValidatePolicy::Relaxed),
            TValidateError);

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TEmptyRelaxed>("{a:b}", EValidatePolicy::Strict));
    }

    Y_UNIT_TEST(TestOptionalRequired) {
         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TRequiredA>("{}", EValidatePolicy::Strict),
            TValidateError);

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TOptionalA>("{}", EValidatePolicy::Strict));

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TRequiredA>("{a:b}", EValidatePolicy::Strict));

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TOptionalA>("{a:b}", EValidatePolicy::Strict));

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TRequiredA>("{b:c}", EValidatePolicy::Relaxed),
            TValidateError);

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TOptionalA>("{b:c}", EValidatePolicy::Relaxed));
    }

    Y_UNIT_TEST(TestFuzzyFieldMatch) {
         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TAbc>("{ab:d}", EValidatePolicy::Relaxed));

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TRequiredA>(
                "{ab:d}",
                EValidatePolicy::Strict),
            TValidateError);

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TRequiredA>(
                "{ab:d}",
                EValidatePolicy::Strict,
                NDomSchemeRuntime::TValidateInfo::ESeverity::Warning),
            TValidateWarning);
    }

    Y_UNIT_TEST(TestSaveDefault) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TDefaultABCD>(
                "{}",
                EValidatePolicy::Strict
            )
        );

        NSc::TValue value = NSc::TValue::FromJson("{}");
        NTest::TDefaultABCD<TSchemeTraits> scheme{&value};

        UNIT_ASSERT_EQUAL(scheme.A(), 42);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, scheme.B(), 0.42);
        UNIT_ASSERT_STRINGS_EQUAL(scheme.C(), "42");
        UNIT_ASSERT_EQUAL(scheme.D(), TDuration::Parse("42ms"));

        scheme.SetDefault();

        UNIT_ASSERT_STRINGS_EQUAL(scheme.ToJson(), R"_({"a":42,"b":0.42,"c":"42","d":"0.042000s"})_");
    }

    Y_UNIT_TEST(TestAny) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TAnyA>(
                "{a:42}",
                EValidatePolicy::Strict
            )
        );

        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TAnyA>(
                "{a:0.42}",
                EValidatePolicy::Strict
            )
        );

        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TAnyA>(
                "{a:\"42\"}",
                EValidatePolicy::Strict
            )
        );
    }

    Y_UNIT_TEST(TestAnyDefault) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TDefaultAnyABC>(
                "{}",
                EValidatePolicy::Strict
            )
        );

        NSc::TValue defaults = NSc::TValue::FromJson("{}");
        NTest::TDefaultAnyABC<TSchemeTraits> defaultsScheme{&defaults};

        UNIT_ASSERT_EQUAL(defaultsScheme.A().AsPrimitive<int>(), 42);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, defaultsScheme.B().AsPrimitive<double>(), 0.42);
        UNIT_ASSERT_STRINGS_EQUAL(defaultsScheme.C().AsString(), "42");

        defaultsScheme.SetDefault();

        UNIT_ASSERT_EQUAL(defaultsScheme.A().AsPrimitive<int>(), 42);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, defaultsScheme.B().AsPrimitive<double>(), 0.42);
        UNIT_ASSERT_STRINGS_EQUAL(defaultsScheme.C().AsString(), "42");

        const TStringBuf str = TStringBuf("{a:1,b:0.2,c:\"3\"}");

        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TDefaultAnyABC>(
                str,
                EValidatePolicy::Strict
            )
        );

        NSc::TValue overriden = NSc::TValue::FromJson(str);
        NTest::TDefaultAnyABC<TSchemeTraits> overridenScheme{&overriden};

        UNIT_ASSERT_EQUAL(overridenScheme.A().AsPrimitive<int>(), 1);
        UNIT_ASSERT_DOUBLES_EQUAL(0.01, overridenScheme.B().AsPrimitive<double>(), 0.2);
        UNIT_ASSERT_STRINGS_EQUAL(overridenScheme.C().AsString(), "3");
    }

    Y_UNIT_TEST(TestArray) {
         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TArrayA>(
                "{a:[4,8,15,16,23,42]}",
                EValidatePolicy::Strict));

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TArrayA>(
                "{a:[4,8,15,16,23,42,]}",
                EValidatePolicy::Strict));

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TArrayA>(
                "{a:[42]}",
                EValidatePolicy::Strict));

         UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TArrayA>(
                "{a:[]}",
                EValidatePolicy::Strict));

         UNIT_ASSERT_EXCEPTION_AND_LOG(
            RunValidate<NTest::TArrayA>(
                "{a:42}",
                EValidatePolicy::Strict),
            TValidateError);
    }

    Y_UNIT_TEST(TestArrayDefault) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TDefaultArrayABC>("{}", EValidatePolicy::Strict));

        NSc::TValue defaults = NSc::TValue::FromJson("{}");
        NTest::TDefaultArrayABC<TSchemeTraits> defaultScheme{&defaults};

        const auto& a = defaultScheme.A();
        const auto& b = defaultScheme.B();
        const auto& c = defaultScheme.C();
        const auto& e = defaultScheme.E();

        auto defA = TVector<i32>{4, 8, 15, 16, 23, 42};
        auto defB = TVector<double>{0.4, 0.8, 0.15, 0.16, 0.23, 0.42};
        auto defC = TVector<TString>{"4", "8", "15", "16", "23", "42"};
        auto defE = TVector<bool>{};

        auto assertContainersEqual = [](const auto& res, const auto& ref) {
            auto it = res.begin();
            for (auto x : ref) {
                UNIT_ASSERT_UNEQUAL(it, res.end());
                if constexpr (std::is_same<std::remove_reference_t<decltype(x)>, TString>::value) {
                    UNIT_ASSERT_STRINGS_EQUAL(x, *it);
                } else {
                    UNIT_ASSERT_EQUAL(x, *it);
                }
                ++it;
            }
            UNIT_ASSERT_EQUAL(it, res.end());
        };

        assertContainersEqual(a, defA);
        assertContainersEqual(b, defB);
        assertContainersEqual(c, defC);
        assertContainersEqual(e, defE);

        defaultScheme.SetDefault();

        assertContainersEqual(a, defA);
        assertContainersEqual(b, defB);
        assertContainersEqual(c, defC);
        assertContainersEqual(e, defE);
    }

    Y_UNIT_TEST(TestArrayToVector) {
        NSc::TValue value = NSc::TValue::FromJson("{a:[4,8,15,16,23,42]}");
        NTest::TArrayA<TSchemeTraits> scheme{&value};

        const auto expected = TVector<i32>{4, 8, 15, 16, 23, 42};
        const auto found = TVector<i32>(scheme.A().begin(), scheme.A().end());
        UNIT_ASSERT_EQUAL(expected, found);
    }

    Y_UNIT_TEST(TestChildOverrideDefault) {
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TChildDefaultA>("{}", EValidatePolicy::Strict));

        {
            NSc::TValue defaultParent = NSc::TValue::FromJson("{}");
            NTest::TParentDefaultA<TSchemeTraits> defaultParentScheme{&defaultParent};
            UNIT_ASSERT_STRINGS_EQUAL(defaultParentScheme.A(), "parent");
            defaultParentScheme.SetDefault();
            UNIT_ASSERT_STRINGS_EQUAL(defaultParentScheme.A(), "parent");
        }

        {
            NSc::TValue defaultChild = NSc::TValue::FromJson("{}");
            NTest::TChildDefaultA<TSchemeTraits> defaultChildScheme{&defaultChild};
            UNIT_ASSERT_STRINGS_EQUAL(defaultChildScheme.A(), "child");
            defaultChildScheme.SetDefault();
            UNIT_ASSERT_STRINGS_EQUAL(defaultChildScheme.A(), "child");
        }
    }

    Y_UNIT_TEST(TestCopySame) {
        const TStringBuf jsonString = TStringBuf(R"_({"a":43,"b":0.43,"c":"43","d":"0.043000s"})_");
        UNIT_ASSERT_NO_EXCEPTION(
            RunValidate<NTest::TDefaultABCD>(
                jsonString,
                EValidatePolicy::Strict
            )
        );

        NSc::TValue value = NSc::TValue::FromJson(jsonString);
        NTest::TDefaultABCD<TSchemeTraits> scheme{&value};
        NTest::TDefaultABCD<TSchemeTraits> scheme2{&value};

        UNIT_ASSERT_EQUAL(scheme.A(), 43);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, scheme.B(), 0.43);
        UNIT_ASSERT_STRINGS_EQUAL(scheme.C(), "43");
        UNIT_ASSERT_EQUAL(scheme.D(), TDuration::Parse("43ms"));

        const NTest::TDefaultABCD<TSchemeTraits>* schemePtr = &scheme;
        if (scheme.A() != 43) {
            // Не даём оптимизатору выкинуть присваивание двумя строчками ниже как тривиальное присваивание в самого себя.
            schemePtr++;
        }
        scheme = *schemePtr;

        UNIT_ASSERT_EQUAL(scheme.A(), 43);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, scheme.B(), 0.43);
        UNIT_ASSERT_STRINGS_EQUAL(scheme.C(), "43");
        UNIT_ASSERT_EQUAL(scheme.D(), TDuration::Parse("43ms"));

        scheme = scheme2;

        UNIT_ASSERT_EQUAL(scheme.A(), 43);
        UNIT_ASSERT_DOUBLES_EQUAL(0.001, scheme.B(), 0.43);
        UNIT_ASSERT_STRINGS_EQUAL(scheme.C(), "43");
        UNIT_ASSERT_EQUAL(scheme.D(), TDuration::Parse("43ms"));
    }

    Y_UNIT_TEST(TestConstFields) {
        const TStringBuf jsonString = TStringBuf(R"_({"a" : 43})_");

        NSc::TValue value = NSc::TValue::FromJson(jsonString);
        NTest::TConstant<TSchemeTraits> scheme{&value};

        // const fields are set, regardless of their presence in value
        scheme.SetDefault();
        UNIT_ASSERT_EQUAL(scheme.A(), 123);

        NSc::TValue value1 = NSc::TValue::FromJson(jsonString);
        NTest::TConstant<TSchemeTraits> scheme1{&value1};

        volatile int a = scheme1.A();
        UNIT_ASSERT_EQUAL(a, 43);

        // const fields are not affected by Assign
        scheme = scheme1;
        a = scheme.A();
        UNIT_ASSERT_EQUAL(a, 123);

        // const fields restored to default values at Assign
        scheme1 = scheme;
        a = scheme1.A();
        UNIT_ASSERT_EQUAL(a, 123);
    }

    Y_UNIT_TEST(TestEmptyContainerAssignment) {
        {
            const TStringBuf jsonString = TStringBuf(R"_({"a":[]})_");
            NSc::TValue value = NSc::TValue::FromJson(jsonString);
            NSc::TValue assigneeValue;
            NTest::TArrayA<TSchemeTraits> emptyA{&value};
            NTest::TArrayA<TSchemeTraits> assignee{&assigneeValue};
            assignee = emptyA;
            UNIT_ASSERT_VALUES_EQUAL(assigneeValue.ToJson(), jsonString);
        }
        {
            const TStringBuf jsonString = TStringBuf(R"_({"a":{}})_");
            NSc::TValue value = NSc::TValue::FromJson(jsonString);
            NSc::TValue assigneeValue;
            NTest::TDictA<TSchemeTraits> emptyA{&value};
            NTest::TDictA<TSchemeTraits> assignee{&assigneeValue};
            assignee = emptyA;
            UNIT_ASSERT_VALUES_EQUAL(assigneeValue.ToJson(), jsonString);
        }
    }
};
