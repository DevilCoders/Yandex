#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <kernel/index_mapping/index_mapping.h>

#include <util/stream/file.h>
#include <util/generic/size_literals.h>

Y_UNIT_TEST_SUITE(IndexMapping) {
    Y_UNIT_TEST(ZeroSizeFiles) {
        EnableGlobalIndexFileMapping();
        {
            TFileOutput fo("test_zero_size");
        }
        UNIT_ASSERT(TFsPath("test_zero_size").Exists());
        UNIT_ASSERT(TFile("test_zero_size", EOpenModeFlag::RdOnly).GetLength() == 0);
        {
            TIndexPrefetchOptions opts;
            TIndexPrefetchResult result = PrefetchMappedIndex("test_zero_size", opts);
            UNIT_ASSERT(result.Locked == false);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr != nullptr);
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(GetMappedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("test_zero_size");
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
        {
            TIndexPrefetchOptions opts;
            opts.TryLock = true;
            TIndexPrefetchResult result = PrefetchMappedIndex("test_zero_size", opts);
            UNIT_ASSERT(result.Locked == true);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr != nullptr);
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(GetMappedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("test_zero_size");
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
    }

    Y_UNIT_TEST(ZeroSizeFilesUseThrottledLockMemory) {
        EnableGlobalIndexFileMapping();
        EnableThrottledReading(TThrottle::TOptions::FromMaxPerSecond(4_MB, TDuration::MilliSeconds(100)));
        {
            TFileOutput fo("test_zero_size");
        }
        UNIT_ASSERT(TFsPath("test_zero_size").Exists());
        UNIT_ASSERT(TFile("test_zero_size", EOpenModeFlag::RdOnly).GetLength() == 0);
        {
            TIndexPrefetchOptions opts;
            TIndexPrefetchResult result = PrefetchMappedIndex("test_zero_size", opts);
            UNIT_ASSERT(result.Locked == false);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr != nullptr);
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(GetMappedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("test_zero_size");
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
        {
            TIndexPrefetchOptions opts;
            opts.TryLock = true;
            TIndexPrefetchResult result = PrefetchMappedIndex("test_zero_size", opts);
            UNIT_ASSERT(result.Locked == true);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr != nullptr);
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(GetMappedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("test_zero_size");
            UNIT_ASSERT(NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
    }

    Y_UNIT_TEST(NoExistsFile) {
        {
            TIndexPrefetchOptions opts;
            TIndexPrefetchResult result;
            try {
                result = PrefetchMappedIndex("no_exists", opts);
            } catch (...) {

            }
            UNIT_ASSERT(result.Locked == false);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr == nullptr);
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("no_exists");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("no_exists");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
        {
            TIndexPrefetchOptions opts;
            opts.TryLock = true;
            TIndexPrefetchResult result;
            try {
                result = PrefetchMappedIndex("no_exists", opts);
            } catch (...) {

            }
            UNIT_ASSERT(result.Locked == false);
            UNIT_ASSERT(result.Prefetched == 0);
            UNIT_ASSERT(result.Addr == nullptr);
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UnlockMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            ReleaseMappedIndexFile("test_zero_size");
            UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("test_zero_size"));
            UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("test_zero_size"));
            UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
            UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
        }
    }

    Y_UNIT_TEST(ReleaseFake) {
        UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("aaa"));
        UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("aaa"));
        UnlockMappedIndexFile("aaa");
        UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("aaa"));
        UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("aaa"));
        ReleaseMappedIndexFile("aaa");
        UNIT_ASSERT(!NMappedFiles::HasMappedIndexFile("aaa"));
        UNIT_ASSERT(!NMappedFiles::HasLockedIndexFile("aaa"));
        UNIT_ASSERT(NMappedFiles::GetLockedFilesCount() == 0);
        UNIT_ASSERT(NMappedFiles::GetMappedFilesCount() == 0);
    }

    Y_UNIT_TEST(EnablePrechargedMapping) {
        EnableGlobalIndexFileMapping();

        const TFile notPrechargedFile("not_precharged_file", OpenAlways);
        TMemoryMap::EOpenMode notPrechargedFileMapMode = GetMappedIndexFile(notPrechargedFile.GetName())->GetMode();
        UNIT_ASSERT(!notPrechargedFileMapMode.HasFlags(TMemoryMap::oPrecharge));

        EnablePrechargedMapping();

        const TFile prechargedFile("precharged_file", OpenAlways);
        TMemoryMap::EOpenMode prechargeFileMapMode = GetMappedIndexFile(prechargedFile.GetName())->GetMode();
        UNIT_ASSERT(prechargeFileMapMode.HasFlags(TMemoryMap::oPrecharge));
    }

    Y_UNIT_TEST(EnablingPrechargedMappingDoesNotChangeOtherFlags) {
        EnableGlobalIndexFileMapping();

        const TFile notPrechargedFile("not_precharged_file", OpenAlways);
        TMemoryMap::EOpenMode notPrechargedFileMapMode = GetMappedIndexFile(notPrechargedFile.GetName())->GetMode();

        EnablePrechargedMapping();

        const TFile prechargedFile("precharged_file", OpenAlways);
        TMemoryMap::EOpenMode prechargedFileMapMode = GetMappedIndexFile(prechargedFile.GetName())->GetMode();

        UNIT_ASSERT(notPrechargedFileMapMode == prechargedFileMapMode.RemoveFlags(TMemoryMap::oPrecharge));
    }

    Y_UNIT_TEST(EnablingPrechargedMappingSavesWritableMapping) {
        EnableGlobalIndexFileMapping();
        EnableWritableMapping();
        EnablePrechargedMapping();

        const TFile prechargedFile("precharged_file", OpenAlways);
        TMemoryMap::EOpenMode prechargedFileMapMode = GetMappedIndexFile(prechargedFile.GetName())->GetMode();

        UNIT_ASSERT(prechargedFileMapMode.HasFlags(TMemoryMap::oRdWr));
        UNIT_ASSERT(prechargedFileMapMode.HasFlags(TMemoryMap::oPrecharge));
    }

    Y_UNIT_TEST(SavingStateAfterEnablingPrechargedMapping) {
        EnableGlobalIndexFileMapping();

        const TFile notPrechargedFile("not_precharged_file", OpenAlways);
        TMemoryMap::EOpenMode modeBeforeEnabling = GetMappedIndexFile(notPrechargedFile.GetName())->GetMode();

        EnablePrechargedMapping();

        TMemoryMap::EOpenMode modeAfterEnabling = GetMappedIndexFile(notPrechargedFile.GetName())->GetMode();
        UNIT_ASSERT(modeBeforeEnabling == modeAfterEnabling);
    }
}
