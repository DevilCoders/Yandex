#pragma once

#include "interfaces.h"
#include "archive_manager.h"
#include "global.h"
#include "part_thread_safe.h"
#include "part_iterator.h"

#include <kernel/multipart_archive/abstract/archive.h>
#include <kernel/multipart_archive/abstract/position.h>

#include <library/cpp/threading/named_lock/named_lock.h>

#include <util/generic/algorithm.h>

namespace NRTYArchive {
    constexpr ui32 PUT_DOCUMENT_RETRIES = 1000;

    class TIteratorFactory {
    public:
        virtual typename IPartIterator::TPtr CreatePartIterator(TArchivePartThreadSafe::TPtr /*part*/, IFat* /*fat*/) const = 0;
        virtual ~TIteratorFactory() = default;
    };

    template<class TDocId>
    class TMultipartReadOnlyImpl {
    public:
        using TPartPtr = TArchivePartThreadSafe::TPtr;
        using TParts = TMap<ui32, TPartPtr>;

    public:
        TMultipartReadOnlyImpl(IFat* fat, const TFsPath& path, IArchiveManager* manager, NNamedLock::TNamedLockPtr archiveLock)
            : Manager(manager)
            , FAT(fat)
            , Path(path)
            , GlobalArchiveLock(archiveLock)
        {
            Path.Fix();
        }

        virtual void InitParts() {
            CHECK_WITH_LOG(Parts.empty());

            TVector<ui32> partIndexes;
            FreePartIndex = FillPartsIndexes(Path, partIndexes);
            for (ui32 i = 0; i < partIndexes.size(); ++i) {
                ui32 index = partIndexes[i];
                TArchivePartThreadSafe::TPtr part = new TArchivePartThreadSafe(Path, index, Manager.Get(), false, GetPartCallback());
                VERIFY_WITH_LOG(Parts.insert(std::make_pair(index, part)).second, "duplicate part %s", part->GetPath().c_str());
            }
        }

    public:
        TBlob DoReadDocument(TDocId docid) const {
            TPosition pos = FAT->Get(docid);

            if (pos.IsRemoved()) {
                return TBlob();
            }

            TPartPtr part = GetPartByNum(pos.GetPart());
            while(!part) {
                pos = FAT->Get(docid);
                if (pos.IsRemoved()) {
                    return TBlob();
                }

                part = GetPartByNum(pos.GetPart());
            }

            return part->GetDocument(pos.GetOffset());
        }

        const TFsPath& GetPath() const {
            return Path;
        }

        size_t GetPartsCount() const {
            return Parts.size();
        }

        size_t GetAllDocsCount() const {
            return FAT->Size();
        }

    private:
        class TPartCallBackStub: public IPartCallBack {
        public:
            void OnPartFree(ui64 /*partNum*/) override {}
            void OnPartFull(ui64 /*partNum*/) override {}
            void OnPartDrop(ui64 /*partNum*/) override {}
            void OnPartClose(ui64 /*partNum*/, ui64 /*docsCount*/) override {}
        };

    protected:
        static ui32 FillPartsIndexes(const TFsPath& path, TVector<ui32>& partIndexes) {
            TVector<TFsPath> files;
            path.Parent().List(files);
            ui32 freePartIndex = 0;

            for (const auto& file : files) {
                ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(file, path);
                if (index != Max<ui32>()) {
                    partIndexes.push_back(index);
                    freePartIndex = Max(freePartIndex, index + 1);
                }
            }

            Sort(partIndexes.begin(), partIndexes.end());
            return freePartIndex;
        }

    protected:
        virtual IPartCallBack& GetPartCallback() {
            return CbStub;
        }

        virtual TArchivePartThreadSafe::TPtr GetPartByNum(ui64 partNum) const {
            auto it = Parts.find(partNum);
            return it != Parts.end() ? it->second : nullptr;
        }

    protected:
        THolder<IArchiveManager> Manager;
        TParts Parts;
        THolder<IFat> FAT;
        TFsPath Path;
        ui32 FreePartIndex = 0;
        TPartCallBackStub CbStub;
        NNamedLock::TNamedLockPtr GlobalArchiveLock;
    };


    template<class TDocId>
    class TMultipartImpl : protected TMultipartReadOnlyImpl<TDocId>, public IPartCallBack, public TLinksCounter {
        using TBase = TMultipartReadOnlyImpl<TDocId>;
        using TSelf = TMultipartImpl<TDocId>;

