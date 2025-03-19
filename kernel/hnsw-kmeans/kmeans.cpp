#include "kmeans.h"
#include "lib/fixed_dimension_l2_sqr_distance.h"

#include <robot/library/yt/static/table.h>

#include <mapreduce/yt/interface/io.h>

#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index/dense_vector_index.h>
#include <library/cpp/hnsw/index_builder/index_builder.h>
#include <library/cpp/hnsw/index_builder/index_data.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>

#include <util/generic/buffer.h>
#include <util/generic/size_literals.h>
#include <util/thread/pool.h>
#include <util/random/random.h>

namespace NHnswKMeans {
    void AutoDetectEmbeddingSize(TParams& params);

    class TSearchNearestClusterMapper: public NYT::IMapper<
        NYT::TTableReader<NHnswKMeansProto::TItem>,
        NYT::TTableWriter<NHnswKMeansProto::TCluster>>
    {
    public:
        TSearchNearestClusterMapper() = default;

        TSearchNearestClusterMapper(ui32 dimension, ui32 searchNeighborhoodSize, TBuffer indexBlob, TBuffer vectorsBlob)
            : Dimension_(dimension)
            , SearchNeighborhoodSize_(searchNeighborhoodSize)
            , IndexBlob_(std::move(indexBlob))
            , VectorsBlob_(std::move(vectorsBlob))
        {
        }

        TSearchNearestClusterMapper(ui32 dimension, ui32 clusters)
            : Dimension_(dimension)
            , InitClusters_(clusters)
        {
        }

        Y_SAVELOAD_JOB(Dimension_, SearchNeighborhoodSize_, IndexBlob_, VectorsBlob_, InitClusters_)

        void Start(NYT::TTableWriter<NHnswKMeansProto::TCluster>* /*output*/) override {
            if (!InitClusters_) {
                HnswIndex_ = MakeHolder<NHnsw::THnswDenseVectorIndex<float>>(TBlob::FromBuffer(IndexBlob_), TBlob::FromBuffer(VectorsBlob_), Dimension_);
            }
        }

        void Do(NYT::TTableReader<NHnswKMeansProto::TItem>* input, NYT::TTableWriter<NHnswKMeansProto::TCluster>* output) override {
            TVector<NHnswKMeansProto::TCluster> clusters(InitClusters_.Defined() ? *InitClusters_ : HnswIndex_->GetNumItems());
            for (size_t i = 0; i < clusters.size(); ++i) {
                clusters[i].SetId(i);
                clusters[i].SetSize(0);
                clusters[i].MutableEmbedding()->MutableValues()->Resize(Dimension_, 0);
            }
            for (; input->IsValid(); input->Next()) {
                const NHnswKMeansProto::TItem& vector = input->GetRow();
                Y_ENSURE(vector.GetEmbedding().ValuesSize() == Dimension_);

                size_t index;
                double energy;
                if (!InitClusters_) {
                    using TNeighbor = NHnsw::THnswDenseVectorIndex<float>::TNeighbor<TFixedDimensionL2SqrDistance::TResult>;

                    TVector<TNeighbor> neighbors = HnswIndex_->GetNearestNeighbors<TFixedDimensionL2SqrDistance>(
                        vector.GetEmbedding().GetValues().data(),
                        1,
                        SearchNeighborhoodSize_,
                        TFixedDimensionL2SqrDistance(Dimension_));

                    Y_ENSURE(neighbors.size() == 1);

                    index = neighbors[0].Id;
                    energy = neighbors[0].Dist;
                } else {
                    index = RandomNumber<ui32>(*InitClusters_);
                    energy = 0;
                }

                Y_ENSURE(index < clusters.size());
                clusters[index].SetSize(clusters[index].GetSize() + 1);
                clusters[index].SetEnergy(clusters[index].GetEnergy() + energy);
                for (size_t j = 0; j < Dimension_; ++j) {
                    clusters[index].MutableEmbedding()->SetValues(
                        j,
                        clusters[index].GetEmbedding().GetValues(j) + vector.GetEmbedding().GetValues(j));
                }
            }

            for (size_t i = 0; i < clusters.size(); ++i) {
                if (clusters[i].GetSize() > 0) {
                    output->AddRow(clusters[i]);
                }
            }
        }

    private:
        ui32 Dimension_ = 0;
        ui32 SearchNeighborhoodSize_ = 0;
        THolder<NHnsw::THnswDenseVectorIndex<float>> HnswIndex_;
        TBuffer IndexBlob_;
        TBuffer VectorsBlob_;
        TMaybe<ui32> InitClusters_;
    };

