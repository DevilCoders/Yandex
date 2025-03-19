#pragma once

#include <library/cpp/deprecated/autoarray/autoarray.h>

#include <util/generic/noncopyable.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>

namespace NM2N {
    template<typename TImpl> class TBaseRemapTable;
    class TRemapTableImpl;
    class TMultipleRemapTableImpl;
    class TCompactMultipleRemapTable;

    typedef TBaseRemapTable<TRemapTableImpl> TRemapTable;
    typedef TBaseRemapTable<TMultipleRemapTableImpl> TMultipleRemapTable;

    template <class TRemap>
    void InitRemapTable(TRemap& remapTable, const char* filename);
}

void MakeOwnersRemapTable(const autoarray<const char *>& infiles, const autoarray<const char*>& outfiles, const char *attr, NM2N::TMultipleRemapTable *res, bool avoidDuplicates);

namespace NM2N {

    class TBaseRemapInfo : TNonCopyable {
        TVector<ui32> DstMaxDocIds;

    public:
        static const ui32 EmptyDoc;
        static const ui16 EmptyCl;

        TBaseRemapInfo()
        {
        }

        void UpdateMaxDstClusterAndDocId(ui16 cl, ui32 docId) {
            if (size_t(cl + 1) > DstMaxDocIds.size())
                DstMaxDocIds.resize(cl + 1, EmptyDoc);
            DstMaxDocIds[cl] = Max<int>(DstMaxDocIds[cl], docId);
        }

        ui16 DstClustersCount() const noexcept {
            return DstMaxDocIds.size();
        }

        // can return TBaseRemapInfo::EmptyDoc
        ui32 GetDstMaxDocId(ui16 cl) const noexcept {
            if (cl >= DstMaxDocIds.size())
                return EmptyDoc;

            return DstMaxDocIds[cl];
        }

        void CopyBaseInfo(const TBaseRemapInfo& source) {
            DstMaxDocIds = source.DstMaxDocIds;
        }
    };

    struct TDocCl {
        ui32 DocId;
        ui16 ClId;

        TDocCl(ui32 docId, ui16 clId)
            : DocId(docId)
            , ClId(clId)
        {
        }
    };

    class TBaseRemapData {
    protected:
        TVector< TVector<ui16> > ClusterIds;
        TVector< TVector<ui32> > DocIds;

    public:
        void ResizeHolders(const TVector<ui32>& srcDocsCount);

        ui16 SrcClustersCount() const noexcept {
            return ClusterIds.size();
        }

        // can return TBaseRemapInfo::EmptyDoc
        ui32 GetSrcMaxDocId(ui16 clId) const noexcept {
            Y_ASSERT(clId < SrcClustersCount());
            return ClusterIds[clId].size() - 1;
        }
    };

    template <typename TImpl>
    class TBaseRemapTable : public TImpl {
    private:
        bool IsCellEmpty(ui16 clId, ui32 docId) const {
            if (clId >= SrcClustersCount() || docId >= TImpl::ClusterIds[clId].size()) {
                return true;
            }

            return TImpl::ClusterIds[clId][docId] == TBaseRemapInfo::EmptyCl;
        }

        bool GetSingleDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const {
            if (IsCellEmpty(srcClId, srcDocId)) {
                return false;
            }

            *dstClId = TImpl::ClusterIds[srcClId][srcDocId];
            *dstDocId = TImpl::DocIds[srcClId][srcDocId];

            return true;
        }

    public:
        void AddRemap(ui32 srcDocId, ui16 srcClId, const TVector<TDocCl>& dsts) {
            for (size_t i = 0; i < dsts.size(); ++i) {
                AddRemap(srcDocId, srcClId, dsts[i].DocId, dsts[i].ClId);
            }
        }

        void AddRemap(ui32 srcDocId, ui16 srcClId, ui32 dstDocId, ui16 dstClId) {
            if (IsCellEmpty(srcClId, srcDocId)) {
                TImpl::ClusterIds[srcClId][srcDocId] = dstClId;
                TImpl::DocIds[srcClId][srcDocId] = dstDocId;
                return;
            }

            TImpl::AddRemapNext(srcDocId, srcClId, dstDocId, dstClId);
        }

        bool GetDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const {
            if (!GetSingleDst(srcDocId, srcClId, dstDocId, dstClId)) {
                return false;
            }

            return TImpl::GetDst(srcDocId, srcClId, dstDocId, dstClId);
        }

        bool GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const {
            ui32 dstDocId;
            ui16 dstClId;
            if (!GetSingleDst(srcDocId, srcClId, &dstDocId, &dstClId)) {
                return false;
            }

            result->clear();
            result->push_back(TDocCl(dstDocId, dstClId));

            return TImpl::GetMultipleDst(srcDocId, srcClId, result);
        }

