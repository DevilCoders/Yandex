#include "replica_table.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TReplicaTableTest)
{
    Y_UNIT_TEST(ShouldTryReplaceDevice)
    {
        TReplicaTable t;
        t.AddReplica("disk-1", {"d1-1", "d1-2", "d1-3", "d1-4"});
        t.AddReplica("disk-1", {"d2-1", "d2-2", "d2-3", "d2-4"});
        t.AddReplica("disk-1", {"d3-1", "d3-2", "d3-3", "d3-4"});
        t.MarkReplacementDevice("disk-1", "d2-2", true);
        UNIT_ASSERT(t.IsReplacementAllowed("disk-1", "d1-2"));
        UNIT_ASSERT(t.ReplaceDevice("disk-1", "d1-2", "d1-2'"));
        UNIT_ASSERT(!t.IsReplacementAllowed("disk-1", "d1-2"));
        UNIT_ASSERT(!t.ReplaceDevice("disk-1", "d1-2", "d1-2'"));
        UNIT_ASSERT(!t.IsReplacementAllowed("disk-1", "d3-2"));
        UNIT_ASSERT(t.IsReplacementAllowed("disk-1", "d1-2'"));
        UNIT_ASSERT(t.ReplaceDevice("disk-1", "d1-2'", "d1-2''"));
        UNIT_ASSERT(t.IsReplacementAllowed("disk-1", "d2-2"));
        UNIT_ASSERT(t.ReplaceDevice("disk-1", "d2-2", "d2-2'"));
        UNIT_ASSERT(!t.IsReplacementAllowed("disk-1", "d3-2"));
        t.MarkReplacementDevice("disk-1", "d2-2'", false);
        UNIT_ASSERT(t.IsReplacementAllowed("disk-1", "d3-2"));
        UNIT_ASSERT(t.ReplaceDevice("disk-1", "d3-2", "d3-2'"));

        UNIT_ASSERT(t.RemoveMirroredDisk("disk-1"));
        UNIT_ASSERT(!t.RemoveMirroredDisk("disk-1"));
    }
}

}   // namespace NCloud::NBlockStore::NStorage
