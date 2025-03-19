#include <kernel/mango/common/portion_tags.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/dirut.h>
#include <util/system/env.h>
#include <util/system/fs.h>

using namespace NMango;

const TString TAG_DIR = "tags";

class TTagsEnvironment {
public:
    TTagsEnvironment()
    {
        NFs::Remove(TAG_DIR);
        MakeDirIfNotExist(TAG_DIR.data());
        SetEnv("PORTION_TAG_DIR", TAG_DIR);
    }

    ~TTagsEnvironment() {
        RemoveDirWithContents(TAG_DIR);
    }
};

Y_UNIT_TEST_SUITE(TPortionTagTest) {
    Y_UNIT_TEST(TestInvalidEnvironment) {
        UNIT_ASSERT_EXCEPTION(TPortionTags("whatever"), yexception);
    }

    Y_UNIT_TEST(TestTags) {
        TTagsEnvironment env;
        TPortionTags tags("tag_type");
        UNIT_ASSERT_VALUES_EQUAL(0U, tags.GetTags().size());
        UNIT_ASSERT_VALUES_EQUAL(false, tags.HasTag("a"));
        tags.SetTag("a");
        UNIT_ASSERT_VALUES_EQUAL(true, tags.HasTag("a"));
        THashSet<TString> tagList = tags.GetTags();
        UNIT_ASSERT_VALUES_EQUAL(true, tagList.find("a") != tagList.end());

        tags.SetTag("b");
        tags.SetTag("c");
        tagList = tags.GetTags();
        UNIT_ASSERT_VALUES_EQUAL(true, tagList.find("a") != tagList.end());
        UNIT_ASSERT_VALUES_EQUAL(true, tagList.find("b") != tagList.end());
        UNIT_ASSERT_VALUES_EQUAL(true, tagList.find("c") != tagList.end());
        UNIT_ASSERT_VALUES_EQUAL(false, tagList.find("d") != tagList.end());
    }

    Y_UNIT_TEST(TestDelayedTags) {
        TTagsEnvironment env;
        TPortionTags tags("tag_type");

        tags.SetTag("a", true);
        tags.SetTag("b", true);
        tags.SetTag("c", false);

        UNIT_ASSERT_VALUES_EQUAL(false, tags.HasTag("a"));
        UNIT_ASSERT_VALUES_EQUAL(false, tags.HasTag("b"));
        UNIT_ASSERT_VALUES_EQUAL(true, tags.HasTag("c"));

        tags.SetDelayedTags();

        UNIT_ASSERT_VALUES_EQUAL(true, tags.HasTag("a"));
        UNIT_ASSERT_VALUES_EQUAL(true, tags.HasTag("b"));
        UNIT_ASSERT_VALUES_EQUAL(true, tags.HasTag("c"));
    }
}