    public:
        using TDocInfo = IPartIterator::TDocInfo;

        template<class T>
        class IDocSerializer {
        public:
            virtual ~IDocSerializer() {}
            virtual TBlob Serialize(const T& doc) const = 0;
        };

        class TIterator {
        public:
            using TPtr = TAtomicSharedPtr<TIterator>;

            TIterator(const TSelf& archive, IFat* fat, TIteratorFactory* factory)
                : Archive(archive)
                , PartIterFactory(factory)
                , Fat(fat)
            {
                Archive.GetPartsIndexes(PartsIndexes);
                CurrentPart = PartsIndexes.begin();
                Next();
            }

            TBlob GetDocument() const {
                return PartIterator->GetDocument();
            }

            TDocId GetDocid() const {
                return PartIterator->GetDocId();
            }

            TDocInfo GetDocumentInfo() const {
                CHECK_WITH_LOG(IsValid());
                return PartIterator->GetDocumentInfo();
            }

            void Next() {
                if (PartIterator && PartIterator->IsValid())
                    PartIterator->Next();

                if (PartIterator && PartIterator->IsValid())
                    return;

                while (CurrentPart != PartsIndexes.end()) {
                    TArchivePartThreadSafe::TPtr part = Archive.GetPartByNum(*CurrentPart);
                    if (!!part) {
                        PartIterator = PartIterFactory->CreatePartIterator(part, Fat);
                        if (PartIterator->IsValid()) {
                            CurrentPart++;
                            return;
                        }
                    }
                    CurrentPart++;
                }

                if (PartIterator && !PartIterator->IsValid()) {
                    PartIterator.Drop();
                }
            }

            bool IsValid() const {
                return PartIterator && PartIterator->IsValid();
            }

        private:
            const TSelf& Archive;
            THolder<TIteratorFactory> PartIterFactory;
            IPartIterator::TPtr PartIterator;
            TSet<ui32> PartsIndexes;
            TSet<ui32>::iterator CurrentPart;
            IFat* Fat;
        };

    public:
        TMultipartImpl(const TFsPath& path, IFat* fat, IArchiveManager* manager, NNamedLock::TNamedLockPtr archiveLock)
            : TBase(fat, path, manager, archiveLock)
            , WritablePartsCount(manager->GetWritableThreadsCount())
            , PartsToWrite(WritablePartsCount, nullptr)
        {
        }

        virtual ~TMultipartImpl() {
            PartsToWrite.clear();
            DEBUG_LOG << "WaitAllTasks for " << TBase::Path << Endl;
            WaitAllTasks();
        }

        typename TIterator::TPtr CreateCommonIterator(TIteratorFactory* factory) const {
            return new TIterator(*this, TBase::FAT.Get(), factory);
        }

        const IArchivePart::TConstructContext& GetContext() const {
            return TBase::Manager->GetContext();
        }

        virtual TBlob ReadDocument(TDocId docid) const final {
            return TBase::DoReadDocument(docid);
        }

        virtual bool IsRemoved(TDocId docid) const {
            const TPosition pos = TBase::FAT->Get(docid);
            return pos.IsRemoved();
        }

        const TFsPath& GetPath() const {
            return TBase::Path;
        }

        ui32 GetDocsCount(bool withDeleted) const {
            if (withDeleted)
                return TBase::GetAllDocsCount();
            return GetLifeDocsCountUnsafe();
        }

        ui64 GetSizeInBytes() const {
            TReadGuard g(Lock);
            ui64 result = 0;
            for (const auto& p : TBase::Parts)
                result += p.second->GetSizeInBytes();
            return result;
        }

        size_t GetPartsCount() const {
            TReadGuard g(Lock);
            return TBase::GetPartsCount();
        }

        bool IsWritable() const {
            return !TBase::Manager->IsReadOnly();
        }

        bool RemoveDocument(TDocId docid) {
            CHECK_WITH_LOG(IsWritable());
            TPosition pos = TBase::FAT->Set(docid, TPosition::Removed());
            if (pos.IsRemoved())
                return false;

            RemoveDocumentImpl(pos);
            return true;
        }

        void RemoveDocumentImpl(const TPosition& pos) {
            RemoveDocumentFromPart(pos.GetPart(), [](auto&&){});
        }

