#pragma once

#include "table_field_viewer.h"

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/rt_background/processes/common/yt.h>
#include <kernel/common_server/rt_background/settings.h>

namespace NCS {

    using TExecutionContext = IRTBackgroundProcess::TExecutionContext;

    class IRTDumper: public TYtProcessTraits {
    public:
        using TPtr = TAtomicSharedPtr<IRTDumper>;
        using TFactory = NObjectFactory::TParametrizedObjectFactory<IRTDumper, TString>;

        bool ProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer, const bool dryRun) const noexcept;
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const = 0;
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
        virtual NJson::TJsonValue SerializeToJson() const = 0;

        virtual TString GetClassName() const = 0;

        virtual ~IRTDumper() = default;

    protected:
        virtual bool DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                      const NYT::TTableSchema& tableYtSchema, const bool dryRun) const = 0;
    };

    class TRTDumperContainer: public TInterfaceContainer<IRTDumper> {
    private:
        using TBase = TInterfaceContainer<IRTDumper>;

    public:
        using TBase::TBase;
    };

    namespace NHelpers {
        void DumpToYtNode(const NStorage::TTableRecordWT& tableRecord, NYT::TNode& recordNode,
                          const ITableFieldViewer& tableViewer);
    }

} // namespace NCS
