#include <library/cpp/malloc/calloc/alloc_header.h>
#include <library/cpp/malloc/calloc/calloc_profile_scan/stack_decoder.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TStackEncoding) {
    Y_UNIT_TEST(Smoke) {
        const TVector<ui64> testData {
            0x0000000029C6E5B4,
            0x0000000029C6E7D8,
            0x0000000029C6D6BB,
            0x0000000029C6CEEE,
            0x0000000029C6C1E0,
            0x0000000029C08E5A,
            0x000000002589BA9E,
            0x00000000258962A0,
            0x000000001DC1E768,
            0x000000001DC1CDC9,
            0x000000001DC17065,
            0x000000001DC70429,
            0x000000001DC718E2,
        };
        ui8 buffer[80];
        size_t bufferPos = 0;
        i64 prevRip = NCalloc::DIFF_ENCODING_START;
        for (size_t i = 0; i < testData.size(); ++i) {
            const i64 rip = (i64)testData[i];
            const i64 ripDiff = rip - prevRip;
            prevRip = rip;
            // +1 чтобы не записывать 0, который является признаком конца данных
            const ui64 zigZaggedRipDiff = NCalloc::NPrivate::ZigZagEncode(ripDiff) + 1;
            const size_t varIntSize = NCalloc::NPrivate::VarintEncode(zigZaggedRipDiff, buffer + bufferPos, sizeof(buffer) - bufferPos);
            if (Y_UNLIKELY(!varIntSize)) {
                break;
            }
            bufferPos += varIntSize;
        }
        memset(buffer + bufferPos, 0, sizeof(buffer) - bufferPos);

        TVector<ui64> decodedData;
        const ui8* bufferPtr = buffer;
        const ui8* endOfBuffer = buffer + sizeof(buffer);
        prevRip = NCalloc::DIFF_ENCODING_START;
        while (bufferPtr != endOfBuffer) {
            const ui64 zigZaggedRipDiff = VarIntDecode(&bufferPtr, endOfBuffer - bufferPtr);
            if (!zigZaggedRipDiff) {
                break;
            }
            const i64 ripDiff = ZigZagDecode(zigZaggedRipDiff - 1);
            const i64 rip = prevRip + ripDiff;
            prevRip = rip;
            decodedData.push_back(rip);
        }

        UNIT_ASSERT_VALUES_EQUAL(testData, decodedData);
    }
}
