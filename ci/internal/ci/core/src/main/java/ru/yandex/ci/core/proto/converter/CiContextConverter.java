package ru.yandex.ci.core.proto.converter;

import java.util.Optional;

import javax.annotation.Nullable;

import com.google.protobuf.StringValue;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.job.ArcRevision;
import ru.yandex.ci.job.Commit;
import ru.yandex.ci.job.LaunchPullRequestInfo;
import ru.yandex.ci.job.PullRequest;
import ru.yandex.ci.job.PullRequestDiffSet;
import ru.yandex.ci.job.PullRequestIssue;
import ru.yandex.ci.job.PullRequestMergeRequirementId;
import ru.yandex.ci.job.PullRequestVcsInfo;
import ru.yandex.ci.job.Version;

public class CiContextConverter {
    private CiContextConverter() {
    }

    public static ArcRevision toProto(OrderedArcRevision arcRevision) {
        return ArcRevision.newBuilder()
                .setHash(arcRevision.getCommitId())
                .setNumber(arcRevision.getNumber())
                .setPullRequestId(arcRevision.getPullRequestId())
                .buildPartial();
    }

    public static Version versionToProto(ru.yandex.ci.core.launch.versioning.Version version) {
        Version.Builder builder = Version.newBuilder()
                .setFull(version.asString())
                .setMajor(version.getMajor());

        if (version.getMinor() != null) {
            builder.setMinor(version.getMinor());
        }
        return builder.build();
    }


    public static LaunchPullRequestInfo pullRequestInfo(
            ru.yandex.ci.core.launch.LaunchPullRequestInfo pullRequestInfo
    ) {
        var prBuilder = PullRequest.newBuilder()
                .setId(pullRequestInfo.getPullRequestId())
                .setAuthor(pullRequestInfo.getAuthor());
        Optional.ofNullable(pullRequestInfo.getSummary())
                .ifPresent(prBuilder::setSummary);
        Optional.ofNullable(pullRequestInfo.getDescription())
                .ifPresent(prBuilder::setDescription);


        var builder = LaunchPullRequestInfo.newBuilder()
                .setPullRequest(
                        prBuilder.build()
                )
                .setDiffSet(PullRequestDiffSet.newBuilder()
                        .setId(pullRequestInfo.getDiffSetId())
                        .build()
                )
                .setVcsInfo(pullRequestVcsInfo(pullRequestInfo.getVcsInfo()));

        Optional.ofNullable(pullRequestInfo.getRequirementId())
                .map(CiContextConverter::pullRequestMergeRequirementId)
                .ifPresent(builder::setMergeRequirementId);

        for (String issue : pullRequestInfo.getPullRequestIssues()) {
            builder.addIssues(
                    PullRequestIssue.newBuilder()
                            .setId(issue)
                            .build()
            );
        }
        builder.addAllLabels(pullRequestInfo.getPullRequestLabels());
        return builder.build();
    }

    public static PullRequestVcsInfo pullRequestVcsInfo(ru.yandex.ci.core.pr.PullRequestVcsInfo pullRequestVcsInfo) {
        PullRequestVcsInfo.Builder builder = PullRequestVcsInfo.newBuilder()
                .setMergeRevisionHash(pullRequestVcsInfo.getMergeRevision().getCommitId())
                .setUpstreamRevisionHash(pullRequestVcsInfo.getUpstreamRevision().getCommitId())
                .setUpstreamBranch(pullRequestVcsInfo.getUpstreamBranch().toString())
                .setFeatureRevisionHash(pullRequestVcsInfo.getFeatureRevision().getCommitId());
        if (pullRequestVcsInfo.getFeatureBranch() != null) {
            builder.setFeatureBranch(
                    StringValue.of(pullRequestVcsInfo.getFeatureBranch().toString())
            );
        }
        return builder.build();
    }

    public static PullRequestMergeRequirementId pullRequestMergeRequirementId(
            ArcanumMergeRequirementId requirementId
    ) {
        return PullRequestMergeRequirementId.newBuilder()
                .setSystem(requirementId.getSystem())
                .setType(requirementId.getType())
                .build();
    }

    public static Commit commit(@Nullable ArcCommit commit, OrderedArcRevision revision) {
        var builder = Commit.newBuilder()
                .setRevision(arcRevision(revision));

        if (commit == null) {
            return builder.build();
        }

        builder.setAuthor(commit.getAuthor())
                .setDate(ProtoConverter.convert(commit.getCreateTime()))
                .addAllIssues(ArcCommitUtils.parseTickets(commit.getMessage()));

        var requestId = ArcCommitUtils.parsePullRequestId(commit.getMessage());
        if (requestId.isPresent()) {
            builder.setPullRequestId(requestId.get());
            builder.setMessage(ArcCommitUtils.cleanupMessage(commit.getMessage()));
        } else {
            builder.setMessage(commit.getMessage());
        }

        return builder.build();
    }

    public static ArcRevision arcRevision(OrderedArcRevision revision) {
        return ArcRevision.newBuilder()
                .setHash(revision.getCommitId())
                .setNumber(revision.getNumber())
                .setPullRequestId(revision.getPullRequestId())
                .build();
    }
}
