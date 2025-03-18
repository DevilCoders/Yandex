package ru.yandex.ci.core.db;

import java.util.List;

import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.core.abc.AbcServiceEntity;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.arc.branch.BranchInfoByCommitId;
import ru.yandex.ci.core.autocheck.AutocheckCommit;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.db.autocheck.model.PoolNameByACEntity;
import ru.yandex.ci.core.db.autocheck.model.PoolNodeEntity;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.db.model.ConfigDiscoveryDir;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.FavoriteProject;
import ru.yandex.ci.core.db.model.KeyValue;
import ru.yandex.ci.core.db.model.TrackerFlow;
import ru.yandex.ci.core.db.model.VirtualConfigState;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchByProcessIdAndArcBranch;
import ru.yandex.ci.core.launch.LaunchByProcessIdAndPinned;
import ru.yandex.ci.core.launch.LaunchByProcessIdAndStatus;
import ru.yandex.ci.core.launch.LaunchByProcessIdAndTag;
import ru.yandex.ci.core.launch.LaunchByPullRequest;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.versioning.Versions;
import ru.yandex.ci.core.potato.ConfigHealth;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.RevisionNumber;
import ru.yandex.ci.core.registry.RegistryTask;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineBranchItemByUpdateDate;
import ru.yandex.ci.core.timeline.TimelineItemEntity;
import ru.yandex.ci.ydb.service.CounterEntity;
import ru.yandex.ci.ydb.service.metric.Metric;

public class CiMainEntities {
    @SuppressWarnings("rawtypes")
    public static final List<Class<? extends Entity>> ALL = List.of(
            AutoReleaseQueueItem.class,
            AutoReleaseSettingsHistory.class,
            CommitDiscoveryProgress.class,
            ConfigEntity.class,
            ConfigState.class,
            VirtualConfigState.class,
            ConfigDiscoveryDir.class,
            DiscoveredCommit.class,
            GraphDiscoveryTask.class,
            KeyValue.class,
            Launch.class,
            LaunchByProcessIdAndArcBranch.class,
            LaunchByProcessIdAndPinned.class,
            LaunchByProcessIdAndStatus.class,
            LaunchByProcessIdAndTag.class,
            LaunchByPullRequest.class,
            PostponeLaunch.class,
            PullRequestDiffSet.class,
            PullRequestDiffSet.ByPullRequestId.class,
            RevisionNumber.class,
            FavoriteProject.class,
            Metric.class,
            BranchInfo.class,
            BranchInfoByCommitId.class,
            TimelineBranchItem.class,
            TimelineBranchItemByUpdateDate.class,
            CounterEntity.class,
            TimelineItemEntity.class,
            ConfigHealth.class,
            Versions.class,
            ArcCommit.class,
            ArcCommit.ByParentCommitId.class,
            TaskletMetadata.class,
            TaskletV2Metadata.class,
            YavToken.class,
            PoolNameByACEntity.class,
            PoolNodeEntity.class,
            RegistryTask.class,
            AutocheckCommit.class,
            TrackerFlow.class,
            AbcServiceEntity.class
    );

    private CiMainEntities() {
    }
}