        TPosition PutDocument(const TBlob& document, TDocId docid, ui32 partSelector = 0, ui32* idx = nullptr) {
            if (document.Empty())
                return TPosition::Removed();

            TWritablePartPtr writablePart;
            IArchivePart::TOffset partOffset = TPosition::InvalidOffset;
            size_t retryCount = 0;
            do {
                writablePart = GetWritablePart(partSelector);
                partOffset = writablePart->TryPutDocument(document, docid, idx);
            } while (partOffset == TPosition::InvalidOffset && ++retryCount < PUT_DOCUMENT_RETRIES);
            CHECK_WITH_LOG(retryCount < PUT_DOCUMENT_RETRIES) << "failed to write document, docid = " << docid;

            TPosition newPos(partOffset, writablePart->GetPartNum());
            TPosition oldPos = TBase::FAT->Set(docid, newPos);

            if (!oldPos.IsRemoved()) {
                RemoveDocumentImpl(oldPos);
            }
            return newPos;
        }

        template<class T>
        void PutDocument(const T& document, TDocId docid, const IDocSerializer<T>& serializer) {
            TBlob blob = serializer.Serialize(document);
            if (blob.Length())
                PutDocument(blob, docid);
        }

        void Flush() {
            TVector<TWritablePartPtr> guards;
            {
                TWriteGuard g(Lock);
                guards = ResetWritableParts();
                DoFlush();
            }
        }

        virtual void DoFlush() {}
        virtual void DoClear() {}

        // External lock required
        ui32 Clear(ui32 newFatSize) {
            for (auto& partToWrite : PartsToWrite) {
                partToWrite.Drop();
            }

            ui32 removedDocsCount = 0;
            for (auto& part : TBase::Parts) {
                removedDocsCount += part.second->GetDocsCount();
                part.second->Drop();
            }

            TBase::Parts.clear();
            TBase::FAT->Clear(newFatSize);
            DoClear();
            ResetWritableParts();
            return removedDocsCount;
        }

    public:
        static ui32 FillPartsIndexes(const TFsPath& path, TVector<ui32>& partIndexes) {
            return TBase::FillPartsIndexes(path, partIndexes);
        }

    private:
        void GetPartsIndexes(TSet<ui32>& result) const {
            TReadGuard g(Lock);
            for (auto&& part : TBase::Parts) {
                result.insert(part.first);
            }
        }

        class TRemovePartTask : public ILinkedTask {
        private:
            TArchivePartThreadSafe::TPtr Part;

        public:
            TRemovePartTask(TArchivePartThreadSafe::TPtr part, TLinksCounter& owner)
                : ILinkedTask(owner)
                , Part(part)
            {}

            virtual void DoProcess(void* /*threadSpecificResource*/) override {
                DEBUG_LOG << "Remove part in task " << Part->GetPath() << Endl;
                Part->Drop();
                Part.Drop();
            }
        };

        class TClosePartTask: public ILinkedTask {
        private:
            TArchivePartThreadSafe::TPtr Part;

        public:
            TClosePartTask(TArchivePartThreadSafe::TPtr part, TLinksCounter& owner)
                : ILinkedTask(owner)
                , Part(part) {}

            virtual void DoProcess(void* /*threadSpecificResource*/) override {
                DEBUG_LOG << "Close part in task " << Part->GetPath() << Endl;
                Part->CloseImpl();
                Part.Drop();
            }
        };

    protected:
        TBlob GetDocumentByPosition(const TPosition& pos) const {
            typename TBase::TParts::const_iterator partIt = TBase::Parts.find(pos.GetPart());
            VERIFY_WITH_LOG(partIt != TBase::Parts.end(), "broken archive structure in %s : unknown part %u", TBase::Path.GetPath().data(), pos.GetPart());
            return partIt->second->GetDocument(pos.GetOffset());
        }

        virtual void InitParts() override {
            TBase::InitParts();

            if (IsWritable()) {
                ResetWritableParts();
            }
        }

        void InitCommon() {
            InitParts();
            if (!IsWritable()) {
                return;
            }

            TSet<ui64> newParts;
            for (auto part : PartsToWrite) {
                newParts.insert(part->GetPartNum());
            }

            for (auto fatIter = TBase::FAT->GetIterator(); fatIter->IsValid(); fatIter->Next()) {
                TPosition pos = fatIter->GetPosition();
                if (!pos.IsRemoved()) {
                    typename TBase::TParts::iterator partIterator = TBase::Parts.find(pos.GetPart());
                    if (partIterator == TBase::Parts.end() || newParts.contains(partIterator->first)) {
                        ERROR_LOG << "Unknown part " << pos.GetPart() << Endl;
                        TBase::FAT->Set(fatIter->GetId(), TPosition::Removed());
                    }
                }
            }
        }

