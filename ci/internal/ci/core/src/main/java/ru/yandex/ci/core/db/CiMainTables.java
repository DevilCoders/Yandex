package ru.yandex.ci.core.db;

import ru.yandex.ci.core.abc.AbcServiceTable;
import ru.yandex.ci.core.arc.ArcCommitTable;
import ru.yandex.ci.core.arc.branch.BranchInfoTable;
import ru.yandex.ci.core.autocheck.AutocheckCommitTable;
import ru.yandex.ci.core.db.autocheck.table.DistBuildPoolNameByACTable;
import ru.yandex.ci.core.db.autocheck.table.DistBuildPoolNodeTable;
import ru.yandex.ci.core.db.table.AutoReleaseQueueTable;
import ru.yandex.ci.core.db.table.AutoReleaseSettingsHistoryTable;
import ru.yandex.ci.core.db.table.ConfigDiscoveryDirTable;
import ru.yandex.ci.core.db.table.ConfigHistoryTable;
import ru.yandex.ci.core.db.table.ConfigStateTable;
import ru.yandex.ci.core.db.table.FavoriteProjectTable;
import ru.yandex.ci.core.db.table.KeyValueTable;
import ru.yandex.ci.core.db.table.TrackerFlowTable;
import ru.yandex.ci.core.db.table.VirtualConfigStateTable;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgressTable;
import ru.yandex.ci.core.discovery.DiscoveredCommitTable;
import ru.yandex.ci.core.discovery.GraphDiscoveryTaskTable;
import ru.yandex.ci.core.launch.LaunchTable;
import ru.yandex.ci.core.launch.PostponeLaunchTable;
import ru.yandex.ci.core.launch.versioning.VersionsTable;
import ru.yandex.ci.core.potato.PotatoConfigHealthTable;
import ru.yandex.ci.core.pr.PullRequestDiffSetTable;
import ru.yandex.ci.core.pr.RevisionNumberTable;
import ru.yandex.ci.core.registry.RegistryTaskTable;
import ru.yandex.ci.core.security.YavTokensTable;
import ru.yandex.ci.core.tasklet.TaskletMetadataTable;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataTable;
import ru.yandex.ci.core.timeline.TimelineBranchItemTable;
import ru.yandex.ci.core.timeline.TimelineTable;
import ru.yandex.ci.ydb.service.CounterTable;
import ru.yandex.ci.ydb.service.metric.MetricTable;
import ru.yandex.lang.NonNullApi;

/**
 * Все таблицы, которые могут работать в рамках одной транзакции, должны быть сюда включены.
 */
@NonNullApi
public interface CiMainTables {

    // main

    AutoReleaseQueueTable autoReleaseQueue();

    AutoReleaseSettingsHistoryTable autoReleaseSettingsHistory();

    ConfigStateTable configStates();

    VirtualConfigStateTable virtualConfigStates();

    ConfigHistoryTable configHistory();

    ConfigDiscoveryDirTable configDiscoveryDirs();

    CommitDiscoveryProgressTable commitDiscoveryProgress();

    DiscoveredCommitTable discoveredCommit();

    KeyValueTable keyValue();

    LaunchTable launches();

    PostponeLaunchTable postponeLaunches();

    PullRequestDiffSetTable pullRequestDiffSetTable();

    RevisionNumberTable revisionNumbers();

    FavoriteProjectTable favoriteProjects();

    BranchInfoTable branches();

    CounterTable counter();

    TimelineTable timeline();

    TimelineBranchItemTable timelineBranchItems();

    PotatoConfigHealthTable potatoConfigHealth();

    MetricTable metrics();

    GraphDiscoveryTaskTable graphDiscoveryTaskTable();

    VersionsTable versions();

    ArcCommitTable arcCommit();

    AutocheckCommitTable autocheckCommit();

    AbcServiceTable abcServices();

    // flow

    TaskletMetadataTable taskletMetadata();

    TaskletV2MetadataTable taskletV2Metadata();

    RegistryTaskTable registryTask();

    // security

    YavTokensTable yavTokensTable();

    // autocheck

    DistBuildPoolNameByACTable distBuildPoolNameByAC();

    DistBuildPoolNodeTable distBuildPoolNodes();

    // tracker

    TrackerFlowTable trackerFlowTable();
}