        ui16 SrcClustersCount() const noexcept {
            return TImpl::SrcClustersCount();
        }

        void FreeUnusedMemory() {
        }
    };

    class TRemapTableImpl : public TBaseRemapInfo, public TBaseRemapData {
    protected:
        void AddRemapNext(ui32 srcDocId, ui16 srcClId, ui32 dstDocId, ui16 dstClId);

    public:
        bool GetDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const;
        bool GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const;

        bool IsEmpty() const {
            return !SrcClustersCount() || !DstClustersCount();
        }
    };

    class TMultipleRemapTableImpl : public TRemapTableImpl {
    protected:
        TVector<TVector<ui32> > Nexts;

        bool IsBufferInited;
        ui16 BufferClId;

        static const ui32 EmptyNext;

    private:
        bool IsNextEmpty(ui16 clId, ui32 docId) const noexcept {
            return Nexts[clId][docId] == Max<ui32>();
        }

        void SetBufferClId(ui16 bufferClId) {
            if (!IsBufferInited) {
                BufferClId = bufferClId;
                ClusterIds.resize(BufferClId + 1);
                DocIds.resize(BufferClId + 1);
                Nexts.resize(BufferClId + 1);

                IsBufferInited = true;
            }
        }

    protected:
        void AddRemapNext(ui32 srcDocId, ui16 srcClId, ui32 dstDocId, ui16 dstClId);

    public:
        TMultipleRemapTableImpl()
            : IsBufferInited(false)
        {}

        void ResizeHolders(const TVector<ui32>& srcDocsCount);

        bool GetDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const;
        bool GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const;

        ui16 SrcClustersCount() const noexcept {
            return ClusterIds.size() - 1;
        }

        friend void ::MakeOwnersRemapTable(const autoarray<const char*>& infiles, const autoarray<const char*>& outfiles, const char* attr, NM2N::TMultipleRemapTable* res, bool);
    };

    //
    // allows only forward iteration, but requires low memory
    //
    template <typename TRemapSource>
    class TForwardMultipleRemapTableImpl : public TBaseRemapInfo {
        TVector<ui32> SrcDocsCount;

        TRemapSource RemapSource;

        bool SeekToRec(ui16 srcClId, ui32 srcDocId) const {
            for (; RemapSource.IsValid() && (RemapSource.GetFromCl() < srcClId); RemapSource.Next()) {}
            for (; RemapSource.IsValid() && (RemapSource.GetFromCl() == srcClId) && (RemapSource.GetFromDocId() < srcDocId); RemapSource.Next()) {}

            if (!RemapSource.IsValid() || RemapSource.GetFromCl() != srcClId || RemapSource.GetFromDocId() != srcDocId)
                return false;

            return true;
        }

    public:
        template <typename T>
        explicit TForwardMultipleRemapTableImpl(T& remapSourceParam)
            : RemapSource(remapSourceParam)
        {
            InitRemapTable(*this, RemapSource);
            RemapSource.Restart();
        }

        void ResizeHolders(const TVector<ui32>& srcDocsCount) {
            SrcDocsCount = srcDocsCount;
        }

        ui16 SrcClustersCount() const noexcept {
            return SrcDocsCount.size();
        }

        // can return TBaseRemapInfo::EmptyDoc
        ui32 GetSrcMaxDocId(ui16 clId) const noexcept {
            Y_ASSERT(clId < SrcClustersCount());
            return SrcDocsCount[clId] - 1;
        }

        bool GetDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const {
            if (!SeekToRec(srcClId, srcDocId))
                return false;

            *dstClId = RemapSource.GetToCl();
            *dstDocId = RemapSource.GetToDocId();

            RemapSource.Next();

            if (RemapSource.IsValid() && RemapSource.GetFromCl() == srcClId && RemapSource.GetFromDocId() == srcDocId)
                ythrow yexception() << "Remap with srcClId=" << srcClId << ", srcDocId=" << srcDocId << " has multiple destinations";

            return true;
        }

        bool GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const {
            if (!SeekToRec(srcClId, srcDocId))
                return false;

            result->clear();

            while (RemapSource.IsValid() && RemapSource.GetFromCl() == srcClId && RemapSource.GetFromDocId() == srcDocId) {
                result->push_back(TDocCl(RemapSource.GetToDocId(), RemapSource.GetToCl()));

                RemapSource.Next();
            }

            return true;
        }

        bool IsEmpty() const {
            return !SrcClustersCount() || !DstClustersCount();
        }
    };

    class TCompactMultipleRemapTable : public TBaseRemapInfo, public TBaseRemapData {
        // TBaseRemapData::ClusterIds:
        //     one bit is used as a flag.
        //     0 means single dest doc.
        //     1 means multiple dst docs which are stored in ClusterDstLists and DocIdsDstLists.
        //     14 bits for cluster number

