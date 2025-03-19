#include "storage_aio.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/storage.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/blockstore/libs/nvme/nvme.h>

#include <cloud/storage/core/libs/aio/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/file_io_service.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/folder/dirut.h>
#include <util/folder/tempdir.h>
#include <util/generic/scope.h>
#include <util/generic/size_literals.h>
#include <util/generic/string.h>
#include <util/system/file.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;
using namespace NNvme;

namespace {

////////////////////////////////////////////////////////////////////////////////

#define UNIT_ASSERT_SUCCEEDED(e) \
    UNIT_ASSERT_C(SUCCEEDED(e.GetCode()), e.GetMessage())

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);

TFuture<NProto::TReadBlocksLocalResponse> ReadBlocksLocal(
    IStorage& storage,
    ui64 startIndex,
    ui32 blockCount,
    ui32 blockSize,
    TGuardedSgList sglist)
{
    auto context = MakeIntrusive<TCallContext>();

    auto request = std::make_shared<NProto::TReadBlocksLocalRequest>();
    request->SetStartIndex(startIndex);
    request->SetBlocksCount(blockCount);
    request->BlockSize = blockSize;
    request->Sglist = std::move(sglist);

    return storage.ReadBlocksLocal(
        std::move(context),
        std::move(request));
}

TFuture<NProto::TWriteBlocksLocalResponse> WriteBlocksLocal(
    IStorage& storage,
    ui64 startIndex,
    ui32 blockCount,
    ui32 blockSize,
    TGuardedSgList sglist)
{
    auto context = MakeIntrusive<TCallContext>();

    auto request = std::make_shared<NProto::TWriteBlocksLocalRequest>();
    request->SetStartIndex(startIndex);
    request->BlocksCount = blockCount;
    request->BlockSize = blockSize;
    request->Sglist = std::move(sglist);

    return storage.WriteBlocksLocal(
        std::move(context),
        std::move(request));
}

TFuture<NProto::TZeroBlocksResponse> ZeroBlocks(
    IStorage& storage,
    ui64 startIndex,
    ui32 blockCount)
{
    auto context = MakeIntrusive<TCallContext>();

    auto request = std::make_shared<NProto::TZeroBlocksRequest>();
    request->SetStartIndex(startIndex);
    request->SetBlocksCount(blockCount);

    return storage.ZeroBlocks(
        std::move(context),
        std::move(request));

}

