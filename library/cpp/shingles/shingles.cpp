#include "word_hash.h"
#include "shingler.h"
#include "readable.h"
#include "crossing.h"

// Test instances to be compiled

using namespace NCrossing;

struct TFakeResult {
    typedef TLineToken TToken;

    void Do(TToken, TToken, ui32) {
        return;
    }
};

void Test() {
    TVector<TWord<ui64>> data;
    TFakeResult result;
    FindCrossing(result, data);
}
