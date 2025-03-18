package ru.yandex.ci.storage.core.db;

import ru.yandex.ci.common.temporal.ydb.TemporalTables;
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

public interface CiStorageDbTables extends BazingaStorageDbTables, TemporalTables {
    CheckIdGeneratorTable checkIds();

    CheckIterationTable checkIterations();

    CheckTable checks();

    CheckTaskStatisticsTable checkTaskStatistics();

    CheckTextSearchTable checkTextSearch();

    CheckTaskTable checkTasks();

    LargeTaskTable largeTasks();

    CheckMergeRequirementsTable checkMergeRequirements();

    ChunkAggregateTable chunkAggregates();

    ChunkTable chunks();

    OldCiMuteActionTable oldCiMuteActionTable();

    SettingsTable settings();

    StorageStatisticsTable storageStatistics();

    SuiteRestartTable suiteRestarts();

    TestDiffByHashTable testDiffsByHash();

    TestDiffsBySuiteTable testDiffsBySuite();

    TestDiffImportantTable importantTestDiffs();

    TestDiffTable testDiffs();

    TestRevisionTable testRevision();

    TestImportantRevisionTable testImportantRevision();

    TestMuteTable testMutes();

    TestResultTable testResults();

    TestStatusTable tests();

    TestStatisticsTable testStatistics();

    TestLaunchTable testLaunches();

    SkippedCheckTable skippedChecks();

    YtExportTable ytExport();

    CounterTable sequences();

    RevisionTable revisions();

    MissingRevisionTable missingRevisions();

    GroupTable groups();

    KeyValueTable keyValues();
}
