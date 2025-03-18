package ru.yandex.ci.flow.engine.model;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;

// TODO: rewrite, make it properly structured
@Value
@Builder
public class FlowLaunchContext {

    @Nonnull
    Instant flowStarted;

    @Nonnull
    Instant jobStarted;

    FlowFullId flowId;

    @Nullable
    OrderedArcRevision previousRevision;
    OrderedArcRevision targetRevision;

    // @Nullable for the backward compatibility, should be @Nonnull
    @Nullable
    ArcCommit targetCommit;

    LaunchId launchId;

    @Nonnull
    LaunchInfo launchInfo;

    FlowLaunchId flowLaunchId;

    String jobId;

    @Nullable
    String jobTitle;

    @Nullable
    String jobDescription;

    int jobLaunchNumber;

    @Nonnull
    YavToken.Id yavTokenUid;

    @Nonnull
    String sandboxOwner;

    @Nullable
    ReleaseVcsInfo vcsReleaseInfo;

    @Nullable
    LaunchPullRequestInfo launchPullRequestInfo;

    RuntimeConfig runtimeConfig;
    RequirementsConfig requirements;

    @Nullable
    String triggeredBy;
    String title;

    @Nonnull
    ArcBranch selectedBranch;

    @Nullable
    JsonObject flowVars;

    @Nullable
    Common.FlowType flowType;

    @Nullable
    Version rollbackToVersion;

    @Nullable
    String manualTriggeredBy;

    @Nullable
    Instant manualTriggeredAt;

    @Nullable
    String jobTriggeredBy;

    @Nonnull
    String projectId;

    @Nullable
    JobAttemptsConfig jobAttemptsConfig;

    public JobInstance.Id toJobInstanceId() {
        return JobInstance.Id.of(getFlowLaunchId().asString(),
                getJobId(),
                getJobLaunchNumber());
    }
}
