#include "device_handler.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/common/sglist.h>
#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/storage_test.h>
#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TTestEnvironment
{
private:
    const ui64 BlocksCount;
    const ui32 BlockSize;
    const ui32 SectorSize;

    TSgList SgList;
    TVector<TString> Blocks;
    TPromise<void> WriteTrigger;
    IDeviceHandlerPtr DeviceHandler;

    TVector<TFuture<NProto::TError>> Futures;

public:
    TTestEnvironment(
            ui64 blocksCount, 
            ui32 blockSize, 
            ui32 sectorsPerBlock)
        : BlocksCount(blocksCount)
        , BlockSize(blockSize)
        , SectorSize(BlockSize / sectorsPerBlock)
    {
        UNIT_ASSERT(SectorSize * sectorsPerBlock == BlockSize);

        SgList = ResizeBlocks(Blocks, BlocksCount, TString(BlockSize, '0'));
        WriteTrigger = NewPromise<void>();

        auto testStorage = std::make_shared<TTestStorage>();
        testStorage->ReadBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
                Y_UNUSED(ctx);

                auto startIndex = request->GetStartIndex();
                auto guard = request->Sglist.Acquire();
                const auto& dst = guard.Get();

                for (const auto& buffer: dst) {
                    Y_VERIFY(buffer.Size() % BlockSize == 0);
                }

                auto src = SgList;
                src.erase(src.begin(), src.begin() + startIndex);
                auto sz = SgListCopy(src, dst);
                UNIT_ASSERT(sz == request->GetBlocksCount() * BlockSize);

                return MakeFuture(NProto::TReadBlocksLocalResponse());
            };
        testStorage->WriteBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
                Y_UNUSED(ctx);

                auto future = WriteTrigger.GetFuture();
                return future.Apply([=] (const auto& f) {
                    Y_UNUSED(f);

                    auto startIndex = request->GetStartIndex();
                    auto guard = request->Sglist.Acquire();
                    const auto& src = guard.Get();

                    for (const auto& buffer: src) {
                        Y_VERIFY(buffer.Size() % BlockSize == 0);
                    }

                    auto dst = SgList;
                    dst.erase(dst.begin(), dst.begin() + startIndex);
                    auto sz = SgListCopy(src, dst);
                    UNIT_ASSERT(sz == request->BlocksCount * BlockSize);

                    return NProto::TWriteBlocksLocalResponse();
                });
            };
        testStorage->ZeroBlocksHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TZeroBlocksRequest> request) {
                Y_UNUSED(ctx);

                auto future = WriteTrigger.GetFuture();
                return future.Apply([=] (const auto& f) {
                    Y_UNUSED(f);

                    auto startIndex = request->GetStartIndex();
                    TSgList src(
                        request->GetBlocksCount(),
                        TBlockDataRef::CreateZeroBlock(BlockSize));

                    auto dst = SgList;
                    dst.erase(dst.begin(), dst.begin() + startIndex);
                    auto sz = SgListCopy(src, dst);
                    UNIT_ASSERT(sz == request->GetBlocksCount() * BlockSize);

                    return NProto::TZeroBlocksResponse();
                });
            };

        DeviceHandler = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            std::move(testStorage),
            "testClientId",
            BlockSize,
            1024,
            false,
            MakeIntrusive<NMonitoring::TCounterForPtr>());
    }

    void WriteSectors(ui64 firstSector, ui64 totalSectors, char data)
    {
        auto buffer = TString(totalSectors * SectorSize, data);
        TSgList sgList;
        for (size_t i = 0; i < totalSectors; ++i) {
            sgList.emplace_back(buffer.data() + i * SectorSize, SectorSize);
        }

        auto future = DeviceHandler->Write(
            MakeIntrusive<TCallContext>(),
            firstSector * SectorSize,
            totalSectors * SectorSize,
            TGuardedSgList(sgList))
        .Apply([buf = std::move(buffer)] (const auto& f) {
            Y_UNUSED(buf);

            return f.GetValue().GetError();
        });

        Futures.push_back(future);
    }

    void ZeroSectors(ui64 firstSector, ui64 totalSectors)
    {
        auto future = DeviceHandler->Zero(
            MakeIntrusive<TCallContext>(),
            firstSector * SectorSize,
            totalSectors * SectorSize)
        .Apply([] (const auto& f) {
            return f.GetValue().GetError();
        });

        Futures.push_back(future);
    }

    void RunWriteService()
    {
        WriteTrigger.SetValue();

        for (const auto& future: Futures) {
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }
    }

    void ReadSectorsAndCheck(
        ui64 firstSector,
        ui64 totalSectors,
        const TString& expected)
    {
        UNIT_ASSERT(expected.size() == totalSectors);

        TString buffer = TString::Uninitialized(totalSectors * SectorSize);
        TSgList sgList;
        for (size_t i = 0; i < totalSectors; ++i) {
            sgList.emplace_back(buffer.data() + i * SectorSize, SectorSize);
        }
        TString checkpointId;

        auto future = DeviceHandler->Read(
            MakeIntrusive<TCallContext>(),
            firstSector * SectorSize,
            totalSectors * SectorSize,
            TGuardedSgList(sgList),
            checkpointId);

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(!HasError(response));

        const char* ptr = buffer.data();
        for (const char& c: expected) {
            for (size_t i = 0; i < SectorSize; ++i) {

                if (c == 'Z') {
                    UNIT_ASSERT(*ptr == 0);
                } else {
                    UNIT_ASSERT(*ptr == c);
                }
                ++ptr;
            }
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDeviceHandlerTest)
{
    Y_UNIT_TEST(ShouldHandleUnalignedReadRequests)
    {
        TTestEnvironment env(2, DefaultBlockSize, 8);

        env.WriteSectors(0, 4, 'a');
        env.WriteSectors(4, 4, 'b');
        env.WriteSectors(8, 4, 'c');
        env.WriteSectors(12, 4, 'd');

        env.RunWriteService();
        env.ReadSectorsAndCheck(0, 16, "aaaabbbbccccdddd");
        env.ReadSectorsAndCheck(8, 1, "c");
        env.ReadSectorsAndCheck(3, 4, "abbb");
        env.ReadSectorsAndCheck(7, 5, "bcccc");
        env.ReadSectorsAndCheck(3, 11, "abbbbccccdd");
    }

    Y_UNIT_TEST(ShouldHandleReadModifyWriteRequests)
    {
        TTestEnvironment env(2, DefaultBlockSize, 8);

        env.WriteSectors(1, 2, 'a');
        env.WriteSectors(4, 4, 'b');
        env.ZeroSectors(5, 2);

        env.RunWriteService();
        env.ReadSectorsAndCheck(0, 8, "0aa0bZZb");
    }

    Y_UNIT_TEST(ShouldHandleAlignedAndRMWRequests)
    {
        TTestEnvironment env(2, DefaultBlockSize, 8);

        env.ZeroSectors(0, 8);
        env.WriteSectors(1, 1, 'a');
        env.WriteSectors(5, 2, 'b');

        env.RunWriteService();
        env.ReadSectorsAndCheck(0, 8, "ZaZZZbbZ");
    }

    Y_UNIT_TEST(ShouldHandleRMWAndAlignedRequests)
    {
        TTestEnvironment env(2, DefaultBlockSize, 8);

        env.ZeroSectors(1, 1);
        env.WriteSectors(5, 2, 'a');
        env.WriteSectors(0, 8, 'b');

        env.RunWriteService();
        env.ReadSectorsAndCheck(0, 8, "bbbbbbbb");
    }

    Y_UNIT_TEST(ShouldHandleComplicatedRMWRequests)
    {
        TTestEnvironment env(2, DefaultBlockSize, 8);

        env.WriteSectors(5, 2, 'a');
        env.ZeroSectors(13, 2);
        env.WriteSectors(0, 16, 'c');
        env.WriteSectors(1, 2, 'd');
        env.ZeroSectors(6, 4);
        env.WriteSectors(12, 3, 'f');
        env.WriteSectors(0, 8, 'g');

        env.RunWriteService();
        env.ReadSectorsAndCheck(0, 16, "ggggggggZZccfffc");
    }

    Y_UNIT_TEST(ShouldSliceHugeZeroRequest)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;
        const ui64 deviceBlocksCount = 8*1024;
        const ui64 blocksCountLimit = deviceBlocksCount / 4;

        auto storage = std::make_shared<TTestStorage>();

        auto deviceHandler = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            blocksCountLimit,
            false,
            MakeIntrusive<NMonitoring::TCounterForPtr>());

        std::array<bool, deviceBlocksCount> zeroBlocks;
        for (auto& zeroBlock: zeroBlocks) {
            zeroBlock = false;
        }

        ui32 requestCounter = 0;

        storage->ZeroBlocksHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TZeroBlocksRequest> request)
        {
            Y_UNUSED(callContext);

            UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
            UNIT_ASSERT(request->GetBlocksCount() <= blocksCountLimit);
            UNIT_ASSERT(request->GetStartIndex() + request->GetBlocksCount() <= deviceBlocksCount);

            for (ui32 i = 0; i < request->GetBlocksCount(); ++i) {
                auto index = request->GetStartIndex() + i;
                auto& zeroBlock = zeroBlocks[index];

                UNIT_ASSERT(!zeroBlock);
                zeroBlock = true;
            }

            ++requestCounter;
            return MakeFuture<NProto::TZeroBlocksResponse>();
        };

        ui64 startIndex = 3;
        ui64 blocksCount = deviceBlocksCount - 9;

        auto future = deviceHandler->Zero(
            MakeIntrusive<TCallContext>(),
            startIndex * blockSize,
            blocksCount * blockSize);

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(!HasError(response));

        UNIT_ASSERT(requestCounter > 1);

        for (ui64 i = 0; i < deviceBlocksCount; ++i) {
            const auto& zeroBlock = zeroBlocks[i];
            auto contains = (startIndex <= i && i < (startIndex + blocksCount));
            UNIT_ASSERT(zeroBlock == contains);
        }
    }

    Y_UNIT_TEST(ShouldHandleAlignedRequestsWhenUnalignedRequestsDisabled)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;

        auto storage = std::make_shared<TTestStorage>();

        auto device = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            1024,
            true,
            nullptr);

        ui32 startIndex = 42;
        ui32 blocksCount = 17;

        auto buffer = TString::Uninitialized(blocksCount * DefaultBlockSize);
        TSgList sgList{{ buffer.data(), buffer.size() }};

        storage->ReadBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
                Y_UNUSED(ctx);

                UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
                UNIT_ASSERT(request->GetStartIndex() == startIndex);
                UNIT_ASSERT(request->GetBlocksCount() == blocksCount);

                return MakeFuture<NProto::TReadBlocksLocalResponse>();
            };

        {
            auto future = device->Read(
                MakeIntrusive<TCallContext>(),
                startIndex * blockSize,
                blocksCount * blockSize,
                TGuardedSgList(sgList),
                {});

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        storage->WriteBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
                Y_UNUSED(ctx);
                
                UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
                UNIT_ASSERT(request->GetStartIndex() == startIndex);
                UNIT_ASSERT(request->BlocksCount == blocksCount);

                return MakeFuture<NProto::TWriteBlocksLocalResponse>();
            };

        {
            auto future = device->Write(
                MakeIntrusive<TCallContext>(),
                startIndex * blockSize,
                blocksCount * blockSize,
                TGuardedSgList(sgList));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        storage->ZeroBlocksHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TZeroBlocksRequest> request) {
                Y_UNUSED(ctx);

                UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
                UNIT_ASSERT(request->GetStartIndex() == startIndex);
                UNIT_ASSERT(request->GetBlocksCount() == blocksCount);

                return MakeFuture<NProto::TZeroBlocksResponse>();
            };

        {
            auto future = device->Zero(
                MakeIntrusive<TCallContext>(),
                startIndex * blockSize,
                blocksCount * blockSize);

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }
    }

    Y_UNIT_TEST(ShouldNotHandleUnalignedRequestsWhenUnalignedRequestsDisabled)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;

        auto storage = std::make_shared<TTestStorage>();

        auto device = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            1024,
            true,
            nullptr);

        {
            auto future = device->Read(
                MakeIntrusive<TCallContext>(),
                blockSize * 5 / 2,
                blockSize * 8 / 3,
                TGuardedSgList(),
                {});

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }

        {
            auto future = device->Write(
                MakeIntrusive<TCallContext>(),
                blockSize * 3,
                blockSize * 7 / 3,
                TGuardedSgList());

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }

        {
            auto future = device->Zero(
                MakeIntrusive<TCallContext>(),
                blockSize * 3 / 2,
                blockSize * 4);

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }
    }

    Y_UNIT_TEST(ShouldSliceHugeUnalignedZeroRequest)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;
        const ui64 deviceBlocksCount = 12;
        const ui64 blocksCountLimit = deviceBlocksCount / 4;

        TString device(deviceBlocksCount * blockSize, 1);
        TString zeroBlock(blockSize, 0);

        auto storage = std::make_shared<TTestStorage>();

        auto deviceHandler = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            blocksCountLimit,
            false,
            MakeIntrusive<NMonitoring::TCounterForPtr>());

        storage->ZeroBlocksHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TZeroBlocksRequest> request)
        {
            Y_UNUSED(callContext);

            UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
            UNIT_ASSERT(request->GetBlocksCount() <= blocksCountLimit);
            UNIT_ASSERT(request->GetStartIndex() + request->GetBlocksCount() <= deviceBlocksCount);

            TSgList src(
                request->GetBlocksCount(),
                TBlockDataRef(zeroBlock.Data(), zeroBlock.Size()));

            TBlockDataRef dst(
                device.Data() + request->GetStartIndex() * blockSize,
                src.size() * blockSize);

            auto bytesCount = SgListCopy(src, dst);
            UNIT_ASSERT_VALUES_EQUAL(dst.Size(), bytesCount);

            return MakeFuture(NProto::TZeroBlocksResponse());
        };

        storage->WriteBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TWriteBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);

            UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
            UNIT_ASSERT(request->BlocksCount <= blocksCountLimit);
            UNIT_ASSERT(request->GetStartIndex() + request->BlocksCount <= deviceBlocksCount);

            TBlockDataRef dst(
                device.Data() + request->GetStartIndex() * blockSize,
                request->BlocksCount * blockSize);

            auto guard = request->Sglist.Acquire();
            UNIT_ASSERT(guard);

            auto bytesCount = SgListCopy(guard.Get(), dst);
            UNIT_ASSERT_VALUES_EQUAL(dst.Size(), bytesCount);

            return MakeFuture(NProto::TWriteBlocksResponse());
        };

        storage->ReadBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TReadBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);

            UNIT_ASSERT(request->GetHeaders().GetClientId() == clientId);
            UNIT_ASSERT(request->GetBlocksCount() <= blocksCountLimit);
            UNIT_ASSERT(request->GetStartIndex() + request->GetBlocksCount() <= deviceBlocksCount);

            TBlockDataRef src(
                device.Data() + request->GetStartIndex() * blockSize,
                request->GetBlocksCount() * blockSize);

            NProto::TReadBlocksResponse response;

            auto guard = request->Sglist.Acquire();
            UNIT_ASSERT(guard);

            auto bytesCount = SgListCopy(src, guard.Get());
            UNIT_ASSERT_VALUES_EQUAL(src.Size(), bytesCount);

            return MakeFuture(std::move(response));
        };

        ui64 from = 4567;
        ui64 length = deviceBlocksCount * blockSize - 9876;

        auto future = deviceHandler->Zero(
            MakeIntrusive<TCallContext>(),
            from,
            length);

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response);

        for (ui64 i = 0; i < deviceBlocksCount * blockSize; ++i) {
            bool isZero = (from <= i && i < (from + length));
            UNIT_ASSERT_VALUES_EQUAL_C(isZero ? 0 : 1, device[i], i);
        }
    }

    Y_UNIT_TEST(ShouldReturnErrorForHugeUnalignedReadWriteRequests)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;

        auto storage = std::make_shared<TTestStorage>();

        auto deviceHandler = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            1024,
            false,
            MakeIntrusive<NMonitoring::TCounterForPtr>());

        storage->WriteBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TWriteBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);
            Y_UNUSED(request);
            return MakeFuture(NProto::TWriteBlocksResponse());
        };

        storage->ReadBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TReadBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);
            Y_UNUSED(request);
            return MakeFuture(NProto::TReadBlocksResponse());
        };

        ui64 from = 1;
        ui64 length = 64_MB;

        {
            TGuardedSgList sgList;
            TString checkpointId;
            auto future = deviceHandler->Read(
                MakeIntrusive<TCallContext>(),
                from,
                length,
                sgList,
                checkpointId);

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }

        {
            TGuardedSgList sgList;
            auto future = deviceHandler->Write(
                MakeIntrusive<TCallContext>(),
                from,
                length,
                sgList);

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }
    }

    Y_UNIT_TEST(ShouldReturnErrorForInvalidBufferSize)
    {
        const auto clientId = "testClientId";
        const ui32 blockSize = DefaultBlockSize;

        auto storage = std::make_shared<TTestStorage>();

        auto deviceHandler = CreateDefaultDeviceHandlerFactory()->CreateDeviceHandler(
            storage,
            clientId,
            blockSize,
            1024,
            false,
            MakeIntrusive<NMonitoring::TCounterForPtr>());

        storage->WriteBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TWriteBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);
            Y_UNUSED(request);
            return MakeFuture(NProto::TWriteBlocksResponse());
        };

        storage->ReadBlocksLocalHandler = [&] (
            TCallContextPtr callContext,
            std::shared_ptr<NProto::TReadBlocksLocalRequest> request)
        {
            Y_UNUSED(callContext);
            Y_UNUSED(request);
            return MakeFuture(NProto::TReadBlocksResponse());
        };

        ui64 from = 0;
        ui64 length = blockSize;
        auto buffer = TString::Uninitialized(blockSize + 1);
        TSgList sgList{{ buffer.data(), buffer.size() }};

        {
            TString checkpointId;
            auto future = deviceHandler->Read(
                MakeIntrusive<TCallContext>(),
                from,
                length,
                TGuardedSgList(sgList),
                checkpointId);

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }

        {
            auto future = deviceHandler->Write(
                MakeIntrusive<TCallContext>(),
                from,
                length,
                TGuardedSgList(sgList));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response));
            UNIT_ASSERT(response.GetError().GetCode() == E_ARGUMENT);
        }
    }
}

}   // namespace NCloud::NBlockStore
