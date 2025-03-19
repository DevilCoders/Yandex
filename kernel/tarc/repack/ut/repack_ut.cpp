#include <kernel/tarc/repack/repack.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <fstream>

static bool CheckIfReversible(TBlob original, NRepack::TCodec& codec) {
    const TBlob repacked = NRepack::RepackArchiveDocText(original, codec);
    const TBlob inversedZeroComp = NRepack::RestoreArchiveDocText(repacked, codec, 0);
    const TBlob repackedAgain = NRepack::RepackArchiveDocText(inversedZeroComp, codec);
    const TBlob inversed = NRepack::RestoreArchiveDocText(repackedAgain, codec, 6);

    return original.AsStringBuf() == inversed.AsStringBuf();
}

static TBlob GetArc(size_t num) {
    return TBlob::FromFile("./arcs/arc-" + std::to_string(num) + ".bin");
}

Y_UNIT_TEST_SUITE(TRepackSimpleTest) {
    Y_UNIT_TEST(TestEmptyRepack) {
        NRepack::TCodecWithInfo codec = NRepack::TCodecWithInfo::GetLatest();
        TArchiveTextHeader hdr{};
        TBlob empty = TBlob::NoCopy(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        CheckIfReversible(empty, codec.codec);
    }

    Y_UNIT_TEST(TestRepackIsReversible) {
        NRepack::TCodecWithInfo codec = NRepack::TCodecWithInfo::GetLatest();
        for (size_t i = 0; i < 3000; ++i) {
            UNIT_ASSERT(CheckIfReversible(GetArc(i), codec.codec));
        }
    }

    Y_UNIT_TEST(TestRepackIsAtLeastSomewhatEffective) {
        NRepack::TCodecWithInfo codec = NRepack::TCodecWithInfo::GetLatest();
        ui64 originalSize = 0, repackedSize = 0;
        for (size_t i = 0; i < 3000; ++i) {
            const TBlob original = GetArc(i);
            const TBlob repacked = NRepack::RepackArchiveDocText(original, codec.codec);

            originalSize += original.Size();
            repackedSize += repacked.Size();
        }

        UNIT_ASSERT(static_cast<double>(repackedSize) / originalSize < 0.85);
    }
}
