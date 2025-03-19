#include "part_thread_safe.h"
#include "part_header.h"

#include <kernel/multipart_archive/abstract/globals.h>

#include <library/cpp/logger/global/global.h>

#include <util/stream/file.h>
#include <util/system/fs.h>


namespace NRTYArchive {

    namespace {

        IArchivePart::IPolicy* CreatePolicy(const IArchivePart::TConstructContext& ctx, bool writable) {
            if (writable) {
                return new TPartPolicy<true>(ctx);
            } else {
                return new TPartPolicy<false>(ctx);
            }
        }
    }

    TArchivePartThreadSafe::TArchivePartThreadSafe(const TFsPath& path, ui32 index, const IArchiveManager* manager, bool writable, IPartCallBack& owner)
        : IClosable(!writable)
        , Owner(owner)
        , Manager(manager)
        , Path(path)
        , PartIndex(index)
        , Dropped(false)
    {
        CHECK_WITH_LOG(Manager);

        IArchivePart::TConstructContext ctx = Manager->GetContext();
        EOpenMode headerOpenMode = writable ? CreateAlways : RdWr;
        if (Manager->IsReadOnly()) {
            headerOpenMode = RdOnly;
        }

        auto ms = Manager->CreateMetaSaver(GetPartMetaPath(Path, PartIndex));
        if (ms) {
            if (!Manager->IsReadOnly()) {
                if (!writable) {
                    CHECK_WITH_LOG(ms->GetProto().GetStage() == TPartMetaInfo::CLOSED) << Path << "." << PartIndex;
                }
                (*ms)->SetStage(writable ? TPartMetaInfo::BUILD : TPartMetaInfo::OPENED);
            }
            AtomicSet(DocsCount, ms->GetProto().GetDocsCount());
            AtomicSet(RemovedDocsCount, ms->GetProto().GetRemovedDocsCount());
            if (!TryFromString(ms->GetProto().GetCompression(), ctx.Type)) {
                DEBUG_LOG << "Can't read compression from metafile in part " << GetPartPath(path, index) << " , using context information : " << ctx.Type << Endl;
            } else {
                DEBUG_LOG << "Read compression from metafile in part " << GetPartPath(Path, PartIndex) << " : " << ctx.Type << Endl;
            }
        }
        Policy.Reset(CreatePolicy(ctx, writable));

        Slave.Reset(IArchivePart::TFactory::Construct(ctx.Type, (PartIndex == Max<ui32>() ? Path : GetPartPath(Path, PartIndex)), *Policy));
        PartHeader.Reset(Manager->CreateHeader(GetPartHeaderPath(Path, PartIndex), headerOpenMode));
        CHECK_WITH_LOG(PartHeader);
        CHECK_WITH_LOG(Slave);
        AtomicSet(SizeInBytes, Slave->GetSizeInBytes());
    }

    TArchivePartThreadSafe::~TArchivePartThreadSafe() {
        DEBUG_LOG << "Destroy part " << GetPartPath(Path, PartIndex) << Endl;
        Slave.Destroy();
        PartHeader.Destroy();
        auto ms = Manager->CreateMetaSaver(GetPartMetaPath(Path, PartIndex));
        if (ms && !Manager->IsReadOnly()) {
            (*ms)->SetStage(TPartMetaInfo::CLOSED);
            (*ms)->SetDocsCount(AtomicGet(DocsCount));
            (*ms)->SetRemovedDocsCount(AtomicGet(RemovedDocsCount));
            (*ms)->SetCompression(ToString(Policy->GetContext().Type));
        }
        ms.Drop();

        if (Dropped) {
            TArchivePartThreadSafe::Remove(Path, PartIndex);
        }
    }

    bool TArchivePartThreadSafe::AllRight(const TFsPath& path, ui32 index) {
        const TPartMetaSaver meta(GetPartMetaPath(path, index), true, /*allowUnknownField*/ true);
        return meta->GetStage() == TPartMetaInfo::CLOSED;
    }

    void TArchivePartThreadSafe::Remove(const TFsPath& path, ui32 index) {
        GetPartPath(path, index).ForceDelete();
        GetPartHeaderPath(path, index).ForceDelete();
        GetPartMetaPath(path, index).ForceDelete();
    }


    void TArchivePartThreadSafe::Rename(const TFsPath& from, const TFsPath& to, ui32 index) {
        GetPartPath(from, index).RenameTo(GetPartPath(to, index));
        GetPartHeaderPath(from, index).RenameTo(GetPartHeaderPath(to, index));
        GetPartMetaPath(from, index).RenameTo(GetPartMetaPath(to, index));
    }

