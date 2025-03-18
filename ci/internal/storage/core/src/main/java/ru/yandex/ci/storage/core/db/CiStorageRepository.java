package ru.yandex.ci.storage.core.db;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.experimental.Delegate;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import yandex.cloud.repository.BaseDb;
import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import yandex.cloud.repository.kikimr.statement.Statement;

import ru.yandex.ci.common.temporal.ydb.TemporalRepository;
import ru.yandex.ci.common.temporal.ydb.TemporalTables;
import ru.yandex.ci.common.ydb.KikimrRepositoryCi;
import ru.yandex.ci.core.db.table.KeyValueTable;
import ru.yandex.ci.storage.core.db.model.check.CheckTable;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGeneratorTable;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationTable;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsTable;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskTable;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskTable;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsTable;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchTable;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkTable;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateTable;
import ru.yandex.ci.storage.core.db.model.groups.GroupTable;
import ru.yandex.ci.storage.core.db.model.revision.MissingRevisionTable;
import ru.yandex.ci.storage.core.db.model.revision.RevisionTable;
import ru.yandex.ci.storage.core.db.model.settings.SettingsTable;
import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckTable;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatisticsTable;
import ru.yandex.ci.storage.core.db.model.suite_restart.SuiteRestartTable;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultTable;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashTable;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantTable;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffTable;
import ru.yandex.ci.storage.core.db.model.test_diff.index.TestDiffsBySuiteTable;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchTable;
import ru.yandex.ci.storage.core.db.model.test_mute.OldCiMuteActionTable;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteTable;
import ru.yandex.ci.storage.core.db.model.test_revision.TestImportantRevisionTable;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionTable;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsTable;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusTable;
import ru.yandex.ci.storage.core.db.model.yt.YtExportTable;
import ru.yandex.ci.ydb.service.CounterTable;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDbTables;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageRepository;
import ru.yandex.lang.NonNullApi;

public class CiStorageRepository extends KikimrRepositoryCi {
    private static final Logger QUERY_LOG = LoggerFactory.getLogger("ydb-queries");

    public CiStorageRepository(@Nonnull KikimrConfig config) {
        super(config);
    }

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new CiStorageDbTransaction<>(this, options);
    }

    @SuppressWarnings("MissingOverride")
    @NonNullApi
    public static class CiStorageDbTransaction<R extends KikimrRepository>
            extends KikimrRepositoryTransactionCi<R> implements CiStorageDbTables {

        private final BazingaStorageRepository.BazingaStorageDbTablesProxy bazingaProxy;
        private final TemporalRepository.DbTransactionProxy temporalProxy;

        public CiStorageDbTransaction(R kikimrRepository, @Nonnull TxOptions options) {
            super(kikimrRepository, options);
            this.bazingaProxy = new BazingaStorageRepository.BazingaStorageDbTablesProxy(this);
            this.temporalProxy = new TemporalRepository.DbTransactionProxy(this);
        }

        @Override
        public TestDiffByHashTable testDiffsByHash() {
            return new TestDiffByHashTable(this);
        }

        @Override
        public TestDiffsBySuiteTable testDiffsBySuite() {
            return new TestDiffsBySuiteTable(this);
        }

        @Override
        public TestDiffTable testDiffs() {
            return new TestDiffTable(this);
        }

        @Override
        public TestDiffImportantTable importantTestDiffs() {
            return new TestDiffImportantTable(this);
        }

        @Override
        public TestResultTable testResults() {
            return new TestResultTable(this);
        }

        @Override
        public SettingsTable settings() {
            return new SettingsTable(this);
        }

        @Override
        public TestRevisionTable testRevision() {
            return new TestRevisionTable(this);
        }

        @Override
        public TestImportantRevisionTable testImportantRevision() {
            return new TestImportantRevisionTable(this);
        }

        @Override
        public TestMuteTable testMutes() {
            return new TestMuteTable(this);
        }

        @Override
        public OldCiMuteActionTable oldCiMuteActionTable() {
            return new OldCiMuteActionTable(this);
        }

        @Override
        public CheckTaskTable checkTasks() {
            return new CheckTaskTable(this);
        }

        @Override
        public LargeTaskTable largeTasks() {
            return new LargeTaskTable(this);
        }

        @Override
        public CheckMergeRequirementsTable checkMergeRequirements() {
            return new CheckMergeRequirementsTable(this);
        }

        @Override
        public CheckTaskStatisticsTable checkTaskStatistics() {
            return new CheckTaskStatisticsTable(this);
        }

        @Override
        public CheckTextSearchTable checkTextSearch() {
            return new CheckTextSearchTable(this);
        }

        @Override
        public TestStatusTable tests() {
            return new TestStatusTable(this);
        }

        @Override
        public TestStatisticsTable testStatistics() {
            return new TestStatisticsTable(this);
        }

        @Override
        public TestLaunchTable testLaunches() {
            return new TestLaunchTable(this);
        }

        @Override
        public SkippedCheckTable skippedChecks() {
            return new SkippedCheckTable(this);
        }

        @Override
        public YtExportTable ytExport() {
            return new YtExportTable(this);
        }

        @Override
        public CounterTable sequences() {
            return new CounterTable(this);
        }

        @Override
        public RevisionTable revisions() {
            return new RevisionTable(this);
        }

        @Override
        public MissingRevisionTable missingRevisions() {
            return new MissingRevisionTable(this);
        }

        @Override
        public GroupTable groups() {
            return new GroupTable(this);
        }

        @Override
        public CheckTable checks() {
            return new CheckTable(this);
        }

        @Override
        public CheckIdGeneratorTable checkIds() {
            return new CheckIdGeneratorTable(this);
        }

        @Override
        public CheckIterationTable checkIterations() {
            return new CheckIterationTable(this);
        }

        @Override
        public ChunkAggregateTable chunkAggregates() {
            return new ChunkAggregateTable(this);
        }

        @Override
        public ChunkTable chunks() {
            return new ChunkTable(this);
        }

        @Override
        public StorageStatisticsTable storageStatistics() {
            return new StorageStatisticsTable(this);
        }

        @Override
        public SuiteRestartTable suiteRestarts() {
            return new SuiteRestartTable(this);
        }

        @Override
        public KeyValueTable keyValues() {
            return new KeyValueTable(this);
        }

        @Delegate(excludes = BaseDb.class)
        public BazingaStorageDbTables bazingaProxy() {
            return this.bazingaProxy;
        }

        @Delegate(excludes = BaseDb.class)
        public TemporalTables temporalProxy() {
            return this.temporalProxy;
        }

        @Override
        public <PARAMS, RESULT> List<RESULT> execute(Statement<PARAMS, RESULT> statement, PARAMS params) {
            if (statement.getQueryType() == Statement.QueryType.SELECT && QUERY_LOG.isTraceEnabled()) {
                var query = statement.getQuery("", YqlVersion.YQLv1);
                QUERY_LOG.trace("Read query:\n{}", query);
                return super.execute(statement, params);
            }

            return super.execute(statement, params);
        }
    }
}
