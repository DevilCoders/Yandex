package ru.yandex.ci.engine.event;

import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowDescription;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.proto.converter.CiContextConverter;
import ru.yandex.ci.core.timeline.ReleaseCommit;
import ru.yandex.ci.job.ArcRevision;
import ru.yandex.ci.job.Commit;
import ru.yandex.ci.job.CommitOffset;
import ru.yandex.ci.job.ConfigInfo;
import ru.yandex.ci.job.Release;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public final class LaunchMappers {

    private LaunchMappers() {
    }

    public static Event.Builder launchContext(Launch launch) {
        ArcRevision targetRevision = CiContextConverter.arcRevision(launch.getVcsInfo().getRevision());

        var builder = Event.newBuilder()
                .setLaunchId(launch.getId().getProcessId())
                .setLaunchNumber(launch.getId().getLaunchNumber())
                .setTargetRevision(targetRevision)
                .setConfigInfo(configInfo(launch))
                .addAllTags(launch.getTags())
                .setFlowId(launch.getFlowInfo().getFlowId().getId());

        Optional.ofNullable(launch.getVcsInfo().getPreviousRevision())
                .map(CiContextConverter::arcRevision)
                .ifPresent(builder::setPreviousRevision);

        Optional.ofNullable(launch.getVcsInfo().getPullRequestInfo())
                .map(CiContextConverter::pullRequestInfo)
                .ifPresent(builder::setLaunchPullRequestInfo);

        Optional.ofNullable(launch.getFlowLaunchId())
                .ifPresent(builder::setFlowLaunchId);

        Optional.of(launch.getFlowInfo())
                .map(LaunchFlowInfo::getFlowDescription)
                .map(LaunchFlowDescription::getFlowType)
                .ifPresent(builder::setFlowType);

        return builder;
    }

    public static ConfigInfo configInfo(Launch launch) {
        var processId = launch.getProcessId();
        return ConfigInfo.newBuilder()
                .setDir(processId.getDir())
                .setId(processId.getSubId())
                .setPath(processId.getPath().toString()) // TODO: deprecated
                .build();
    }

    public static CommitOffset commitOffset(ReleaseCommit last) {
        return CommitOffset.newBuilder()
                .setBranch(last.getRevision().getBranch().asString())
                .setNumber(last.getRevision().getNumber())
                .build();
    }

    public static Commit commit(ReleaseCommit releaseCommit) {
        var commit = releaseCommit.getCommit();
        var revision = releaseCommit.getRevision();
        return CiContextConverter.commit(commit, revision);
    }

    public static Release release(Launch release, @Nullable String url) {
        var builder = Release.newBuilder()
                .setTitle(release.getTitle())
                .setVersion(CiContextConverter.versionToProto(release.getVersion()));
        if (url != null) {
            builder.setCiUrl(url);
        }
        return builder.build();
    }
}
