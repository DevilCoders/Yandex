#include "data_generator.h"

#include <util/generic/ylimits.h>
#include <util/random/random.h>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

struct TRandomDataGenerator final
    : IDataGenerator
{
    void Generate(IAllocator::TBlock* block) override
    {
        memset(block->Data, 1 + RandomNumber<ui8>(Max<ui8>()), block->Len);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TConstDataGenerator final
    : IDataGenerator
{
    ui8 Value;

    TConstDataGenerator(ui8 value)
        : Value(value)
    {
    }

    void Generate(IAllocator::TBlock* block) override
    {
        memset(block->Data, Value, block->Len);
    }
};

////////////////////////////////////////////////////////////////////////////////

IDataGeneratorPtr CreateRandomDataGenerator()
{
    return std::make_shared<TRandomDataGenerator>();
}

IDataGeneratorPtr CreateConstDataGenerator(ui8 value)
{
    return std::make_shared<TConstDataGenerator>(value);
}

}   // namespace NCloud::NBlockStore::NLoadTest
