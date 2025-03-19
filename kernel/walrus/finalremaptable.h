#pragma once

#include <util/generic/vector.h>
#include "advhititer.h"

class TFinalRemapTable : private TNonCopyable {
public:
    static const ui32 DELETED_DOCUMENT = 0xFFFFFFFF; // if output cluster is equal to this value - it means deleted doc

private:
    class TInputCluster {
        TVector<TRemapItem> Table;
        bool ChangedDocIds;
    public:
        TInputCluster()
            : ChangedDocIds(false)
        {
        }
        void EnsureSize(ui32 docId, ui32 defaultOutputCluster) {
            Y_ASSERT(docId != Max<ui32>());
            Resize(docId + 1, defaultOutputCluster);
        }
        void SetOutputCluster(ui32 docId, ui32 outputCluster) {
            Y_ASSERT(outputCluster < 31);
            TRemapItem& item = Table[docId];
            ui32& oi = item.OutputIdx;
            if (oi == DELETED_DOCUMENT) {
                item.NewDocId = docId;
                oi = outputCluster;
            } else {
                Y_ASSERT(item.NewDocId == docId); // NewDocId must already be assigned and equal to docId
                if (oi & 0x80000000) {
                    const ui32 addIndex = 1 << outputCluster;
                    oi |= addIndex;
                } else {
                    const ui32 index1 = 1 << oi;
                    const ui32 index2 = 1 << outputCluster;
                    oi = 0x80000000 | index1 | index2;
                }
            }
        }
        void SetDeletedDoc(ui32 docId) {
            Table[docId].OutputIdx = DELETED_DOCUMENT;
        }
        void SetNewDocId(ui32 docId, ui32 newDocId) {
            if (docId != newDocId) {
                ChangedDocIds = true;
                Table[docId].NewDocId = newDocId;
            }
        }
        void SetRemapItem(ui32 docId, ui32 outputCluster, ui32 newDocId) {
            if (docId != newDocId)
                ChangedDocIds = true;
            TRemapItem& item = Table[docId];
            item.NewDocId = newDocId;
            item.OutputIdx = outputCluster;
        }
        const TRemapItem* GetRemapItems() const {
            return Table.begin();
        }
        size_t GetItemCount() const {
            return Table.size();
        }
        TRemapItem& operator[](size_t i) {
            return Table[i];
        }
        const TRemapItem& operator[](size_t i) const {
            return Table[i];
        }
        void Resize(size_t newSize, ui32 defaultOutputCluster) {
            const size_t oldSize = Table.size();
            if (newSize > oldSize) {
                const TRemapItem defaultItem = { Max<ui32>(), defaultOutputCluster };
                Table.resize(newSize, defaultItem);
                if (defaultOutputCluster != DELETED_DOCUMENT) {
                    for (size_t i = oldSize; i < Table.size(); ++i)
                        Table[i].NewDocId = (ui32)i;
                }
            }
        }
        bool HasChangedDocIds() const {
            return ChangedDocIds;
        }
    };

    TVector<TInputCluster> RemapTable;    // global docid -> (newdocid, destination output); output (ui32)-1 means deleted document

    // if srcDocId is greater than size of FinalRemapTable then this member is used as value of TRemapItem::OutputIdx and NewDocId == srcDocId
    // if this member is equal to Max<ui32>() than document will be deleted from output
    // also it can be bitset, for example 0x80000007 - document should be added to #0, #1 and #2 outputs
    ui32 DefaultOutputCluster;

public:
    TFinalRemapTable()
        : DefaultOutputCluster(DELETED_DOCUMENT)
    {
    }
    void Create(size_t inputClusterCount, ui32 defaultOutputCluster) {
        Y_ASSERT(RemapTable.empty());
        RemapTable.resize(inputClusterCount);
        DefaultOutputCluster = defaultOutputCluster;
    }
    bool IsEmpty() const {
        return RemapTable.empty();
    }
    bool HasChangedDocIds() const {
        for (size_t i = 0; i < RemapTable.size(); ++i) {
            if (RemapTable[i].HasChangedDocIds())
                return true;
        }
        return false;
    }
    ui32 GetDefaultOutputCluster() const {
        return DefaultOutputCluster;
    }
    void ResizeInputCluster(ui32 inputCluster, size_t newSize) {
        RemapTable[inputCluster].Resize(newSize, DefaultOutputCluster);
    }
    void SetDeletedDoc(ui32 inputCluster, ui32 docId) {
        TInputCluster& cluster = RemapTable[inputCluster];
        cluster.EnsureSize(docId, DefaultOutputCluster);
        cluster.SetDeletedDoc(docId);
    }
    void SetOutputCluster(ui32 inputCluster, ui32 docId, ui32 outputCluster) {
        TInputCluster& cluster = RemapTable[inputCluster];
        cluster.EnsureSize(docId, DefaultOutputCluster);
        cluster.SetOutputCluster(docId, outputCluster);
    }
    void SetNewDocId(ui32 inputCluster, ui32 docId, ui32 newDocId) {
        TInputCluster& cluster = RemapTable[inputCluster];
        cluster.EnsureSize(docId, DefaultOutputCluster);
        cluster.SetNewDocId(docId, newDocId);
    }
    void SetRemapItem(ui32 inputCluster, ui32 docId, const TRemapItem& remapItem) {
        TInputCluster& cluster = RemapTable[inputCluster];
        cluster.EnsureSize(docId, DefaultOutputCluster);
        cluster.SetRemapItem(docId, remapItem.OutputIdx, remapItem.NewDocId);
    }
    void SetRemapItem(ui32 inputCluster, ui32 docId, ui32 outputCluster, ui32 newDocId) {
        TInputCluster& cluster = RemapTable[inputCluster];
        cluster.EnsureSize(docId, DefaultOutputCluster);
        cluster.SetRemapItem(docId, outputCluster, newDocId);
    }
    const TInputCluster& GetInputCluster(ui32 inputCluster) const {
        return (RemapTable.size() > 1 ? RemapTable[inputCluster] : RemapTable[0]);
    }
    const TRemapItem* GetInputClusterItems(size_t i) const {
        return GetInputCluster((ui32)i).GetRemapItems();
    }
    size_t GetInputClusterSize(size_t i) const {
        return GetInputCluster((ui32)i).GetItemCount();
    }
    size_t GetInputClusterCount() const {
        return RemapTable.size();
    }
    bool GetDst(ui32 inputCluster, ui32 docId, ui32& outputCluster, ui32& newDocId) const {
        if (IsEmpty()) {
            outputCluster = 0;
            newDocId = docId;
            return true;
        }

        const TFinalRemapTable::TInputCluster& cluster = GetInputCluster(inputCluster);
        if (docId >= cluster.GetItemCount()) {
            if (DefaultOutputCluster == DELETED_DOCUMENT)
                return false;
            outputCluster = DefaultOutputCluster;
            newDocId = docId;
            return true;
        }

        const TRemapItem& remapItem = cluster[docId];
        if (remapItem.OutputIdx == DELETED_DOCUMENT)
            return false;
        outputCluster = remapItem.OutputIdx;
        newDocId = remapItem.NewDocId;
        return true;
    }
};

