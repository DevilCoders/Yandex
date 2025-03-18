#include "generator.h"

using namespace NXdeltaAggregateColumn;

TDataPtr RandData(size_t size)
{
    auto result = TDataPtr(reinterpret_cast<ui8*>(malloc(size)));
    for (size_t i = 0; i < size; ++i) {
        result.get()[i] = rand() & 0xFF;
    }
    return result;
}

TDataPtr RandDataModification(const ui8* in, size_t inSize, size_t size) {
    auto result = TDataPtr(reinterpret_cast<ui8*>(malloc(size)));
    for (size_t i = 0; i < size; ++i) {
        if (rand() % 10 < 1 || 0 == inSize) {
            result.get()[i] = rand() & 0xFF;
        } else {
            result.get()[i] = in[i % inSize];
        }
    }
    return result;
}
