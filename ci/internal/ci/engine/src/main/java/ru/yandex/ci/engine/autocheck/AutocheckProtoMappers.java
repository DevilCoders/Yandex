package ru.yandex.ci.engine.autocheck;

import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.storage.core.Common;

public class AutocheckProtoMappers {

    private AutocheckProtoMappers() {
    }

    public static Common.OrderedRevision toProtoOrderedRevision(OrderedArcRevision revision) {
        return Common.OrderedRevision.newBuilder()
                .setBranch(revision.getBranch().asString())
                .setRevision(revision.getCommitId())
                .setRevisionNumber(revision.getNumber())
                .build();
    }

    public static OrderedArcRevision toOrderedRevision(Common.OrderedRevision revision) {
        return OrderedArcRevision.fromHash(
                revision.getRevision(),
                revision.getBranch(),
                revision.getRevisionNumber(),
                -1
        );
    }

    public static ru.yandex.ci.api.proto.Common.CommitId toProtoCommitId(Common.OrderedRevision revision) {
        return ru.yandex.ci.api.proto.Common.CommitId.newBuilder()
                .setCommitId(revision.getRevision())
                .build();
    }

    public static Autocheck.AutocheckConfiguration toProtoAutocheckConfiguration(AutocheckConfigurationConfig config,
                                                                                 CommitId revision) {
        return toProtoAutocheckConfiguration(config, revision, null);
    }

    public static Autocheck.AutocheckConfiguration toProtoAutocheckConfiguration(AutocheckConfigurationConfig config,
                                                                                 CommitId revision,
                                                                                 @Nullable Integer svnRevision) {
        var builder = Autocheck.AutocheckConfiguration.newBuilder()
                .setId(config.getId())
                .setPartitionCount(config.getPartitions().getCount())
                .setRevision(ProtoMappers.toCommitId(revision));
        Optional.ofNullable(svnRevision).ifPresent(builder::setSvnRevision);
        return builder.build();
    }

}
