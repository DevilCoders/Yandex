#pragma once

#include <mapreduce/lib/all.h>
#include <mapreduce/yt/interface/client.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    class TLimitedInputYT {
    public:
        TLimitedInputYT(NYT::IClientPtr ytClient, const TString& table, const TInstant& deadline, ui64 maxCount)
          : Deadline(deadline)
          , MaxCount(maxCount)
          , InputIter(ytClient->CreateTableReader<NYT::TYaMRRow>(table))
        {
        }

        bool IsValid() {
            return MaxCount > 0 && Now() < Deadline && InputIter->IsValid();
        }

        const NYT::TYaMRRow& GetRow() const {
            return InputIter->GetRow();
        }

        inline TLimitedInputYT& Next() {
            InputIter->Next();
            --MaxCount;
            return *this;
        }

    private:
        const TInstant Deadline;
        i64 MaxCount;
        NYT::TTableReaderPtr<NYT::TYaMRRow> InputIter;
    };

    class TBatchedUpdateYT {
    private:
        struct TBatch {
            NYT::IClientPtr YtClient;
            TVector<NYT::TTableWriterPtr<NYT::TYaMRRow>> Updates;
            const TInstant Deadline;
            i64 MaxCount;

            TBatch(NYT::IClientPtr& ytClient, const TVector<NYT::TRichYPath>& tables, const TInstant& deadline, ui64 maxCount)
              : YtClient(ytClient)
              , Updates(tables.size())
              , Deadline(deadline)
              , MaxCount(maxCount)
            {
                for (size_t i = 0; i < Updates.size(); ++i) {
                    Updates[i] = YtClient->CreateTableWriter<NYT::TYaMRRow>(tables[i]);
                }
            }
        };

    private:
        void Rotate() {
            Batch.Reset(new TBatch(YtClient, Tables, Now() + RotationTime, RotationCount));
            if (!Rotated) {
                for (NYT::TRichYPath& table : Tables) {
                    table.Append(true);
                }
                Rotated = true;
            }
        }

        void MaybeRotate() {
            if (ShouldRotate()) {
                Rotate();
            }
            TickCount();
        }

        void TickCount() {
            --Batch->MaxCount;
        }

    public:
        TBatchedUpdateYT(NYT::IClientPtr ytClient, const NYT::TRichYPath& table, const TDuration& rotationTime = TDuration::Minutes(15), ui64 rotationCount = 100000)
          : YtClient(ytClient)
          , Tables(1, table)
          , RotationTime(rotationTime)
          , RotationCount(rotationCount)
        {
            Y_VERIFY(RotationCount > 0);
            Rotate();
        }

        TBatchedUpdateYT(NYT::IClientPtr ytClient, const TVector<NYT::TRichYPath>& tables, const TDuration& rotationTime = TDuration::Minutes(15), ui64 rotationCount = 100000)
          : YtClient(ytClient)
          , Tables(tables)
          , RotationTime(rotationTime)
          , RotationCount(rotationCount)
        {
            Y_VERIFY(RotationCount > 0);
            Rotate();
        }

        NYT::TTableWriterPtr<NYT::TYaMRRow>& Get(size_t index = 0) {
            MaybeRotate();
            return Batch->Updates.at(index);
        }

        bool ShouldRotate() const {
            return Batch->MaxCount <= 0 || Batch->Deadline < Now();
        }

        void ForceRotate() {
            Rotate();
        }

        NYT::TTableWriterPtr<NYT::TYaMRRow>& GetNoRotate(size_t index = 0) {
            TickCount();
            return Batch->Updates.at(index);
        }

    private:
        NYT::IClientPtr YtClient;
        bool Rotated = false;
        TVector<NYT::TRichYPath> Tables;
        const TDuration RotationTime;
        const ui64 RotationCount;
        THolder<TBatch> Batch;
    };
}