    bool TArchivePartThreadSafe::Repair(const TFsPath& path, ui32 index, IArchivePart::TConstructContext ctx, IRepairer* repairer) {
        VERIFY_WITH_LOG(!!repairer, "there is no repairer");
        TFsPath headerPath(GetPartHeaderPath(path, index));
        TPartMetaSaver ms(GetPartMetaPath(path, index));
        INFO_LOG << "Fix broken part " << headerPath << " ..." << Endl;

        if (!TryFromString(ms->GetCompression(), ctx.Type)) {
            DEBUG_LOG << "Can't read compression from metafile in part " << GetPartPath(path, index) << " , using context information " << ctx.Type << Endl;
        } else {
            DEBUG_LOG << "Read compression from metafile in part " << GetPartPath(path, index) << " : " << ctx.Type << Endl;
        }

        auto partPolicy = MakeHolder<TPartPolicy<false>>(ctx);

        IArchivePart::TPtr slave = IArchivePart::TFactory::Construct(ctx.Type, GetPartPath(path, index), *partPolicy);
        IArchivePart::IIterator::TPtr iter = slave->CreateIterator();

        ui64 docNum = 0;
        ui64 docOffset = 0;
        iter->SkipTo(docOffset);

        while (true) {
            if (iter->GetDocument().Empty())
                break;

            repairer->OnIterNext(docNum, docOffset);
            docNum++;
            docOffset = iter->SkipNext();
        }

        if (repairer->GetFullDocsCount() == 0) {
            INFO_LOG << "Fix broken part " << headerPath << " ...FAILS" << Endl;
            return false;
        }

        if (repairer->GetRepairedDocsCount() == 0) {
            INFO_LOG << "Fix broken part: no documents found " << headerPath << " ...FAILS" << Endl;
            return false;
        }

        repairer->Flush(headerPath);
        CHECK_WITH_LOG(repairer->GetFullDocsCount() >= repairer->GetRepairedDocsCount());

        ms->SetDocsCount(repairer->GetFullDocsCount());
        ms->SetRemovedDocsCount(repairer->GetFullDocsCount() - repairer->GetRepairedDocsCount());
        ms->SetStage(TPartMetaInfo::CLOSED);
        ms->SetCompression(ToString(ctx.Type));

        INFO_LOG << "Fix broken part " << headerPath << " ...OK (repaired:" << repairer->GetRepairedDocsCount()
            << ";found:" << repairer->GetFullDocsCount()
            << ";lost:" << repairer->GetLost() << ")" << Endl;
        return true;
    }

    TBlob TArchivePartThreadSafe::GetDocument(IArchivePart::TOffset offset) const {
        TReadGuard g(DataMutex);
        return Slave->GetDocument(offset);
    }

    TArchivePartThreadSafe::TPtr TArchivePartThreadSafe::HardLinkOrCopyTo(const TFsPath& path, ui32 index, const IArchiveManager* manager, ui32 newDocsCount, IPartCallBack& owner) const {
        TReadGuard g(DataMutex);
        DEBUG_LOG << "Make hardlink for " << GetPartPath(Path, PartIndex) << Endl;
        VERIFY_WITH_LOG(IsClosed(), "Can't copy not closed writable part");
        CHECK_WITH_LOG(!Dropped);

        ui64 docsCount = AtomicGet(DocsCount);
        ui64 removedDocsCount = AtomicGet(RemovedDocsCount);

        CHECK_WITH_LOG(newDocsCount <= docsCount) << newDocsCount << "/" << docsCount << "/" << removedDocsCount;

        CHECK_WITH_LOG(Slave->HardLinkOrCopyTo(GetPartPath(path, index))) << GetPartPath(Path, PartIndex);
        NFs::Copy(GetPartHeaderPath(Path, PartIndex), GetPartHeaderPath(path, index));

        {
            TPartMetaSaver ms(GetPartMetaPath(path, index));
            ms->SetStage(TPartMetaInfo::CLOSED);
            ms->SetDocsCount(docsCount);
            ms->SetRemovedDocsCount(docsCount - newDocsCount);
            ms->SetCompression(ToString(Policy->GetContext().Type));
        }

        return new TArchivePartThreadSafe(path, index, manager, false, owner);
    }

    void TArchivePartThreadSafe::UpdateHeader(ui64 position, ui32 docid) {
        TWriteGuard g(DataMutex);
        PartHeader->operator[](position) = docid;
    }

    ui32 TArchivePartThreadSafe::GetHeaderData(ui64 position) const {
        TReadGuard g(DataMutex);
        return PartHeader->operator[](position);
    }

