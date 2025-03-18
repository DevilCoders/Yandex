package ru.yandex.ci.core.proto;

import com.google.common.base.Preconditions;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;

public class CiCoreProtoMappers {
    private CiCoreProtoMappers() {

    }

    public static FlowFullId toFlowFullId(Common.FlowProcessId flowProcessId) {
        return new FlowFullId(flowProcessId.getDir(), flowProcessId.getId());
    }

    public static Common.FlowProcessId toProtoFlowProcessId(CiProcessId processId) {
        Preconditions.checkArgument(processId.getType() == CiProcessId.Type.FLOW);
        return Common.FlowProcessId.newBuilder()
                .setDir(processId.getDir())
                .setId(processId.getSubId())
                .build();
    }

    public static Common.FlowProcessId toProtoFlowProcessId(FlowFullId flowFullId) {
        return Common.FlowProcessId.newBuilder()
                .setDir(flowFullId.getDir())
                .setId(flowFullId.getId())
                .build();
    }

    public static Common.OrderedArcRevision toProtoOrderedArcRevision(OrderedArcRevision revision) {
        return Common.OrderedArcRevision.newBuilder()
                .setBranch(revision.getBranch().asString())
                .setHash(revision.getCommitId())
                .setNumber(revision.getNumber())
                .setPullRequestId(revision.getPullRequestId())
                .build();
    }

    public static boolean hasOrderedArcRevision(Common.OrderedArcRevision revision) {
        return !revision.getHash().isEmpty();
    }

    public static OrderedArcRevision toOrderedArcRevision(Common.OrderedArcRevision revision) {
        return OrderedArcRevision.fromHash(
                revision.getHash(),
                revision.getBranch(),
                revision.getNumber(),
                revision.getPullRequestId()
        );
    }
}