        // TBaseRemapData::DocIds:
        // stores either docid (when single dst doc) or offset in lists ClusterDstLists and DocIdsDstLists

        // one bit is used as a flag.
        // 0 means last item in list.
        // 1 means there are other items.
        TVector<ui16> ClusterDstLists;

        TVector<ui32> DocIdsDstLists;

public:
        TVector<ui32> DstCounts;
        TVector<ui32> SrcCounts;
        bool CopyDocIds = false;

    public:
        TCompactMultipleRemapTable()
        {}

        bool IsCellEmpty(ui16 clId, ui32 docId) const {
            if (clId >= SrcClustersCount() || docId >= ClusterIds[clId].size()) {
                return true;
            }

            return ClusterIds[clId][docId] == TBaseRemapInfo::EmptyCl;
        }

        void AddRemap(ui32 srcDocId, ui16 srcClId, ui32 dstDocId, ui16 dstClId) {
            Y_ASSERT(IsCellEmpty(srcClId, srcDocId));
            ClusterIds[srcClId][srcDocId] = dstClId;
            DocIds[srcClId][srcDocId] = dstDocId;
        }

        void UpdateDstCounts(ui32 toCl) {
            if (DstCounts.size() <= toCl)
                DstCounts.resize(toCl + 1);
            DstCounts[toCl]++;
        }

        void UpdateSrcCounts(ui32 fromCl) {
            if (SrcCounts.size() <= fromCl)
                SrcCounts.resize(fromCl + 1);
            SrcCounts[fromCl]++;
        }

        void AddRemap(ui32 srcDocId, ui16 srcClId, const TVector<TDocCl>& dsts) {
            UpdateSrcCounts(srcClId);
            if (dsts.size() == 1) {
                AddRemap(srcDocId, srcClId, dsts[0].DocId, dsts[0].ClId);
                UpdateDstCounts(dsts[0].ClId);
            } else {
                Y_ASSERT(IsCellEmpty(srcClId, srcDocId));
                ClusterIds[srcClId][srcDocId] = 1 << 15;
                // save offset instead of docid
                DocIds[srcClId][srcDocId] = ClusterDstLists.size();

                for (size_t i = 0; i < dsts.size(); ++i) {
                    ui16 cl = dsts[i].ClId;
                    UpdateDstCounts(cl);
                    if (i < (dsts.size() - 1))
                        cl |= (1 << 15);  // flag: there are other items in the list
                    ClusterDstLists.push_back(cl);

                    DocIdsDstLists.push_back(dsts[i].DocId);
                }
            }
        }

        void FreeUnusedMemory() {
            ClusterDstLists.shrink_to_fit();
            DocIdsDstLists.shrink_to_fit();
        }

        bool GetDst(ui32 srcDocId, ui16 srcClId, ui32* dstDocId, ui16* dstClId) const {
            if (IsCellEmpty(srcClId, srcDocId)) {
                return false;
            }

            if (ClusterIds[srcClId][srcDocId] != (1 << 15)) {
                *dstClId = ClusterIds[srcClId][srcDocId];
                *dstDocId = DocIds[srcClId][srcDocId];
            } else {
                ythrow yexception() << "Remap with srcClId=" << srcClId << ", srcDocId=" << srcDocId << " has multiple destinations";
            }

            return true;
        }

        bool GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const {
            if (CopyDocIds) {
                result->clear();
                result->emplace_back(srcDocId, 0);
                return true;
            }

            if (IsCellEmpty(srcClId, srcDocId)) {
                return false;
            }

            result->clear();

            if (ClusterIds[srcClId][srcDocId] != (1 << 15)) {
                result->push_back(TDocCl(DocIds[srcClId][srcDocId], ClusterIds[srcClId][srcDocId]));
            } else {
                ui32 offset = DocIds[srcClId][srcDocId];

                ui32 pos = 0;

                do {
                    ui16 cl = ClusterDstLists[offset + pos] & ~(1 << 15);
                    result->push_back(TDocCl(DocIdsDstLists[offset + pos], cl));

                    pos++;
                } while (ClusterDstLists[offset + pos - 1] & (1 << 15));
            }

            return true;
        }

        bool IsEmpty() const {
            return !SrcClustersCount() || !DstClustersCount();
        }
    };

