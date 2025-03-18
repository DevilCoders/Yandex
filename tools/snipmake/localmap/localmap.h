#pragma once

#include <mapreduce/lib/all.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    class TLimitedInput {
    public:
        TLimitedInput(NMR::TServer& server, const TString& table, const TInstant& deadline, ui64 maxCount)
          : Deadline(deadline)
          , MaxCount(maxCount)
          , Client(server)
          , Input(Client, table)
          , InputIter(Input.Begin())
        {
        }

        bool IsValid() {
            return MaxCount > 0 && Now() < Deadline && InputIter.IsValid();
        }

        const NMR::TTableIterator& Get() const {
            return InputIter;
        }

        inline TLimitedInput& operator++() {
            ++InputIter;
            --MaxCount;
            return *this;
        }

    private:
        const TInstant Deadline;
        i64 MaxCount;
        NMR::TClient Client;
        NMR::TTable Input;
        NMR::TTableIterator InputIter;
    };

    class TBatchedUpdate {
    private:
        struct TBatch {
            NMR::TClient Client;
            TVector<TSimpleSharedPtr<NMR::TUpdate>> Updates;
            const TInstant Deadline;
            ui64 MaxCount;

            TBatch(NMR::TServer& server, const TVector<NMR::TUpdateTable>& tables, const TInstant& deadline, ui64 maxCount)
              : Client(server)
              , Updates(tables.size())
              , Deadline(deadline)
              , MaxCount(maxCount)
            {
                for (size_t i = 0; i < Updates.size(); ++i) {
                    Updates[i].Reset(new NMR::TUpdate(Client, tables[i]));
                }
            }
        };

    private:
        void Rotate() {
            Batch.Reset(new TBatch(Server, Tables, Now() + RotationTime, RotationCount));
            if (!Rotated) {
                for (NMR::TUpdateTable& table : Tables) {
                    if (table.Mode == NMR::UM_REPLACE) {
                        table.Mode = NMR::UM_APPEND;
                    } else if (table.Mode == NMR::UM_SORTED) {
                        table.Mode = NMR::UM_APPEND_SORTED;
                    }
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
        TBatchedUpdate(NMR::TServer& server, const NMR::TUpdateTable& table, const TDuration& rotationTime = TDuration::Minutes(15), ui64 rotationCount = 100000)
          : Server(server)
          , Tables(1, table)
          , RotationTime(rotationTime)
          , RotationCount(rotationCount)
        {
            Y_VERIFY(RotationCount > 0);
            Rotate();
        }

        TBatchedUpdate(NMR::TServer& server, const TVector<NMR::TUpdateTable>& tables, const TDuration& rotationTime = TDuration::Minutes(15), ui64 rotationCount = 100000)
          : Server(server)
          , Tables(tables)
          , RotationTime(rotationTime)
          , RotationCount(rotationCount)
        {
            Y_VERIFY(RotationCount > 0);
            Rotate();
        }

        NMR::TUpdate& Get(size_t index = 0) {
            MaybeRotate();
            return *Batch->Updates.at(index).Get();
        }

        const TVector<TSimpleSharedPtr<NMR::TUpdate>>& GetAll() {
            MaybeRotate();
            return Batch->Updates;
        }

        bool ShouldRotate() const {
            return Batch->MaxCount <= 0 || Batch->Deadline < Now();
        }

        void ForceRotate() {
            Rotate();
        }

        NMR::TUpdate& GetNoRotate(size_t index = 0) {
            TickCount();
            return *Batch->Updates.at(index).Get();
        }

        const TVector<TSimpleSharedPtr<NMR::TUpdate>>& GetAllNoRotate() {
            TickCount();
            return Batch->Updates;
        }

    private:
        NMR::TServer& Server;
        bool Rotated = false;
        TVector<NMR::TUpdateTable> Tables;
        const TDuration RotationTime;
        const ui64 RotationCount;
        THolder<TBatch> Batch;
    };
}
