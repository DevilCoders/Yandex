#include <library/cpp/testing/gtest/gtest.h>
#include <grp.h>

#include <group.h>

static const auto& testGroup = getgrgid(getgid());

TEST(Group, TestOperatorBool) {
    const auto& group = NTokenAgent::TGroup::FromId(testGroup->gr_gid);

    EXPECT_TRUE(group);
}

TEST(Group, TestNotExistentGroupByName) {
    const auto& nonExistentGroup = NTokenAgent::TGroup::FromName("does not exist");

    EXPECT_FALSE(nonExistentGroup);
}

TEST(Group, TestNotExistentGroupById) {
    const auto& nonExistentGroup = NTokenAgent::TGroup::FromId((ui32)-2);

    EXPECT_FALSE(nonExistentGroup);
}

TEST(Group, CreateByName) {
    const auto& group = NTokenAgent::TGroup::FromName(testGroup->gr_name);

    EXPECT_EQ(group.GetName(), testGroup->gr_name);
    EXPECT_EQ(group.GetId(), testGroup->gr_gid);
}

TEST(Group, CreateById) {
    const auto& group = NTokenAgent::TGroup::FromId(testGroup->gr_gid);

    EXPECT_EQ(group.GetName(), testGroup->gr_name);
    EXPECT_EQ(group.GetId(), testGroup->gr_gid);
}