    IArchivePart::IIterator::TPtr TArchivePartThreadSafe::CreateSlaveIterator() {
        CHECK_WITH_LOG(Slave);
        return Slave->CreateIterator();
    }

    void TArchivePartThreadSafe::Drop() {
        INFO_LOG << "Dropping part " << GetPath() << Endl;
        TWriteGuard g(DataMutex);
        Slave->Drop();
        Dropped = true;
    }

    IArchivePart::TOffset TArchivePartThreadSafe::TryPutDocument(const TBlob& document, ui32 docid, ui32* idx) {
        TWriteGuard g(DataMutex);
        VERIFY_WITH_LOG(!IsClosed(), "put document to readonly part %s", Slave->GetPath().GetPath().data());

        IArchivePart::TOffset res = Slave->TryPutDocument(document);
        if (res == TPosition::InvalidOffset) {
            return res;
        }
        ui32 idxNewDoc = PartHeader->PushDocument(docid);
        if (idx) {
            *idx = idxNewDoc;
        }
        AtomicIncrement(DocsCount);
        AtomicSet(SizeInBytes, Slave->GetSizeInBytes());

        if (!IsFullNotified && Slave->IsFull()) {
            IsFullNotified = true;
            Owner.OnPartFull(PartIndex);
        }
        return res;
    }

    bool TArchivePartThreadSafe::RemoveDocument() {
        TWriteGuard g(DataMutex);
        if (DocsCount - RemovedDocsCount == 0)
            return false;

        AtomicIncrement(RemovedDocsCount);
        if (IsClosed() && DocsCount == RemovedDocsCount) {
            Owner.OnPartFree(PartIndex);
        }
        return true;
    }

    bool TArchivePartThreadSafe::IsFull() const {
        TReadGuard g(DataMutex);
        VERIFY_WITH_LOG(!IsClosed(), "check IsFull in readonly part %s", Slave->GetPath().GetPath().data());
        return Slave->IsFull();
    }

    ui64 TArchivePartThreadSafe::GetSizeInBytes() const {
        return AtomicGet(SizeInBytes);
    }

    ui64 TArchivePartThreadSafe::GetPartSizeLimit() const {
        return Manager->GetContext().SizeLimit;
    }

    ui64 TArchivePartThreadSafe::GetDocsCount(bool withRemoved) const {
        return withRemoved ? AtomicGet(DocsCount) : AtomicGet(DocsCount) - AtomicGet(RemovedDocsCount);
    }

    void TArchivePartThreadSafe::DoClose() {
        TWriteGuard g(DataMutex);
        ui64 docsCount = AtomicGet(DocsCount);
        ui64 removedDocsCount = AtomicGet(RemovedDocsCount);
        Owner.OnPartClose(PartIndex, docsCount - removedDocsCount);
    }

    void TArchivePartThreadSafe::CloseImpl() {
        TWriteGuard g(DataMutex);
        ui64 docsCount = AtomicGet(DocsCount);
        ui64 removedDocsCount = AtomicGet(RemovedDocsCount);

        CHECK_WITH_LOG(!Manager->IsReadOnly());

        if (Dropped) {
            return;
        }

        if (docsCount == removedDocsCount) {
            Owner.OnPartFree(PartIndex);
            return;
        }

        Slave->Close();
        try {
            PartHeader->Close();
            auto ms = Manager->CreateMetaSaver(GetPartMetaPath(Path, PartIndex));
            if (ms) {
                (*ms)->SetStage(TPartMetaInfo::OPENED);
                (*ms)->SetDocsCount(docsCount);
                (*ms)->SetRemovedDocsCount(removedDocsCount);
                (*ms)->SetCompression(ToString(Policy->GetContext().Type));
            }
        } catch (...) {
            ERROR_LOG << CurrentExceptionMessage() << Endl;
        }
        AtomicSet(SizeInBytes, Slave->GetSizeInBytes());
    }

    TFsPath TArchivePartThreadSafe::GetPath() const {
        return GetPartPath(Path, PartIndex);
    }

    ui32 TArchivePartThreadSafe::GetPartNum() const {
        return PartIndex;
    }

    ui32 TArchivePartThreadSafe::ParsePartFromFileName(const TFsPath& file, const TFsPath& archivePrefix) {
        ui32 index = 0;
        TString partPrefix = archivePrefix.GetName() + PART_EXT;
        if (file.GetName().StartsWith(partPrefix) && TryFromString(file.GetName().substr(partPrefix.size()), index)) {
            if (file.GetExtension() != PART_HEADER_EXT)
                return index;
        }
        return Max<ui32>();
    }
}
