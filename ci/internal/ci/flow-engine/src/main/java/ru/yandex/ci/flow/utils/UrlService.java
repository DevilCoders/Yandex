package ru.yandex.ci.flow.utils;

import java.nio.file.Path;

import javax.annotation.Nullable;

import com.google.common.base.CharMatcher;
import com.google.common.base.Preconditions;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class UrlService {
    private final String arcanumUrlPrefix;
    private final FlowUrls flowUrls;

    public UrlService(String arcanumUrlPrefix) {
        this.arcanumUrlPrefix = CharMatcher.is('/').trimTrailingFrom(arcanumUrlPrefix);
        this.flowUrls = new FlowUrls(this.arcanumUrlPrefix);
    }

    public String getArcanumUrlPrefix() {
        return arcanumUrlPrefix;
    }

    public String toFlowLaunch(String projectId, LaunchId launchId) {
        return this.flowUrls.toFlowLaunch(
                projectId, launchId.getProcessId().getDir(), launchId.getProcessId().getSubId(), launchId.getNumber()
        );
    }

    public String toReleaseLaunch(String projectId, LaunchId launchId, Version version) {
        return this.flowUrls.toReleasePrefix(projectId) + "/flow"
                + "?dir=" + FlowUrls.encodeParameter(launchId.getProcessId().getDir())
                + "&id=" + FlowUrls.encodeParameter(launchId.getProcessId().getSubId())
                + "&version=" + version.asString();
    }

    @Nullable
    public String toLaunch(FlowLaunchContext launchContext) {
        var projectId = launchContext.getProjectId();
        var launchId = launchContext.getLaunchId();
        return switch (launchId.getProcessId().getType()) {
            case FLOW -> toFlowLaunch(projectId, launchId);
            case RELEASE -> toReleaseLaunch(projectId, launchId, launchContext.getLaunchInfo().getVersion());
            default -> null;
        };
    }

    @Nullable
    public String toLaunch(Launch launch) {
        var projectId = launch.getProject();
        var launchId = launch.getLaunchId();
        return switch (launch.getProcessId().getType()) {
            case FLOW -> toFlowLaunch(projectId, launchId);
            case RELEASE -> toReleaseLaunch(projectId, launchId, launch.getVersion());
            default -> null;
        };
    }

    public String toJobInReleaseLaunch(String projectId,
                                       LaunchId launchId,
                                       Version version,
                                       String jobId,
                                       int jobLaunchNumber) {
        return this.flowUrls.toReleasePrefix(projectId) + "/flow"
                + "?" + this.flowUrls.encodeProcessId(launchId.getProcessId())
                + "&version=" + version.asString()
                + "&selectedJob=" + jobId
                + "&launchNumber=" + jobLaunchNumber;
    }

    public String toJobInFlowLaunch(String projectId, LaunchId launchId, String jobId, int launchNumber) {
        return this.flowUrls.toActionPrefix(projectId) + "/flow"
                + "?" + this.flowUrls.encodeProcessId(launchId.getProcessId())
                + "&number=" + launchId.getNumber()
                + "&selectedJob=" + jobId
                + "&launchNumber=" + launchNumber;
    }

    @Nullable
    public String toJobInLaunch(FlowLaunchContext launchContext) {
        var projectId = launchContext.getProjectId();
        var launchId = launchContext.getLaunchId();
        var jobId = launchContext.getJobId();
        int jobLaunchNumber = launchContext.getJobLaunchNumber();
        return switch (launchId.getProcessId().getType()) {
            case FLOW -> toJobInFlowLaunch(projectId, launchId, jobId, jobLaunchNumber);
            case RELEASE -> toJobInReleaseLaunch(projectId, launchId, launchContext.getLaunchInfo().getVersion(),
                    jobId, jobLaunchNumber);
            default -> null;
        };
    }

    public String toReleaseTimelineUrl(String projectId, CiProcessId processId) {
        Preconditions.checkArgument(processId.getType().isRelease(), "%s should be a release, but it's not", processId);
        return this.flowUrls.toReleasePrefix(projectId) + "/timeline"
                + "?" + this.flowUrls.encodeProcessId(processId);
    }

    public String toDelegationUrl(Path configPath, long pullRequestId) {
        return arcanumUrlPrefix +
                "/gateway/ci/DelegatePrToken"
                + "?pullRequestId=" + pullRequestId
                + "&configDir=" + FlowUrls.encodeParameter(AYamlService.pathToDir(configPath));
    }

    public String getCiSecretDocsUrl() {
        return "https://docs.yandex-team.ru/ci/secret";
    }

    public String toYavSecretKey(String secretKey) {
        return "https://yav.yandex-team.ru/secret/" + secretKey;
    }

    public String toArcCommit(OrderedArcRevision arcRevision) {
        return "%s/arc_vcs/commit/%s".formatted(arcanumUrlPrefix, arcRevision.getCommitId());
    }

    public String toArcFile(OrderedArcRevision arcRevision, Path filePath) {
        return "%s/arcadia/%s?rev=%s".formatted(arcanumUrlPrefix, filePath.toString(), arcRevision.getCommitId());
    }

    public String toSvnRevision(OrderedArcRevision arcRevision) {
        Preconditions.checkArgument(arcRevision.hasSvnRevision(), "Revision %s has no SVN revision", arcRevision);
        return "%s/arc/commit/r%d".formatted(arcanumUrlPrefix, arcRevision.getNumber());
    }

    public String toPullRequest(long pullRequestId) {
        return "%s/review/%d".formatted(arcanumUrlPrefix, pullRequestId);
    }

    public String toCiCardPreview(String checkId) {
        return "%s/ci-card-preview/%s".formatted(arcanumUrlPrefix, checkId);
    }

}
