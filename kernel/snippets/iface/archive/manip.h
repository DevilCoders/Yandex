#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <util/generic/ptr.h>
#include <util/memory/blob.h>

namespace NSnippets {
    namespace NProto {
        class TArc;
    }
    struct IArchiveConsumer {
        virtual int UnpText(const ui8* doctext) = 0;
        virtual ~IArchiveConsumer() = default;
    };
    struct ISpecificArchiveFetcher {
        virtual bool IsInArchive() = 0;
        virtual TBlob FetchData() = 0;
        virtual TBlob FetchExtInfo() = 0;
        virtual ~ISpecificArchiveFetcher() = default;
    };
    class TVoidFetcher : public ISpecificArchiveFetcher {
    public:
        TVoidFetcher() = default;
        bool IsInArchive() override {
            return false;
        }
        TBlob FetchData() override {
            Y_ASSERT(false);
            return TBlob();
        }
        TBlob FetchExtInfo() override {
            Y_ASSERT(false);
            return TBlob();
        }
    };

    class TArc: private TNonCopyable {
    private:
        ISpecificArchiveFetcher& ArchiveData;
        THolder<bool> Valid;
        THolder<TBlob> Data;
        THolder<TBlob> DescrBlob;
        THolder<TDocDescr> Descr;
        TDocInfosPtr DocInfos;
    private:
        void ResetDescr() {
            Descr.Reset();
            DocInfos.Reset();
        }
        bool FetchValid() const {
            return ArchiveData.IsInArchive();
        }
        TBlob FetchData() const {
            return ArchiveData.FetchData();
        }
        TBlob FetchExtInfo() const {
            return ArchiveData.FetchExtInfo();
        }
    public:
        explicit TArc(ISpecificArchiveFetcher& archiveData)
          : ArchiveData(archiveData)
        {
        }
        void Reset() {
            Valid.Reset();
            Data.Reset();
            DescrBlob.Reset();
            ResetDescr();
        }
    public:
        bool IsValid() {
            if (!Valid.Get()) {
                Valid = MakeHolder<bool>(FetchValid());
            }
            return *Valid;
        }
        void SetExtInfo(TBlob&& descrBlob) {
            if (IsValid()) {
                DescrBlob = MakeHolder<TBlob>(std::move(descrBlob));
            }
        }
        void PrefetchExtInfo() {
            if (IsValid() && !DescrBlob) {
                DescrBlob = MakeHolder<TBlob>(FetchExtInfo());
            }
        }
        void SetData(TBlob&& data) {
            if (IsValid()) {
                Data = MakeHolder<TBlob>(std::move(data));
            }
        }
        void PrefetchData() {
            if (IsValid() && !Data) {
                Data = MakeHolder<TBlob>(FetchData());
            }
        }
        void PrefetchAll() {
            PrefetchExtInfo();
            PrefetchData();
        }
        void FillDescr() {
            PrefetchExtInfo();
            if (IsValid() && !Descr && DescrBlob) {
                Descr = MakeHolder<TDocDescr>();
                Descr->UseBlob(DescrBlob->Data(), DescrBlob->Size());
            }
            if (IsValid() && Descr && !DocInfos) {
                DocInfos = TDocInfosPtr(new TDocInfos());
                Descr->ConfigureDocInfos(*DocInfos);
            }
        }
    public:
        TBlob GetData() {
            PrefetchData();
            return Data ? *Data : TBlob();
        }
        TBlob GetDescrBlob() {
            PrefetchExtInfo();
            return DescrBlob ? *DescrBlob : TBlob();
        }
        TDocDescr GetDescr() {
            FillDescr();
            return Descr ? *Descr : TDocDescr();
        }
        TDocInfosPtr GetDocInfosPtr() {
            FillDescr();
            return DocInfos ? DocInfos : TDocInfosPtr(new TDocInfos());
        }
    public:
        void SaveState(NProto::TArc& res) const;
        void LoadState(const NProto::TArc& res);
        void SaveStateUnfetchedComplement(NProto::TArc& res) const;
    };

    class TArcManip: private TNonCopyable
    {
        private:
            TArc TextArc;
            TArc LinkArc;

        private:
            static int GetUnpAnyText(TArc& arc, IArchiveConsumer& onArchive);

        public:
            TArcManip(ISpecificArchiveFetcher& textArchiveData, ISpecificArchiveFetcher& linkArchiveData)
                : TextArc(textArchiveData)
                , LinkArc(linkArchiveData)
            {
            }

            TArc& GetTextArc()
            {
                return TextArc;
            }

            TArc& GetLinkArc()
            {
                return LinkArc;
            }

            int GetUnpText(IArchiveConsumer& onArchive, bool byLink);
    };
}