        template <class TActor>
        bool RemoveDocumentFromPart(ui32 part, const TActor& onDocRemove) {
            TArchivePartThreadSafe::TPtr partPtr = GetPartByNum(part);
            if (!partPtr) {
                return false;
            }

            onDocRemove(partPtr);
            partPtr->RemoveDocument();
            return true;
        }

        ui32 GetLifeDocsCountUnsafe() const {
            ui32 result = 0;
            TReadGuard g(Lock);
            for (const auto& p : TBase::Parts)
                result += p.second->GetDocsCount();
            return result;
        }

    protected:
        class TPartCloser {
        public:
            template<class T>
            static inline void Destroy(T* part) noexcept {
                part->Close();
            }
        };

        using TWritablePartPtr = TAtomicSharedPtr<TArchivePartThreadSafe, TPartCloser>;

        TWritablePartPtr GetWritablePart(ui32 partSelector = 0) {
            ui32 opId = partSelector ? partSelector : AtomicIncrement(OperationId);
            TReadGuard g(Lock);
            auto partToWrite = PartsToWrite[opId % WritablePartsCount];
            CHECK_WITH_LOG(!!partToWrite);
            return partToWrite;
        }

        IPartCallBack& GetPartCallback() override {
            return *this;
        }

        TArchivePartThreadSafe::TPtr GetPartByNum(ui64 partNum) const override {
            TReadGuard g(Lock);
            return TBase::GetPartByNum(partNum);
        }

        TVector<TWritablePartPtr> ResetWritableParts() {
            TVector<TWritablePartPtr> results;
            for (ui32 i = 0; i < PartsToWrite.size(); ++i) {
                results.push_back(ResetWritablePart(i));
            }
            return results;
        }

        TWritablePartPtr ResetWritablePart(ui32 index = 0) {
            CHECK_WITH_LOG(index < PartsToWrite.size());

            TWritablePartPtr oldPart = PartsToWrite[index];
            TWritablePartPtr partToWrite(new TArchivePartThreadSafe(TBase::Path.GetPath(), TBase::FreePartIndex++, TBase::Manager.Get(), true, *this));
            PartsToWrite[index] = partToWrite;
            TBase::Parts[partToWrite->GetPartNum()].Reset(partToWrite.Get());
            return oldPart;
        }

    public:
        virtual void OnPartFull(ui64 partNum) override {
            DEBUG_LOG << "OnPartFull " << partNum << Endl;
            TWritablePartPtr guard;
            {
                TWriteGuard g(Lock);
                ui32 partIndex = Max<ui32>();
                for (ui32 i = 0; i < PartsToWrite.size(); ++i) {
                    if (partNum == PartsToWrite[i]->GetPartNum()) {
                        partIndex = i;
                        break;
                    }
                }
                CHECK_WITH_LOG(partIndex != Max<ui32>());
                guard = ResetWritablePart(partIndex);
            }
        }

        virtual void OnPartFree(ui64 partNum) override {
            DEBUG_LOG << "OnPartFree " << partNum << Endl;
            TArchivePartThreadSafe::TPtr partPtr;
            {
                TWriteGuard g(Lock);
                auto it = TBase::Parts.find(partNum);
                if (it == TBase::Parts.end()) {
                    return;
                }
                partPtr = it->second;
                TBase::Parts.erase(it);
            }
            TArchiveGlobals::AddTask(new TRemovePartTask(partPtr, *this));
        }

        virtual void OnPartDrop(ui64 /*partNum*/) override {
        }

        virtual void OnPartClose(ui64 partNum, ui64 docsCount) override {
            DEBUG_LOG << "OnPartClose " << partNum << " with " << docsCount << " docs" << Endl;
            if (docsCount == 0) {
                OnPartFree(partNum);
            } else {
                typename TBase::TPartPtr partPtr = GetPartByNum(partNum);
                CHECK_WITH_LOG(!!partPtr);
                TArchiveGlobals::AddTask(new TClosePartTask(partPtr, *this));
            }
        }

    protected:
        TRWMutex Lock;
        ui32 WritablePartsCount = 1;

        TVector<TWritablePartPtr> PartsToWrite;
        TAtomic OperationId = 0;
    };
}