    REGISTER_MAPPER(TSearchNearestClusterMapper)

    class TClustersReducer: public NYT::IReducer<NYT::TTableReader<NHnswKMeansProto::TCluster>, NYT::TTableWriter<NHnswKMeansProto::TCluster>> {
    public:
        void Do(NYT::TTableReader<NHnswKMeansProto::TCluster>* input, NYT::TTableWriter<NHnswKMeansProto::TCluster>* output) override {
            Y_ENSURE(input->IsValid());
            NHnswKMeansProto::TCluster cluster = input->MoveRow();
            for (input->Next(); input->IsValid(); input->Next()) {
                const NHnswKMeansProto::TCluster& anotherCluster = input->GetRow();
                Y_ENSURE(cluster.GetId() == anotherCluster.GetId());
                cluster.SetSize(cluster.GetSize() + anotherCluster.GetSize());
                cluster.SetEnergy(cluster.GetEnergy() + anotherCluster.GetEnergy());
                Y_ENSURE(cluster.GetEmbedding().ValuesSize() == anotherCluster.GetEmbedding().ValuesSize());
                for (size_t i = 0; i < cluster.GetEmbedding().ValuesSize(); ++i) {
                    cluster.MutableEmbedding()->SetValues(
                        i,
                        cluster.GetEmbedding().GetValues(i) + anotherCluster.GetEmbedding().GetValues(i));
                }
            }
            Y_ENSURE(cluster.GetSize() > 0);
            output->AddRow(cluster);
        }
    };

    REGISTER_REDUCER(TClustersReducer)

    class TLoadClusterVectorsTask: public IObjectInQueue {
    public:
        TLoadClusterVectorsTask(NYT::IClientPtr client, ui32 dimension, const NYT::TRichYPath& clustersTablePath, ui32 from, ui32 to)
            : Client_(client)
            , Dimension_(dimension)
            , ClustersTablePath_(clustersTablePath)
            , From_(from)
            , To_(to)
        {
            Y_ENSURE(From_ < To_);
        }

        void Process(void*) override {
            try {
                NYT::TTableReaderPtr<NHnswKMeansProto::TCluster> input = Client_->CreateTableReader<NHnswKMeansProto::TCluster>(
                    NYT::TRichYPath(ClustersTablePath_)
                        .AddRange(NYT::TReadRange::FromRowIndices(From_, To_)));

                for (; input->IsValid(); input->Next()) {
                    const NHnswKMeansProto::TCluster& cluster = input->GetRow();
                    Y_ENSURE(cluster.GetSize() > 0);
                    TVector<float> vector(cluster.GetEmbedding().GetValues().begin(), cluster.GetEmbedding().GetValues().end());
                    Y_ENSURE(vector.size() == Dimension_);
                    for (size_t i = 0; i < Dimension_; ++i) {
                        vector[i] /= cluster.GetSize();
                    }
                    Vectors_.push_back(std::move(vector));
                    Ids_.push_back(cluster.GetId());
                }
            } catch (...) {
                toThrow = CurrentExceptionMessage();
            }
            Finished_.Signal();
        }

        const TVector<TVector<float>>& Vectors() const {
            return Vectors_;
        }

        const TVector<ui32>& Ids() {
            return Ids_;
        }

        void Wait() {
            Finished_.WaitI();
            if (toThrow) {
                throw yexception() << toThrow;
            }
        }

    private:
        NYT::IClientPtr Client_;
        ui32 Dimension_ = 0;
        NYT::TRichYPath ClustersTablePath_;
        ui32 From_ = 0;
        ui32 To_ = 0;

        TVector<TVector<float>> Vectors_;
        TVector<ui32> Ids_;

        TManualEvent Finished_;
        TString toThrow;
    };