TFsPath TryGetRamDrivePath()
{
    auto p = GetRamDrivePath();
    return !p
        ? GetSystemTempDir()
        : p;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TAioStorageTest)
{
    void ShouldHandleLocalReadWriteRequestsImpl(ui32 blockSize)
    {
        const ui64 blockCount = 1024;
        const auto filePath = TryGetRamDrivePath() / "test";

        TFile fileData(filePath, EOpenModeFlag::CreateAlways);
        fileData.Resize(blockSize * blockCount);

        auto service = CreateAIOService();
        service->Start();
        Y_DEFER { service->Stop(); };

        auto provider = CreateAioStorageProvider(
            service,
            CreateNvmeManagerStub(),
            false   // directIO
        );

        NProto::TVolume volume;
        volume.SetDiskId(filePath);
        volume.SetBlockSize(blockSize);
        volume.SetBlocksCount(blockCount);

        auto future = provider->CreateStorage(
            volume,
            "",
            NProto::VOLUME_ACCESS_READ_WRITE);
        auto storage = future.GetValue();

        auto writeBuffer = storage->AllocateBuffer(blockSize);
        memset(writeBuffer.get(), 'a', blockSize);
        auto writeSgList = TGuardedSgList({{writeBuffer.get(), blockSize}});

        auto writeResponse = WriteBlocksLocal(
            *storage,
            0,
            1,
            blockSize,
            writeSgList
        ).GetValue(WaitTimeout);
        UNIT_ASSERT_SUCCEEDED(writeResponse.GetError());

        UNIT_ASSERT_VALUES_EQUAL(
            E_ARGUMENT,
            WriteBlocksLocal(
                *storage,
                blockCount,
                1,
                blockSize,
                writeSgList
            ).GetValue(WaitTimeout).GetError().GetCode());

        auto readBuffer = storage->AllocateBuffer(blockSize);
        auto readSgList = TGuardedSgList({{readBuffer.get(), blockSize}});

        auto readResponse = ReadBlocksLocal(
            *storage,
            0,
            1,
            blockSize,
            readSgList
        ).GetValue(WaitTimeout);
        UNIT_ASSERT_SUCCEEDED(readResponse.GetError());

        UNIT_ASSERT(memcmp(writeBuffer.get(), readBuffer.get(), blockSize) == 0);

        UNIT_ASSERT_VALUES_EQUAL(
            E_ARGUMENT,
            ReadBlocksLocal(
                *storage,
                blockCount,
                1,
                blockSize,
                readSgList
            ).GetValue(WaitTimeout).GetError().GetCode());
    }

    Y_UNIT_TEST(ShouldHandleLocalReadWriteRequests_512)
    {
        ShouldHandleLocalReadWriteRequestsImpl(512);
    }

    Y_UNIT_TEST(ShouldHandleLocalReadWriteRequests_1024)
    {
        ShouldHandleLocalReadWriteRequestsImpl(1024);
    }

    Y_UNIT_TEST(ShouldHandleLocalReadWriteRequests_4096)
    {
        ShouldHandleLocalReadWriteRequestsImpl(DefaultBlockSize);
    }

    Y_UNIT_TEST(ShouldHandleZeroBlocksRequests)
    {
        const ui32 blockSize = 4096;
        const ui64 blockCount = 32_MB / blockSize;
        const auto filePath = TryGetRamDrivePath() / "test";

        TFile fileData(filePath, EOpenModeFlag::CreateAlways);

        {
            TVector<char> buffer(blockSize, 'X');
            for (ui32 i = 0; i != blockCount; ++i) {
                fileData.Write(buffer.data(), blockSize);
            }

            fileData.Flush();
        }

        auto service = CreateAIOService();
        service->Start();
        Y_DEFER { service->Stop(); };

        auto provider = CreateAioStorageProvider(
            service,
            CreateNvmeManagerStub(),
            false   // directIO
        );

        NProto::TVolume volume;
        volume.SetDiskId(filePath);
        volume.SetBlockSize(blockSize);
        volume.SetBlocksCount(blockCount);

        auto future = provider->CreateStorage(
            volume,
            "",
            NProto::VOLUME_ACCESS_READ_WRITE);
        auto storage = future.GetValue();

        auto readBuffer = storage->AllocateBuffer(blockSize);
        auto readSgList = TGuardedSgList({{readBuffer.get(), blockSize}});

        auto verifyData = [&] (char c) {
            for (ui64 i = 0; i < blockCount; ++i) {
                auto response = ReadBlocksLocal(
                    *storage,
                    i,
                    1,
                    blockSize,
                    readSgList
                ).GetValue(WaitTimeout);
                UNIT_ASSERT_SUCCEEDED(response.GetError());

                UNIT_ASSERT_VALUES_EQUAL(
                    blockSize,
                    std::count(
                        readBuffer.get(),
                        readBuffer.get() + blockSize,
                        c));
            }
        };

        verifyData('X');

        UNIT_ASSERT_SUCCEEDED(ZeroBlocks(*storage, 0, blockCount)
            .GetValue(WaitTimeout).GetError());

        verifyData('\0');
    }

    Y_UNIT_TEST(ShouldHandleZeroBlocksRequestsForBigFiles)
    {
        const ui32 blockSize = 4_KB;
        const ui64 blockCount = 8_GB / blockSize;
        const auto filePath = TryGetRamDrivePath() / "test";

        TFile fileData(filePath, EOpenModeFlag::CreateAlways);
        fileData.Resize(blockSize * blockCount);

        auto service = CreateAIOService();
        service->Start();
        Y_DEFER { service->Stop(); };

        auto provider = CreateAioStorageProvider(
            service,
            CreateNvmeManagerStub(),
            false   // directIO
        );

        NProto::TVolume volume;
        volume.SetDiskId(filePath);
        volume.SetBlockSize(blockSize);
        volume.SetBlocksCount(blockCount);

        auto future = provider->CreateStorage(
            volume,
            "",
            NProto::VOLUME_ACCESS_READ_WRITE);
        auto storage = future.GetValue();

        auto writeBuffer = storage->AllocateBuffer(blockSize);
        memset(writeBuffer.get(), 'a', blockSize);
        auto writeSgList = TGuardedSgList({{writeBuffer.get(), blockSize}});

        const ui64 startIndex = 7_GB / blockSize;

        {
            auto writeResponse = WriteBlocksLocal(
                *storage,
                startIndex,
                1,
                blockSize,
                writeSgList
            ).GetValue(WaitTimeout);
            UNIT_ASSERT_SUCCEEDED(writeResponse.GetError());
        }

        auto readBuffer = storage->AllocateBuffer(blockSize);
        auto readSgList = TGuardedSgList({{readBuffer.get(), blockSize}});

        // verify data

        {
            auto response = ReadBlocksLocal(
                *storage,
                startIndex,
                1,
                blockSize,
                readSgList
            ).GetValue(WaitTimeout);
            UNIT_ASSERT_SUCCEEDED(response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(
                0,
                memcmp(writeBuffer.get(), readBuffer.get(), blockSize)
            );
        }

        // erase 10241 blocks (40MB + 4KB)

        UNIT_ASSERT_SUCCEEDED(ZeroBlocks(*storage, startIndex, 10241)
            .GetValue(WaitTimeout).GetError());

        // verify

        auto response = ReadBlocksLocal(
            *storage,
            startIndex,
            1,
            blockSize,
            readSgList
        ).GetValue(WaitTimeout);
        UNIT_ASSERT_SUCCEEDED(response.GetError());

        UNIT_ASSERT_VALUES_EQUAL(
            blockSize,
            std::count(
                readBuffer.get(),
                readBuffer.get() + blockSize,
                '\0'));

        // should reject too big requests

        {
            const ui32 len = 64_MB;
            auto tooBigBuffer = storage->AllocateBuffer(len);
            auto sgList = TGuardedSgList({{ tooBigBuffer.get(), len }});

            UNIT_ASSERT_EQUAL(
                E_ARGUMENT,
                ReadBlocksLocal(
                    *storage,
                    startIndex,
                    len / blockSize,
                    blockSize,
                    sgList
                ).GetValue(WaitTimeout).GetError().GetCode()
            );

            UNIT_ASSERT_EQUAL(
                E_ARGUMENT,
                WriteBlocksLocal(
                    *storage,
                    startIndex,
                    len / blockSize,
                    blockSize,
                    sgList
                ).GetValue(WaitTimeout).GetError().GetCode()
            );
        }
    }
}

}   // namespace NCloud::NBlockStore::NServer
