package ru.yandex.ci.storage.core.db;

import java.util.Collection;
import java.util.List;

import one.util.streamex.StreamEx;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.temporal.ydb.TemporalEntities;
import ru.yandex.ci.core.db.model.KeyValue;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGeneratorEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.groups.GroupEntity;
import ru.yandex.ci.storage.core.db.model.revision.MissingRevisionEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.settings.SettingsEntity;
import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckEntity;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.suite_restart.SuiteRestartEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.index.TestDiffBySuiteEntity;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.OldCiMuteActionEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.TestImportantRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntity;
import ru.yandex.ci.ydb.service.CounterEntity;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageEntities;

public class CiStorageEntities {

    private static final List<Class<? extends Entity>> STORAGE_ENTITIES = List.of(
            CheckEntity.class,
            CheckIdGeneratorEntity.class,
            CheckIterationEntity.class,
            CheckTaskEntity.class,
            LargeTaskEntity.class,
            CheckTaskStatisticsEntity.class,
            CheckTextSearchEntity.class,
            CheckMergeRequirementsEntity.class,
            ChunkAggregateEntity.class,
            ChunkEntity.class,
            OldCiMuteActionEntity.class,
            SettingsEntity.class,
            SkippedCheckEntity.class,
            StorageStatisticsEntity.class,
            SuiteRestartEntity.class,
            TestDiffByHashEntity.class,
            TestDiffEntity.class,
            TestDiffImportantEntity.class,
            TestDiffBySuiteEntity.class,
            TestStatisticsEntity.class,
            TestRevisionEntity.class,
            TestMuteEntity.class,
            TestResultEntity.class,
            YtExportEntity.class,
            TestStatusEntity.class,
            CounterEntity.class,
            TestLaunchEntity.class,
            TestImportantRevisionEntity.class,
            RevisionEntity.class,
            MissingRevisionEntity.class,
            GroupEntity.class,
            KeyValue.class
    );
    private static final List<Class<? extends Entity>> ALL_ENTITIES;

    static {
        ALL_ENTITIES = StreamEx.of(
                STORAGE_ENTITIES,
                BazingaStorageEntities.ALL,
                TemporalEntities.ALL
        ).flatMap(Collection::stream).toImmutableList();
    }

    private CiStorageEntities() {
    }

    public static List<Class<? extends Entity>> getAllEntities() {
        return ALL_ENTITIES;
    }

    public static List<Class<? extends Entity>> getStorageEntities() {
        return STORAGE_ENTITIES;
    }
}
