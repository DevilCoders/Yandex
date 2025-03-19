#pragma once

#include "public.h"

#include <util/memory/alloc.h>
#include <util/system/defaults.h>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

struct IDataGenerator
{
    virtual ~IDataGenerator() = default;

    virtual void Generate(IAllocator::TBlock* block) = 0;
};

////////////////////////////////////////////////////////////////////////////////

IDataGeneratorPtr CreateRandomDataGenerator();
IDataGeneratorPtr CreateConstDataGenerator(ui8 value);

}   // namespace NCloud::NBlockStore::NLoadTest
