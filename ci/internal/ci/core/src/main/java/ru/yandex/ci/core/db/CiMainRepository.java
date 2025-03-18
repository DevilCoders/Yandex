package ru.yandex.ci.core.db;

import javax.annotation.Nonnull;

import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import ru.yandex.ci.common.ydb.KikimrRepositoryCi;
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

public class CiMainRepository extends KikimrRepositoryCi {
    public CiMainRepository(@Nonnull KikimrConfig config) {
        super(config);
    }

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new CiMainRepositoryTransaction<>(this, options);
    }

    @NonNullApi
    protected static class CiMainRepositoryTransaction<R extends KikimrRepository>
            extends KikimrRepositoryTransactionCi<R> implements CiMainTables {
        public CiMainRepositoryTransaction(R kikimrRepository, @Nonnull TxOptions options) {
            super(kikimrRepository, options);
        }

        // main

        @Override
        public ConfigStateTable configStates() {
            return new ConfigStateTable(this);
        }

        @Override
        public VirtualConfigStateTable virtualConfigStates() {
            return new VirtualConfigStateTable(this);
        }

        @Override
        public ConfigHistoryTable configHistory() {
            return new ConfigHistoryTable(this);
        }

        @Override
        public ConfigDiscoveryDirTable configDiscoveryDirs() {
            return new ConfigDiscoveryDirTable(this);
        }

        @Override
        public CommitDiscoveryProgressTable commitDiscoveryProgress() {
            return new CommitDiscoveryProgressTable(this);
        }

        @Override
        public DiscoveredCommitTable discoveredCommit() {
            return new DiscoveredCommitTable(this);
        }

        @Override
        public AutoReleaseQueueTable autoReleaseQueue() {
            return new AutoReleaseQueueTable(this);
        }

        @Override
        public AutoReleaseSettingsHistoryTable autoReleaseSettingsHistory() {
            return new AutoReleaseSettingsHistoryTable(this);
        }

        @Override
        public RevisionNumberTable revisionNumbers() {
            return new RevisionNumberTable(this);
        }

        @Override
        public MetricTable metrics() {
            return new MetricTable(this);
        }

        @Override
        public GraphDiscoveryTaskTable graphDiscoveryTaskTable() {
            return new GraphDiscoveryTaskTable(this);
        }

        @Override
        public KeyValueTable keyValue() {
            return new KeyValueTable(this);
        }

        @Override
        public LaunchTable launches() {
            return new LaunchTable(this);
        }

        @Override
        public PostponeLaunchTable postponeLaunches() {
            return new PostponeLaunchTable(this);
        }

        @Override
        public PullRequestDiffSetTable pullRequestDiffSetTable() {
            return new PullRequestDiffSetTable(this);
        }

        @Override
        public FavoriteProjectTable favoriteProjects() {
            return new FavoriteProjectTable(this);
        }

        @Override
        public BranchInfoTable branches() {
            return new BranchInfoTable(this);
        }

        @Override
        public TimelineBranchItemTable timelineBranchItems() {
            return new TimelineBranchItemTable(this);
        }

        @Override
        public CounterTable counter() {
            return new CounterTable(this);
        }

        @Override
        public TimelineTable timeline() {
            return new TimelineTable(this);
        }

        @Override
        public PotatoConfigHealthTable potatoConfigHealth() {
            return new PotatoConfigHealthTable(this);
        }

        @Override
        public VersionsTable versions() {
            return new VersionsTable(this);
        }

        @Override
        public ArcCommitTable arcCommit() {
            return new ArcCommitTable(this);
        }

        @Override
        public AutocheckCommitTable autocheckCommit() {
            return new AutocheckCommitTable(this);
        }

        @Override
        public AbcServiceTable abcServices() {
            return new AbcServiceTable(this);
        }

        // flow

        @Override
        public TaskletMetadataTable taskletMetadata() {
            return new TaskletMetadataTable(this);
        }

        @Override
        public TaskletV2MetadataTable taskletV2Metadata() {
            return new TaskletV2MetadataTable(this);
        }

        @Override
        public RegistryTaskTable registryTask() {
            return new RegistryTaskTable(this);
        }

        // security

        @Override
        public YavTokensTable yavTokensTable() {
            return new YavTokensTable(this);
        }

        // autocheck

        @Override
        public DistBuildPoolNameByACTable distBuildPoolNameByAC() {
            return new DistBuildPoolNameByACTable(this);
        }

        @Override
        public DistBuildPoolNodeTable distBuildPoolNodes() {
            return new DistBuildPoolNodeTable(this);
        }

        // tracker


        @Override
        public TrackerFlowTable trackerFlowTable() {
            return new TrackerFlowTable(this);
        }
    }
}
