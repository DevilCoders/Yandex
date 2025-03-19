#include "fresh_blocks.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TFreshBlockVisitor final
    : public IFreshBlockVisitor
{
private:
    TVector<TBlock> Blocks;

public:
    void Accept(const TBlock& block, TStringBuf blockData) override
    {
        Y_UNUSED(blockData);
        Blocks.push_back(block);
    }

    TVector<TBlock> Finish()
    {
        return std::move(Blocks);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TFreshBlocksTest)
{
    Y_UNIT_TEST(ShouldStoreBlocks)
    {
        ui64 nodeId = 1;
        ui32 blockIndex = 123;

        TFreshBlocks freshBlocks;
        freshBlocks.AddBlock(nodeId, blockIndex, "x", 1);

        auto block = freshBlocks.FindBlock(nodeId, blockIndex, 1);
        UNIT_ASSERT(block);

        UNIT_ASSERT_VALUES_EQUAL(block->NodeId, nodeId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockIndex, blockIndex);
        UNIT_ASSERT_VALUES_EQUAL(block->MinCommitId, 1);
        UNIT_ASSERT_VALUES_EQUAL(block->MaxCommitId, InvalidCommitId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockData, "x");

        block = freshBlocks.FindBlock(nodeId, blockIndex + 1, 1);
        UNIT_ASSERT(!block);

        freshBlocks.AddBlock(nodeId + 2, blockIndex, "y", 1);
        block = freshBlocks.FindBlock(nodeId + 1, blockIndex, 1);
        UNIT_ASSERT(!block);

        UNIT_ASSERT(!freshBlocks.FindBlock(nodeId + 1, blockIndex));

        block = freshBlocks.FindBlock(nodeId + 2, blockIndex, 1);
        UNIT_ASSERT(block);

        UNIT_ASSERT_VALUES_EQUAL(block->NodeId, nodeId + 2);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockIndex, blockIndex);
        UNIT_ASSERT_VALUES_EQUAL(block->MinCommitId, 1);
        UNIT_ASSERT_VALUES_EQUAL(block->MaxCommitId, InvalidCommitId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockData, "y");

        UNIT_ASSERT(freshBlocks.FindBlock(nodeId + 2, blockIndex));
    }

    Y_UNIT_TEST(ShouldOverwriteBlocks)
    {
        ui64 nodeId = 1;
        ui32 blockIndex = 123;

        TFreshBlocks freshBlocks;
        freshBlocks.AddBlock(nodeId, blockIndex, "x", 1);

        ui64 minCommitId = freshBlocks.MarkBlockDeleted(nodeId, blockIndex, 2);
        UNIT_ASSERT_VALUES_EQUAL(minCommitId, 1);

        freshBlocks.AddBlock(nodeId, blockIndex, "y", 2);

        auto block = freshBlocks.FindBlock(nodeId, blockIndex, 1);
        UNIT_ASSERT(block);

        UNIT_ASSERT_VALUES_EQUAL(block->NodeId, nodeId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockIndex, blockIndex);
        UNIT_ASSERT_VALUES_EQUAL(block->MinCommitId, 1);
        UNIT_ASSERT_VALUES_EQUAL(block->MaxCommitId, 2);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockData, "x");

        block = freshBlocks.FindBlock(nodeId, blockIndex, 2);
        UNIT_ASSERT(block);

        UNIT_ASSERT_VALUES_EQUAL(block->NodeId, nodeId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockIndex, blockIndex);
        UNIT_ASSERT_VALUES_EQUAL(block->MinCommitId, 2);
        UNIT_ASSERT_VALUES_EQUAL(block->MaxCommitId, InvalidCommitId);
        UNIT_ASSERT_VALUES_EQUAL(block->BlockData, "y");
    }

    Y_UNIT_TEST(ShouldFindBlocks)
    {
        ui64 nodeId = 1;
        ui32 blockIndex = 123;

        TFreshBlocks freshBlocks;
        for (size_t i = 0; i < 10; ++i) {
            freshBlocks.AddBlock(nodeId, blockIndex + i, "x", 1 + i);
        }

        {
            TFreshBlockVisitor visitor;
            freshBlocks.FindBlocks(visitor, nodeId, blockIndex, 10);

            auto blocks = visitor.Finish();
            UNIT_ASSERT_VALUES_EQUAL(blocks.size(), 10);
        }

        {
            TFreshBlockVisitor visitor;
            freshBlocks.FindBlocks(visitor, nodeId, blockIndex, 10, 5);

            auto blocks = visitor.Finish();
            UNIT_ASSERT_VALUES_EQUAL(blocks.size(), 5);
        }
    }
}

}   // namespace NCloud::NFileStore::NStorage
