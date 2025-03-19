#include <library/cpp/testing/gtest/gtest.h>
#include <user.h>
#include <pwd.h>
#include <sys/types.h>

const auto current_user_id = getuid();

TEST(UserTest, GetTest) {
    auto current_user = getpwuid(current_user_id);
    auto user = NTokenAgent::TUser(current_user->pw_name, current_user->pw_uid, current_user->pw_gid);
    EXPECT_EQ(user.GetName(), current_user->pw_name);
    EXPECT_EQ(user.GetId(), current_user->pw_uid);
    EXPECT_EQ(user.GetGroupId(), current_user->pw_gid);
}

TEST(UserTest, FromNameTest) {
    auto current_user = getpwuid(current_user_id);
    auto user = NTokenAgent::TUser::FromName(current_user->pw_name);
    EXPECT_TRUE(user);

    auto nonExistentUser = NTokenAgent::TUser::FromName("");
    EXPECT_FALSE(nonExistentUser);
}

TEST(UserTest, FromIdTest) {
    auto user = NTokenAgent::TUser::FromId(current_user_id);
    EXPECT_TRUE(user);

    auto nonExistentUser = NTokenAgent::TUser::FromId(-1);
    EXPECT_FALSE(nonExistentUser);
}
