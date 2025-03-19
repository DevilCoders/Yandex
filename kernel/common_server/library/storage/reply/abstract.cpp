#include "abstract.h"
#include <util/string/join.h>
#include <library/cpp/logger/global/global.h>

namespace NCS {
    namespace NStorage {
        void IPackedRecordsSet::AddRow(const TVector<TStringBuf>& values) {
            CHECK_WITH_LOG(++CurrentRecord <= RecordsCount);
            CHECK_WITH_LOG(ColumnsCount == values.size());
        }

        NCS::NStorage::IPackedRecordsSet& TPackedRecordsSet::StoreOriginalData(THolder<IOriginalContainer>&& data) {
            CHECK_WITH_LOG(!OriginalData);
            OriginalData = std::move(data);
            return *this;
        }

        void TPanicOnFailPolicy::OnFail(const TVector<TStringBuf>& values) {
            S_FAIL_LOG << Header << ": " << JoinSeq(", ", values) << Endl;
        }

    }
}