    std::tuple<TBuffer, TBuffer, TBuffer> BuildHnswIndex(TParams params) {
        AutoDetectEmbeddingSize(params);
        NJupiter::TTable<NHnswKMeansProto::TCluster> clustersTable(params.Client, params.ClustersTable);

        const i64 signedRecordsCount = clustersTable.GetRecordsCount();
        Y_ENSURE(signedRecordsCount > 0);
        Y_ENSURE(static_cast<ui64>(signedRecordsCount) < Max<ui32>());

        const ui32 recordsCount = static_cast<ui32>(signedRecordsCount);

        const ui32 tasksCountLimit = Min<ui32>(Max<ui32>(recordsCount / 1000, 1u), 512);
        const ui32 threads = Min<ui32>(tasksCountLimit, 32);
        const ui32 rowsPerTask = (recordsCount + tasksCountLimit - 1) / tasksCountLimit;

        TVector<THolder<TLoadClusterVectorsTask>> tasks;
        THolder<IThreadPool> queue = MakeHolder<TThreadPool>();
        queue->Start(threads, tasksCountLimit);

        for (ui32 fromRow = 0; fromRow < recordsCount; fromRow += rowsPerTask) {
            tasks.push_back(MakeHolder<TLoadClusterVectorsTask>(
                params.Client,
                *params.Dimension,
                params.ClustersTable,
                fromRow,
                Min(fromRow + rowsPerTask, recordsCount)));

            Y_ENSURE(queue->Add(tasks.back().Get()));
        }

        const ui64 vectorsBufferSize = static_cast<ui64>(recordsCount) * (*params.Dimension) * sizeof(float);
        Y_ENSURE(vectorsBufferSize < Max<size_t>());

        TBuffer vectorsBuffer;
        vectorsBuffer.Resize(vectorsBufferSize);

        TBuffer idBuffer;
        idBuffer.Resize(recordsCount * sizeof(ui32));

        ui64 counter = 0;
        for (THolder<TLoadClusterVectorsTask>& task : tasks) {
            task->Wait();
            memcpy(idBuffer.data() + counter * sizeof(ui32), task->Ids().data(), task->Ids().size() * sizeof(ui32));

            for (const TVector<float>& vector : task->Vectors()) {
                memcpy(
                    vectorsBuffer.data() + counter * (*params.Dimension) * sizeof(float),
                    vector.data(),
                    (*params.Dimension) * sizeof(float));
                counter++;
            }
            task.Reset();
        }

        TBufferOutput out;
        TBuffer vectorsCopy = vectorsBuffer; // TODO: optimize deep copy performed here
        NHnsw::TDenseVectorItemStorage<float> itemStorage(TBlob::FromBuffer(vectorsBuffer), *params.Dimension);
        NHnsw::THnswIndexData indexData = NHnsw::BuildIndex<
            TFixedDimensionL2SqrDistance,
            TFixedDimensionL2SqrDistance::TResult,
            TFixedDimensionL2SqrDistance::TLess,
            NHnsw::TDenseVectorItemStorage<float>>(
                params.HnswOpts,
                itemStorage,
                TFixedDimensionL2SqrDistance(*params.Dimension));

        NHnsw::WriteIndex(indexData, out);

        return std::make_tuple(std::move(out.Buffer()), std::move(vectorsCopy), std::move(idBuffer));
    }

    void AutoDetectEmbeddingSize(TParams& params) {
        if (!params.Dimension) {
            NYT::TTableReaderPtr<NHnswKMeansProto::TItem> input = params.Client->CreateTableReader<NHnswKMeansProto::TItem>(params.ItemsTable);
            Y_ENSURE(input->IsValid());
            params.Dimension = input->GetRow().GetEmbedding().ValuesSize();
        }
    }

