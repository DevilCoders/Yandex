#pragma once

#include <kernel/yt/dynamic/fwd.h>
#include <kernel/yt/dynamic/ut/tables.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/gtest.h>
#include <util/system/env.h>


#define ASSERT_PROTO_EQ(a, b) \
    ASSERT_EQ((a).DebugString(), (b).DebugString())

namespace NYT {
namespace NProtoApiTest {

using namespace NProtoApi;
using ::TStringBuilder;

} // NProtoApiTest
} // NYT
