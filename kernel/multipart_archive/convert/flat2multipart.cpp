#include <kernel/multipart_archive/multipart.h>
#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/merge/builder.h>

#include <util/system/file.h>
#include <util/system/filemap.h>
#include <util/generic/yexception.h>
#include <util/thread/pool.h>


namespace NRTYArchive {

    class TRepairerTask : public IObjectInQueue {
    public:
        TRepairerTask(TArchiveOwner::TPtr result,
                const TFileMappedArray<ui64>& indexDir,
                const void* indexArc,
                ui32 beginDoc,
                ui32 endDoc)
            : Result(result)
            , IndexDir(indexDir)
            , IndexArc(indexArc)
            , BeginDoc(beginDoc)
            , EndDoc(endDoc)
        {}

        virtual void Process(void* /*threadSpecificResource*/) override {
            for (size_t docId = BeginDoc; docId < EndDoc; ++docId) {
                ui32 count = docId - BeginDoc;
                if ((docId - BeginDoc) % 10000 == 0) {
                    INFO_LOG << "Process " << count << " from " << BeginDoc << Endl;
                }
                if (IndexDir[docId] == Max<ui64>()) {
                    continue;
                }

                const void* docPtr = (const char*) IndexArc + IndexDir[docId];
                const TArchiveHeader* info = static_cast<const TArchiveHeader*>(docPtr);
                CHECK_WITH_LOG(docId == info->DocId) << docId << "/" << info->DocId;

                TBlob document = TBlob::NoCopy(docPtr, info->DocLen);
                Result->PutDocument(document, docId);
            }
        }

    private:
        TArchiveOwner::TPtr Result;
        const TFileMappedArray<ui64>& IndexDir;
        const void* IndexArc;
        ui32 BeginDoc = 0;
        ui32 EndDoc = 0;
    };

    void ConvertFlatArchiveToMultipart(const TFsPath& fatFile, const TFsPath& dataFile,
                                       const TFsPath& resultMultipart, const TMultipartConfig& config,
                                       bool inParallel)
    {
        if (!fatFile.Exists())
            ythrow yexception() << "there is no " << fatFile.GetPath();
        if (!dataFile.Exists())
            ythrow yexception() << "there is no " << dataFile.GetPath();
        if (TArchiveOwner::Exists(resultMultipart)) {
            ythrow yexception() << "multipart already exists " << resultMultipart.GetPath();
        }

        TFileMappedArray<ui64> indexDir;
        indexDir.Init(fatFile.GetPath());
        size_t docsCount = indexDir.Size();

        TArchiveOwner::TPtr newArc = TArchiveOwner::Create(resultMultipart, config, docsCount);
        TFileMap indexArc(dataFile.GetPath());
        indexArc.Map(0, indexArc.GetFile().GetLength());

        INFO_LOG << "ConvertFlatArchiveToMultipart for " << fatFile << "(" << docsCount << ")..." << Endl;

        ui32 threadsCount = 4;
        TThreadPool tasks;
        tasks.Start(threadsCount);

        ui32 docsPerTask = docsCount / threadsCount;
        ui32 start = 0;

        if (inParallel) {
            for (ui32 i = 0; i < threadsCount - 1; ++i) {
                CHECK_WITH_LOG(tasks.AddAndOwn(MakeHolder<TRepairerTask>(newArc, indexDir, indexArc.Ptr(), start, start + docsPerTask)));
                start += docsPerTask;
            }
        }
        CHECK_WITH_LOG(tasks.AddAndOwn(MakeHolder<TRepairerTask>(newArc, indexDir, indexArc.Ptr(), start, docsCount)));
        tasks.Stop();

        INFO_LOG << "ConvertFlatArchiveToMultipart for " << fatFile << "... OK" << Endl;
    }

    void ConvertMultipartArchiveToFlat(const TFsPath& multipartArchive, const TMultipartConfig& config, const TFsPath& resultFlat) {
        TArchiveOwner::TPtr oldArchive = TArchiveOwner::Create(multipartArchive, config);

        ITextArchiveBuilder::TConstructContext newArcCtx;
        newArcCtx.ThreadsCount = 1;
        newArcCtx.DocsCount = oldArchive->GetDocsCount(false);
        newArcCtx.ArchiveVersion = ARCVERSION;
        newArcCtx.DirConfig.NewPrefix = resultFlat;

        TAtomicSharedPtr<ITextArchiveBuilder> builder = ITextArchiveBuilder::TFactory::Construct(AT_FLAT, newArcCtx);

        INFO_LOG << "ConvertMultipartArchiveToFlat for " << multipartArchive << "..." << Endl;

        auto arcIter = oldArchive->CreateIterator();
        builder->Start();
        while (arcIter->IsValid()) {
            builder->IndexDoc(arcIter->GetDocid(), arcIter->GetDocument(), 0);
            arcIter->Next();
        }

        builder->Close(nullptr);

        INFO_LOG << "ConvertMultipartArchiveToFlat for " << multipartArchive << "... OK" << Endl;
    }
}
