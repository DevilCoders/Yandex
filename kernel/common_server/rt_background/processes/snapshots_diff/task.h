#pragma once
#include <kernel/common_server/rt_background/tasks/abstract/object.h>
#include <kernel/common_server/resources/object.h>
#include <kernel/common_server/proto/background.pb.h>

namespace NCS {
    class TSnapshotsDiffState: public NCS::NResources::IProtoResource<NCommonServerProto::TSnapshotsDiffState> {
    private:
        CS_ACCESS(TSnapshotsDiffState, ui32, LastSnapshotId, 0);
        static TFactory::TRegistrator<TSnapshotsDiffState> Registrator;
    protected:
        virtual NCS::NScheme::TScheme DoGetScheme(const IBaseServer& /*server*/) const override {
            NCS::NScheme::TScheme result;
            result.Add<TFSNumeric>("last_snapshot_id").SetRequired(false);
            return result;
        }

        virtual void DoSerializeToProto(NCommonServerProto::TSnapshotsDiffState& proto) const override {
            proto.SetLastSnapshotId(LastSnapshotId);
        }

        virtual bool DoDeserializeFromProto(const NCommonServerProto::TSnapshotsDiffState& proto) override {
            LastSnapshotId = proto.GetLastSnapshotId();
            return true;
        }
    public:
        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        static TString GetTypeName() {
            return "snapshots_diff";
        }
    };

    class TSnapshotsDiffTask: public NCS::NBackground::IRTQueueActionWithResource<NCS::NBackground::IRTQueueActionProto<NCommonServerProto::TSnapshotsDiffTask>, TSnapshotsDiffState> {
    private:
        static TFactory::TRegistrator<TSnapshotsDiffTask> Registrator;
        CSA_DEFAULT(TSnapshotsDiffTask, TString, SnapshotsGroupId);
    protected:
        virtual bool DoDeserializeFromProto(const NCommonServerProto::TSnapshotsDiffTask& proto) override;
        virtual NCommonServerProto::TSnapshotsDiffTask DoSerializeToProto() const override;

        virtual TAtomicSharedPtr<TSnapshotsDiffState> DoExecuteImpl(const NCS::NBackground::IRTQueueTask& /*task*/, TAtomicSharedPtr<TSnapshotsDiffState> state, const IBaseServer& bServer) const override;
    public:
        TSnapshotsDiffTask() = default;

        TSnapshotsDiffTask(const TString& groupId)
            : SnapshotsGroupId(groupId)
        {

        }

        static TString GetTypeName() {
            return "snapshots_diff";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
