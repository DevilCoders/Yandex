#pragma once

#include "constants.h"

#include <library/cpp/knn_index/index_info.pb.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

namespace NKNNIndex {
    /**
     * @brief Writer class for KNNIndex.
     * Data definitions:
     *  1. Each cluster consist of zero or more documents
     *  2. Each document consist of zero or more items and has some DocumentId
     *  3. Each item is a feature vector of fixed length(dimension) and has some ItemId
     *
     * Data blob has 3 parts (each part is 16-byte aligned):
     *  <items>
     *      Array of 16-byte aligned feature vector of items of all the documents
     *      grouped by clusters
     *  </items>
     *  <clusters>
     *      Array of 16-byte aligned feature vectors of clusters
     *  </clusters>
     *  <header>
     *      TIndexInfo(proto-structure)     feature vector dimension, array sizes, ...
     *      TUserIndexInfo(proto-structure) arbitrary user-defined structures
     *      ClusterOffsetsArray             offsets to get documents of cluster
     *      ItemIdArray                     get itemId (by item position) for each item
     *      DocumentIdArray                 get documentId (by item position) for each item
     *      DocumentItems                   list of item positions sorted by docId
     *      DocumentOffset                  offset for each document into DocumentItems
     *  </header>
     *
     * @code
     *  TWriter writer(dimension, outputStream);
     *  for (auto cluster : clusters) {
     *      for (auto item : cluster) {
     *          writer.WriteItem(item.DocumentId(), item.ItemId(), item.FeatureBeginPtr());
     *      }
     *      writer.WriteCluster(cluster.FeatureBeginPtr());
     *  }
     *  writer.Finish();
     * @endcode
     */
    template <class FeatureType, class UserInfo = TDefaultUserIndexInfo>
    class TWriter {
    public:
        using TFeatureType = FeatureType;
        using TUserIndexInfo = UserInfo;

        explicit TWriter(size_t dimension, IOutputStream* output,
                         const TUserIndexInfo& userIndexInfo = TUserIndexInfo()) {
            Y_VERIFY(output);
            Reset(dimension, output, userIndexInfo);
        }

        explicit TWriter(size_t dimension, const TString& path,
                         const TUserIndexInfo& userIndexInfo = TUserIndexInfo()) {
            OutputHolder_.Reset(new TOFStream(path));
            Reset(dimension, OutputHolder_.Get(), userIndexInfo);
        }

        size_t Dimension() const {
            return Info_.GetDimension();
        }

        void WriteItem(ui32 documentId, ui32 itemId, const TFeatureType* item) {
            static TVector<TFeatureType> zeroData(AlignedDimension(), 0);

            Y_VERIFY(item);
            DocumentId_.push_back(documentId);
            ItemId_.push_back(itemId);
            Info_.SetMaxDocumentId(Max(Info_.GetMaxDocumentId(), documentId));
            Output_->Write(item, Dimension() * sizeof(TFeatureType));
            Output_->Write(zeroData.data(), (AlignedDimension() - Dimension()) * sizeof(TFeatureType));
        }

        void WriteCluster(const TFeatureType* cluster) {
            Y_VERIFY(cluster);
            ClusterOffset_.push_back(ItemId_.size());
            Write(cluster, &ClusterBuffer_);
        }

        void Finish() {
            if (!IsFinished_) {
                IsFinished_ = true;

                TBuffer header;
                WriteHeader(&header);

                Output_->Write(ClusterBuffer_.data(), ClusterBuffer_.size());
                Output_->Write(header.data(), header.size());
                ui32 headerSize = header.size();
                Output_->Write(&headerSize, sizeof(headerSize));
            }
        }

        ~TWriter() {
            Finish();
        }

    private:
        size_t AlignedDimension() const {
            return AlignUp(Dimension() * sizeof(TFeatureType), DataAlignment) / sizeof(TFeatureType);
        }

        void Reset(size_t dimension, IOutputStream* output, const TUserIndexInfo& userIndexInfo) {
            UserIndexInfo_ = userIndexInfo;
            Info_.SetDimension(dimension);
            Info_.SetMaxDocumentId(0);
            Output_ = output;
            ClusterOffset_.assign(1, 0); /* Get cluster docs in searcher faster and more comfortable */
        }

        void Write(const TFeatureType* vector, TBuffer* buffer) {
            buffer->Append((const char*)vector, sizeof(TFeatureType) * Dimension());
            buffer->AlignUp(DataAlignment, 0);
        }

        void WriteHeader(TBuffer* buffer) {
            Y_VERIFY(!ClusterOffset_.empty()); /* At least offset 0 must exist in offsets vector */
            Info_.SetClusterCount(ClusterOffset_.size() - 1);
            Info_.SetItemCount(ItemId_.size());

            /* Serialize IndexInfo-proto structure into header */
            TString serializedInfo = Info_.SerializeAsString();
            ui32 infoSize = serializedInfo.length();
            buffer->Append((const char*)&infoSize, sizeof(infoSize));
            buffer->Append(serializedInfo.data(), serializedInfo.size());

            /* Serialize UserIndexInfo-proto structure into header */
            TString serializedUserInfo = UserIndexInfo_.SerializeAsString();
            ui32 userInfoSize = serializedUserInfo.length();
            buffer->Append((const char*)&userInfoSize, sizeof(userInfoSize));
            buffer->Append(serializedUserInfo.data(), serializedUserInfo.size());

            /* Calculate mapping of DocumentId -> ItemList */
            ui32 documentCount = Info_.GetMaxDocumentId() + 1;
            TVector<TVector<ui32>> documentItems(documentCount);
            for (size_t i = 0; i < DocumentId_.size(); ++i) {
                documentItems[DocumentId_[i]].push_back(i);
            }
            TVector<ui32> documentItemsFlat;
            documentItemsFlat.reserve(ItemId_.size());
            TVector<ui32> documentOffsets(documentCount + 1, 0);
            for (size_t documentId = 0; documentId < documentItems.size(); ++documentId) {
                for (size_t i = 0; i < documentItems[documentId].size(); ++i) {
                    documentItemsFlat.push_back(documentItems[documentId][i]);
                }
                documentOffsets[documentId + 1] = documentItemsFlat.size();
            }

            /* Serialize all the array into header */
            buffer->Append((const char*)ClusterOffset_.data(), sizeof(ClusterOffset_[0]) * ClusterOffset_.size());
            buffer->Append((const char*)ItemId_.data(), sizeof(ItemId_[0]) * ItemId_.size());
            buffer->Append((const char*)DocumentId_.data(), sizeof(DocumentId_[0]) * DocumentId_.size());
            buffer->Append((const char*)documentItemsFlat.data(), sizeof(documentItemsFlat[0]) * documentItemsFlat.size());
            buffer->Append((const char*)documentOffsets.data(), sizeof(documentOffsets[0]) * documentOffsets.size());
        }

    private:
        bool IsFinished_ = false;

        THolder<IOutputStream> OutputHolder_;
        IOutputStream* Output_ = nullptr;

        TUserIndexInfo UserIndexInfo_;
        TIndexInfo Info_;
        TBuffer ClusterBuffer_;
        TVector<ui32> ClusterOffset_;
        TVector<ui32> DocumentId_; /* indexed by ItemPosition */
        TVector<ui32> ItemId_;     /* indexed by ItemPosition */
    };

}