    class TAssignMapper: public NYT::IMapper<
        NYT::TTableReader<NHnswKMeansProto::TItem>,
        NYT::TTableWriter<NHnswKMeansProto::TItem>>
    {
    public:
        TAssignMapper() = default;

        TAssignMapper(ui32 dimension, ui32 searchNeighborhoodSize, TBuffer indexBlob, TBuffer vectorsBlob, TBuffer idsBlob)
            : Dimension_(dimension)
            , SearchNeighborhoodSize_(searchNeighborhoodSize)
            , IndexBlob_(std::move(indexBlob))
            , VectorsBlob_(std::move(vectorsBlob))
            , IdsBlob_(std::move(idsBlob))
        {
        }

        Y_SAVELOAD_JOB(Dimension_, SearchNeighborhoodSize_, IndexBlob_, VectorsBlob_, IdsBlob_)

        void Start(NYT::TTableWriter<NHnswKMeansProto::TItem>* /*output*/) override {
            HnswIndex_ = MakeHolder<NHnsw::THnswDenseVectorIndex<float>>(TBlob::FromBuffer(IndexBlob_), TBlob::FromBuffer(VectorsBlob_), Dimension_);
        }

        void Do(NYT::TTableReader<NHnswKMeansProto::TItem>* input, NYT::TTableWriter<NHnswKMeansProto::TItem>* output) override {
            TConstArrayRef<ui32> ids = TConstArrayRef<ui32>((ui32*)(IdsBlob_.data()), (ui32*)(IdsBlob_.data() + IdsBlob_.size()));
            for (; input->IsValid(); input->Next()) {
                NHnswKMeansProto::TItem vector = input->MoveRow();
                Y_ENSURE(vector.GetEmbedding().ValuesSize() == Dimension_);

                using TNeighbor = NHnsw::THnswDenseVectorIndex<float>::TNeighbor<TFixedDimensionL2SqrDistance::TResult>;

                TVector<TNeighbor> neighbors = HnswIndex_->GetNearestNeighbors<TFixedDimensionL2SqrDistance>(
                    vector.GetEmbedding().GetValues().data(),
                    1,
                    SearchNeighborhoodSize_,
                    TFixedDimensionL2SqrDistance(Dimension_));

                Y_ENSURE(neighbors.size() == 1);

                const size_t index = ids[neighbors[0].Id];

                vector.SetClusterId(index);
                output->AddRow(vector);
            }
        }

    private:
        ui32 Dimension_ = 0;
        ui32 SearchNeighborhoodSize_ = 0;
        THolder<NHnsw::THnswDenseVectorIndex<float>> HnswIndex_;
        TBuffer IndexBlob_;
        TBuffer VectorsBlob_;
        TBuffer IdsBlob_;
    };

    REGISTER_MAPPER(TAssignMapper)

    double CalcTotalEnergy(const TParams& params) {
        double energy = 0;
        auto reader = params.Client->CreateTableReader<NYT::TNode>(NYT::TRichYPath(params.ClustersTable).Columns({"energy"}));
        for (; reader->IsValid(); reader->Next()) {
            energy += reader->GetRow().At("energy").AsDouble();
        }
        return energy;
    }

    void PrintIterationInfo(ui32 iteration, double energy) {
        Cout << "Iteration: " << iteration << Endl;
        Cout << "Energy: " << energy << Endl;
    }



    void AssignClusters(TParams params, NYT::TMapOperationSpec spec) {
        AutoDetectEmbeddingSize(params);
        auto [index, vectors, ids] = BuildHnswIndex(params);

        params.Client->Map(
            spec
                .AddInput<NHnswKMeansProto::TItem>(params.ItemsTable)
                .AddOutput<NHnswKMeansProto::TItem>(params.ItemsTable),
            new TAssignMapper(
                *params.Dimension,
                params.NearestClusterSearchNeighborhoodSize,
                std::move(index),
                std::move(vectors),
                std::move(ids)));
    }

    static constexpr ui64 MinApproxUsedMemory = 2_GBs;

    void BuildClustering(ui32 clusters, ui32 iters, TParams params, NYT::TMapReduceOperationSpec spec) {
        AutoDetectEmbeddingSize(params);

        if (!params.Continue) {
            NYT::TMapReduceOperationSpec mrspec = spec;
            params.Client->MapReduce(
                mrspec
                    .AddInput<NHnswKMeansProto::TItem>(params.ItemsTable)
                    .AddOutput<NHnswKMeansProto::TCluster>(params.ClustersTable)
                    .ReduceBy({ "id" }),
                new TSearchNearestClusterMapper(*params.Dimension, clusters),
                new TClustersReducer);
        }

        for (ui32 i = 0; i < iters; i++) {
            auto [index, vectors, _] = BuildHnswIndex(params);

            NYT::TUserJobSpec userJobSpec;
            userJobSpec.MemoryLimit(
                Max<ui64>(
                    MinApproxUsedMemory +
                    index.Size() + vectors.Size(),
                    MinApproxUsedMemory
                )
            );
            spec.MapperSpec(userJobSpec);

            NYT::TMapReduceOperationSpec mrspec = spec;
            params.Client->MapReduce(
                mrspec
                    .AddInput<NHnswKMeansProto::TItem>(params.ItemsTable)
                    .AddOutput<NHnswKMeansProto::TCluster>(params.ClustersTable)
                    .ReduceBy({ "id" }),
                new TSearchNearestClusterMapper(
                        *params.Dimension,
                        params.NearestClusterSearchNeighborhoodSize,
                        std::move(index),
                        std::move(vectors)),
                new TClustersReducer);

            if (params.Verbose) {
                PrintIterationInfo(i, CalcTotalEnergy(params));
            }
        }
    }
}
