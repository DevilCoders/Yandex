#include "archive_test_base.h"
#include "multipart.h"
#include "archive_fat.h"

#include <kernel/multipart_archive/archive_impl/part_optimization.h>
#include <kernel/multipart_archive/config/config.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NRTYArchive;
using namespace NRTYArchiveTest;

namespace {

    static const ui64 UNLIMITED_SIZE = NRTYArchive::IArchivePart::TConstructContext::NOT_LIMITED_PART;

    class TPartStateMock: public IPartState {
    public:
        TPartStateMock(ui32 docsCount, ui64 sizeInBytes, ui32 headerSize, ui64 partSizeLimit, TFsPath path = "")
            : DocsCount(docsCount)
            , SizeInBytes(sizeInBytes)
            , HeaderSize(headerSize)
            , PartSizeLimit(partSizeLimit)
            , Path(path)
        {
        }

        ui64 GetDocsCount(bool withRemoved) const final {
            return withRemoved ? HeaderSize : DocsCount;
        }

        ui64 GetSizeInBytes() const final {
            return SizeInBytes;
        }

        ui64 GetPartSizeLimit() const final {
            return PartSizeLimit;
        }

        TFsPath GetPath() const final {
            return Path;
        }

    private:
        ui32 DocsCount = 0;
        ui64 SizeInBytes = 0;
        ui32 HeaderSize = 0;
        ui64 PartSizeLimit = 0;
        TFsPath Path;
    };

    static void RecordVisit(ui32 partNum, TPartOptimizationCheckVisitor& visitor, ui32 docsCount, ui64 sizeInBytes, ui32 headerSize, ui64 partSizeLimit) {
        TPartStateMock part(docsCount, sizeInBytes, headerSize, partSizeLimit);
        visitor.VisitPart(partNum, part);
    }

} // namespace

Y_UNIT_TEST_SUITE(TRtyOptimizationAlgorithmSuite) {

    Y_UNIT_TEST(TestOptimizeNothing) {
        NRTYArchive::TOptimizationOptions opts;
        TPartOptimizationCheckVisitor visitor(opts);
        visitor.Start();
        RecordVisit(0, visitor, 10, 100, 10, UNLIMITED_SIZE);
        RecordVisit(1, visitor, 20, 100, 20, UNLIMITED_SIZE);
        UNIT_ASSERT(visitor.GetOptimizablePartNums().empty());

    }

    Y_UNIT_TEST(TestOptimizeUnderpopulated) {
        NRTYArchive::TOptimizationOptions opts;
        TPartOptimizationCheckVisitor visitor(opts);

        opts.SetPopulationRate(0.2);
        visitor.Start();
        RecordVisit(0, visitor, 10, 100, 10, UNLIMITED_SIZE);
        RecordVisit(1, visitor, 20, 100, 20, UNLIMITED_SIZE);
        RecordVisit(2, visitor, 10, 100, 20, UNLIMITED_SIZE);
        RecordVisit(3, visitor, 30, 100, 30, UNLIMITED_SIZE);
        UNIT_ASSERT(visitor.GetOptimizablePartNums().empty());

        opts.SetPopulationRate(0.8);
        visitor.Start();
        RecordVisit(0, visitor, 10, 100, 10, UNLIMITED_SIZE);
        RecordVisit(1, visitor, 20, 100, 20, UNLIMITED_SIZE);
        RecordVisit(2, visitor, 10, 100, 20, UNLIMITED_SIZE);
        RecordVisit(3, visitor, 30, 100, 30, UNLIMITED_SIZE);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({2}));
    }

    Y_UNIT_TEST(TestOptimizeOversized) {
        NRTYArchive::TOptimizationOptions opts;
        TPartOptimizationCheckVisitor visitor(opts);

        opts.SetPopulationRate(0.2);
        visitor.Start();
        RecordVisit(0, visitor, 10, 100, 10, 100);
        RecordVisit(1, visitor, 10, 100, 10, 10);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({1}));
    }

    Y_UNIT_TEST(TestOptimizeUndersizedDefault) {
        NRTYArchive::TOptimizationOptions opts;
        TPartOptimizationCheckVisitor visitor(opts);

        opts.SetPopulationRate(0.2);
        visitor.Start();
        RecordVisit(0, visitor, 10, 10, 10, 100);
        RecordVisit(1, visitor, 10, 100, 10, 100);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({0}));
    }

    Y_UNIT_TEST(TestOptimizeWithUndersizedLimit) {
        NRTYArchive::TOptimizationOptions opts;
        opts.SetMaxUndersizedPartsCount(3).SetPopulationRate(0.8);
        TPartOptimizationCheckVisitor visitor(opts);

        visitor.Start();
        RecordVisit(0, visitor, 10, 10, 10, 100);
        RecordVisit(1, visitor, 10, 100, 10, 100);
        RecordVisit(2, visitor, 10, 10, 10, 100);
        RecordVisit(3, visitor, 10, 10, 10, 100);
        UNIT_ASSERT(visitor.GetOptimizablePartNums().empty());
        RecordVisit(4, visitor, 10, 10, 10, 100);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({0,2,3,4}));

        visitor.Start();
        RecordVisit(0, visitor, 10, 10, 10, 100);
        RecordVisit(1, visitor, 10, 100, 10, 100);
        RecordVisit(2, visitor, 10, 10, 10, 100);
        RecordVisit(3, visitor, 10, 10, 10, 100);
        UNIT_ASSERT(visitor.GetOptimizablePartNums().empty());
        RecordVisit(4, visitor, 10, 200, 10, 100);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({0,2,3,4}));

        visitor.Start();
        RecordVisit(0, visitor, 10, 10, 10, 100);
        RecordVisit(1, visitor, 10, 100, 10, 100);
        RecordVisit(2, visitor, 10, 10, 10, 100);
        RecordVisit(3, visitor, 10, 10, 10, 100);
        UNIT_ASSERT(visitor.GetOptimizablePartNums().empty());
        RecordVisit(4, visitor, 3, 100, 10, 100);
        UNIT_ASSERT_VALUES_EQUAL(visitor.GetOptimizablePartNums(), TSet<ui32>({0,2,3,4}));
    }
}