    template <class TRemap>
    void FillCompactRemapTable(const TRemap& source, TCompactMultipleRemapTable* result) {
        result->CopyBaseInfo(source);

        TVector<ui32> srcDocsCount;
        for (ui16 cl = 0; cl < source.SrcClustersCount(); ++cl)
            srcDocsCount.push_back(source.GetSrcMaxDocId(cl) + 1);

        result->ResizeHolders(srcDocsCount);

        TVector<TDocCl> dsts;

        for (ui16 srcCluster = 0; srcCluster < source.SrcClustersCount(); ++srcCluster) {
            if (source.GetSrcMaxDocId(srcCluster) == NM2N::TBaseRemapInfo::EmptyDoc)
                continue;

            for (ui32 srcDocId = 0; srcDocId <= source.GetSrcMaxDocId(srcCluster); ++srcDocId) {
                if (source.GetMultipleDst(srcDocId, srcCluster, &dsts)) {
                    result->AddRemap(srcDocId, srcCluster, dsts);
                }
            }
        }

        result->FreeUnusedMemory();
    }

    template <class TRemap, class TRemapSource>
    void InitRemapTable(TRemap& remapTable, TRemapSource& remapSource) {
        TVector<ui32> srcDocsCount;

        {
            for (remapSource.Restart(); remapSource.IsValid(); remapSource.Next()) {
                const ui16 fromCl = remapSource.GetFromCl();

                if (srcDocsCount.size() <= fromCl) {
                    srcDocsCount.resize(fromCl + 1, 0);
                }

                srcDocsCount[fromCl] = Max<int>(remapSource.GetFromDocId() + 1, srcDocsCount[fromCl]);
                remapTable.UpdateMaxDstClusterAndDocId(remapSource.GetToCl(), remapSource.GetToDocId());
            }
        }

        remapTable.ResizeHolders(srcDocsCount);
    }

    template <class TRemap, class TRemapSource>
    void LoadRemapTableFromSource(TRemap& remapTable, TRemapSource& remapSource, bool allowManyLinksToDstDoc = false) {
        InitRemapTable(remapTable, remapSource);

        // prevent from mapping to the same doc twice
        TVector< TVector<bool> > LinksToDstDocsMask(remapTable.DstClustersCount());

        for (ui16 cl = 0; cl < remapTable.DstClustersCount(); ++cl) {
            LinksToDstDocsMask[cl].resize(remapTable.GetDstMaxDocId(cl) + 1, false);
        }

        TVector<TDocCl> dsts;

        ui16 lastSrcCl = Max<ui16>();
        ui32 lastSrcDocId = Max<ui32>();

        for (remapSource.Restart(); remapSource.IsValid(); remapSource.Next()) {
            const ui16 fromCl = remapSource.GetFromCl();
            const ui32 fromDocId = remapSource.GetFromDocId();
            const ui16 toCl = remapSource.GetToCl();
            const ui32 toDocId = remapSource.GetToDocId();

            if (!allowManyLinksToDstDoc) {
                if (!LinksToDstDocsMask[toCl][toDocId])
                    LinksToDstDocsMask[toCl][toDocId] = true;
                else
                    ythrow yexception() << "more than one links to cl " << toCl << " and docid " << toDocId;
            }

            if (lastSrcCl != fromCl || lastSrcDocId != fromDocId) {
                if (!dsts.empty())
                    remapTable.AddRemap(lastSrcDocId, lastSrcCl, dsts);

                dsts.clear();

                lastSrcCl = fromCl;
                lastSrcDocId = fromDocId;
            }

            dsts.push_back(TDocCl(toDocId, toCl));
        }

        if (!dsts.empty())
            remapTable.AddRemap(lastSrcDocId, lastSrcCl, dsts);

        remapTable.FreeUnusedMemory();
    }
}

template<class TSrcRemapTable, class TDestRemapTable>
void InvertRemapTable(const TSrcRemapTable& source, TDestRemapTable* result) {
    TVector<ui32> dstClusterDocCount;
    for (ui16 dstCluster = 0; dstCluster < source.DstClustersCount(); ++dstCluster)
        dstClusterDocCount.push_back(source.GetDstMaxDocId(dstCluster) + 1);

    result->ResizeHolders(dstClusterDocCount);

    TVector<NM2N::TDocCl> dsts;

    for (ui16 srcCluster = 0; srcCluster < source.SrcClustersCount(); ++srcCluster) {
        if (source.GetSrcMaxDocId(srcCluster) == NM2N::TBaseRemapInfo::EmptyDoc)
            continue;

        for (ui32 srcDocId = 0; srcDocId <= source.GetSrcMaxDocId(srcCluster); ++srcDocId) {
            if (source.GetMultipleDst(srcDocId, srcCluster, &dsts)) {
                for (size_t i = 0; i < dsts.size(); ++i) {
                    result->AddRemap(dsts[i].DocId, dsts[i].ClId, srcDocId, srcCluster);
                    result->UpdateMaxDstClusterAndDocId(srcCluster, srcDocId);
                }
            }
        }
    }
}
