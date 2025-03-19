#include <kernel/feature_pool/feature_filter/feature_filter.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

#include <google/protobuf/text_format.h>

using namespace NMLPool;

static TPoolInfo GetPoolInfo() {
    static const char* text =
        "FeatureInfo{\n"
        "    Name: \"FooX\"\n"
        "    Tags: [\"TG_A\", \"TG_B\"]\n"
        "}\n"
        "FeatureInfo{\n"
        "    Name: \"FooY\"\n"
        "    Tags: [\"TG_B\", \"TG_C\"]\n"
        "}\n"
        "FeatureInfo{\n"
        "    Name: \"BarX\"\n"
        "}\n"
        "FeatureInfo{\n"
        "    Name: \"BarZ\"\n"
        "    Tags: [\"TG_A\", \"TG_B\", \"TG_C\"]\n"
        "}\n";
    TPoolInfo info;
    Y_VERIFY(::google::protobuf::TextFormat::ParseFromString(TString(text), &info));
    return info;
}

TVector<size_t> GetFactors(const TPoolInfo& info, const TFeatureFilter& pred) {
    TVector<size_t> res;
    size_t index = 0;
    for (const auto& feature : info.GetFeatureInfo()) {
        if (pred(feature)) {
            res.push_back(index);
        }
        index += 1;
    }
    return res;
}

template <typename T>
static IOutputStream& operator<<(IOutputStream& out, const TVector<T>& values) {
    for (size_t i : xrange(values.size())) {
        if (i > 0) {
            out << " ";
        }
        out << values[i];
    }
    return out;
}

Y_UNIT_TEST_SUITE(FeatureFilterTest) {
    void CheckFeatures(const TString& nameRegExp,
        const TString& tagExp,
        const TVector<size_t> expected)
    {
        TFeatureFilter filter;
        filter.SetNameExpr(nameRegExp);
        filter.SetTagExpr(tagExp);
        Cdbg << "EXPECTED(" << expected << ") == GOT("
            << GetFactors(GetPoolInfo(), filter) << ")" << Endl;
        UNIT_ASSERT_EQUAL(GetFactors(GetPoolInfo(), filter), expected);
    }

    Y_UNIT_TEST(TestEmpty) {
        CheckFeatures("", "", {0, 1, 2, 3});
    }
    Y_UNIT_TEST(TestRegEx) {
        CheckFeatures("^Foo", "", {0, 1});
        CheckFeatures("X", "", {0, 2});
        CheckFeatures("(Foo|Bar)(X|Y)", "", {0, 1, 2});
    }
    Y_UNIT_TEST(TestTagExp) {
        CheckFeatures("", "TG_A", {0, 3});
        CheckFeatures("", "TG_A && !TG_C", {0});
        CheckFeatures("", "!TG_B && (TG_A || TG_C)", {});
        CheckFeatures("", "!TG_FOO", {0, 1, 2, 3});
        CheckFeatures("", "TG_BAR", {});
    }
    Y_UNIT_TEST(TestIndexExp) {
        {
            TIndexFilter filter("5,2-7,3-4,15,20-25");
            TSet<size_t> expected{2,3,4,5,6,7,15,20,21,22,23,24,25};
            for (size_t i : xrange(26)) {
                UNIT_ASSERT_EQUAL(filter(i), expected.contains(i));
            }
        }
        {
            TIndexFilter filter("");
            for (size_t i : xrange(26)) {
                UNIT_ASSERT_EQUAL(filter(i), false);
            }
        }
        {
            UNIT_ASSERT_EXCEPTION(TIndexFilter("wtf"), yexception);
        }
    }
}