Y_UNIT_TEST_SUITE_IMPL(ArchiveOptimizeSuite, TMultipartArchiveTestBase) {

    Y_UNIT_TEST(TestOptimizeEmptyArchive) {
        TMultipartConfig config;
        auto arc = TEngine::Create("test", config);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetPartsCount(), 1);
        arc->OptimizeParts(config.CreateOptimizationOptions());
        UNIT_ASSERT_VALUES_EQUAL(arc->GetPartsCount(), 1);
    }

    Y_UNIT_TEST(TestOptimizeWithNotFlushedPart) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(100, data, 30);
        auto archive = TArchiveConstructor::ConstructDirect(data);
        IArchivePart::TConstructContext ctx = archive->GetContext();
        archive.Drop();

        {
            TFATMultipart fat("test.fat", 0, TMemoryMapCommon::oRdWr);
            ui32 size = fat.Size();
            TPosition pos (3400, 0);
            fat.Set(size, pos);
            TPartMetaSaver(GetPartMetaPath("test", 0))->SetRemovedDocsCount(200);
            TPartMetaSaver(GetPartMetaPath("test", 0))->SetDocsCount(301);
        }

        TMultipartConfig config;
        config.ReadContextDataAccessType = ctx.DataAccessorContext.Type;
        config.WriteContextDataAccessType = ctx.WriteDAType;
        config.PartSizeLimit = ctx.SizeLimit;

        auto arc = TEngine::Create("test", config);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetPartsCount(), 2);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetDocsCount(true), 101);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetDocsCount(false), 101);
        INFO_LOG << arc->ReadDocument(101).Size() << Endl;

        arc->OptimizeParts(config.CreateOptimizationOptions());
        arc->Flush();
        UNIT_ASSERT_VALUES_EQUAL(arc->GetDocsCount(true), 101);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetDocsCount(false), 100);
        UNIT_ASSERT_VALUES_EQUAL(arc->GetPartsCount(), 2);
    }

    Y_UNIT_TEST(TestOptimizeWithEmptyPart) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(105, data, 30);
        auto archive = TArchiveConstructor::ConstructDirect(data, 10);
        IArchivePart::TConstructContext ctx = archive->GetContext();
        TMultipartConfig config;
        config.ReadContextDataAccessType = ctx.DataAccessorContext.Type;
        config.WriteContextDataAccessType = ctx.WriteDAType;
        config.PartSizeLimit = ctx.SizeLimit;
        archive.Drop();

        {
            TPartMetaSaver(GetPartMetaPath("test", 2))->SetRemovedDocsCount(9);
        }
        {
            TFATMultipart fat("test.fat", 0, TMemoryMapCommon::oRdWr);
            for (ui32 i = 22; i < 31; ++i) {
                TPosition pos = fat.Get(i);
                CHECK_WITH_LOG(pos.GetPart() == 2) << pos.GetPart();
                pos.SetRemoved();
                fat.Set(i, pos);
            }
        }

        archive = TEngine::Create("test", config, 0);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 11);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 96);

        TOptimizationOptions opt;
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 9);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 96);
    }

    void TestOptimizeDefault(IArchivePart::TType type, ui32 writableParts = 1) {
        TArchiveDataGen::TDataSet data;
        NRTYArchiveTest::TArchiveDataGen generator(30, false);
        generator.Generate(105, data);

        TEngine::TPtr archive;
        if (type == IArchivePart::RAW) {
            archive = TArchiveConstructor::ConstructDirect(data, 10);
        } else {
            archive = TArchiveConstructor::ConstructCompressedDirect(data, 10);
        }

        for (ui32 docId = 12; docId < 20; ++docId) {
            archive->RemoveDocument(docId);
        }
        archive->WaitAllTasks();
        INFO_LOG << "get info " << Endl;
        TFsPath arcPath = archive->GetPath();

        ui64 partSizeLimit = archive->GetContext().SizeLimit;

        ui32 partsToOptimize = (type == IArchivePart::RAW) ? 2 : 1;
        archive.Drop();
        TMultipartConfig config;
        config.Compression = type;
        config.WritableThreadsCount = writableParts;
        config.PartSizeLimit = partSizeLimit;

        archive = TArchiveOwner::Create(arcPath, config);

        {
            TArchiveInfo info;
            archive->GetInfo(info);
            NJson::TJsonValue json;
            info.ToJson(json);
            DEBUG_LOG << json.GetStringRobust() << Endl;
            UNIT_ASSERT_VALUES_EQUAL(info.PartsCount, 10);
            UNIT_ASSERT_VALUES_EQUAL(info.AliveDocsCount, 97);
            UNIT_ASSERT_VALUES_EQUAL(info.RemovedDocsCount, 8);
            UNIT_ASSERT_VALUES_EQUAL(info.Optimizations["0.2-0.6"].PartsToOptimize, partsToOptimize);
            UNIT_ASSERT_VALUES_EQUAL(info.Optimizations["0.2-0.6"].DocsToMove, type == IArchivePart::RAW ? 9 : 3);
        }

        TOptimizationOptions opt;
        archive->OptimizeParts(opt);

        archive->WaitAllTasks();

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 10 - partsToOptimize + writableParts);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 97);
        {
            TArchiveInfo info;
            archive->GetInfo(info);
            NJson::TJsonValue json;
            info.ToJson(json);
            DEBUG_LOG << json.GetStringRobust() << Endl;
            UNIT_ASSERT_VALUES_EQUAL(info.Optimizations["0.2-0.6"].PartsToOptimize, 0);
            UNIT_ASSERT_VALUES_EQUAL(info.PartsCount, 11 - partsToOptimize);
            UNIT_ASSERT_VALUES_EQUAL(info.AliveDocsCount, 97);
            UNIT_ASSERT_VALUES_EQUAL(info.RemovedDocsCount, 0);
        }

    }

    Y_UNIT_TEST(TestOptimizeDefaultRaw) {
        TestOptimizeDefault(IArchivePart::RAW);
    }

    Y_UNIT_TEST(TestOptimizeDefaultCompressed) {
        TestOptimizeDefault(IArchivePart::COMPRESSED);
    }

    Y_UNIT_TEST(TestOptimizeDefaultRaw3Parts) {
        TestOptimizeDefault(IArchivePart::RAW, 3);
    }

    Y_UNIT_TEST(TestOptimizeDefaultCompressed3Parts) {
        TestOptimizeDefault(IArchivePart::COMPRESSED, 3);
    }


    Y_UNIT_TEST(TestNoOptimizations) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(105, data, 30);
        auto archive = TArchiveConstructor::ConstructDirect(data, 10);

        TOptimizationOptions opt;
        opt.SetPopulationRate(0.4);
        opt.SetPartSizeDeviation(0.6);
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 11);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 105);
    }

    Y_UNIT_TEST(TestOptimizeSeveralParts) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(105, data, 30);
        auto archive = TArchiveConstructor::ConstructDirect(data, 10);

        for (ui32 docId = 12; docId < 18; ++docId) {
            archive->RemoveDocument(docId);
        }

        for (ui32 docId = 32; docId < 38; ++docId) {
            archive->RemoveDocument(docId);
        }

        for (ui32 docId = 52; docId < 58; ++docId) {
            archive->RemoveDocument(docId);
        }

        TOptimizationOptions opt;
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 9);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 87);
    }

    Y_UNIT_TEST(TestOptimizeWhenSizeChanged) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.PartSizeLimit = 13 * (2 + sizeof(ui32));

        auto archive = TEngine::Create(arcPath, config, 105);

        // part.1 0-14 / part2 15 - 28 ...
        for (ui32 docId = 0; docId < 105; ++docId) {
            archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
        }

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 8);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 105);

        archive.Drop();

        TMultipartConfig configNew;
        configNew.PartSizeLimit = 10 * (2 + sizeof(ui32));


        archive = TEngine::Create(arcPath, configNew);

        TOptimizationOptions opt;
        //       opt.SetUseBlocks(true);
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 11);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 105);

        archive.Drop();

        archive = TEngine::Create(arcPath, config);
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 8);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 105);
    }

    Y_UNIT_TEST(TestOptimizeWithMaxPartLimit) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(105, data, 30);
        auto archive = TArchiveConstructor::ConstructDirect(data, 10);
        IArchivePart::TConstructContext ctx = archive->GetContext();
        TMultipartConfig config;
        config.ReadContextDataAccessType = ctx.DataAccessorContext.Type;
        config.WriteContextDataAccessType = ctx.WriteDAType;
        config.PartSizeLimit = ctx.SizeLimit;
        archive.Drop();

        archive = TEngine::Create("test", config, 0);

        TOptimizationOptions opt;
        archive->OptimizeParts(opt);

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 10);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 105);
    }
}
