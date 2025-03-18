package ru.yandex.ci.storage.reader.message;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageTestUtils;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.reader.message.main.CheckTaskMessage;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.main.TaskResultDistributor;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class TaskResultDistributorTest extends CommonTestBase {
    CheckTaskEntity.Id taskId = StorageTestUtils.createTaskId(StorageTestUtils.createIterationId());

    @Test
    void oneChunk() {
        var shardingSettings = ShardingSettings.DEFAULT;
        var chunkDistributor = new ChunkDistributor(mock(CiStorageDb.class), shardingSettings, 1);
        var distributor = new TaskResultDistributor(
                chunkDistributor,
                new ReaderStatistics(
                        mock(MainStreamStatistics.class),
                        mock(ShardOutStreamStatistics.class),
                        mock(EventsStreamStatistics.class),
                        null
                ),
                2
        );

        var suiteId = "suite_id";

        var testResults = TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .build()
                )
                .setOldSuiteId(suiteId)
                .build();

        var fullTaskId = CheckTaskOuterClass.FullTaskId.newBuilder()
                .setIterationId(CheckIteration.IterationId.newBuilder().setCheckId("1"))
                .build();

        var result = distributor.distribute(
                List.of(
                        new CheckTaskMessage(
                                taskId,
                                Common.CheckTaskType.CTT_AUTOCHECK,
                                TaskMessages.TaskMessage.newBuilder()
                                        .setFullTaskId(fullTaskId)
                                        .setAutocheckTestResults(
                                                TaskMessages.AutocheckTestResults
                                                        .newBuilder()
                                                        .addResults(testResults)
                                        )
                                        .build(),
                                0,
                                shardingSettings
                        ),
                        new CheckTaskMessage(
                                taskId,
                                Common.CheckTaskType.CTT_AUTOCHECK,
                                TaskMessages.TaskMessage.newBuilder()
                                        .setFullTaskId(fullTaskId)
                                        .setAutocheckTestResults(
                                                TaskMessages.AutocheckTestResults
                                                        .newBuilder()
                                                        .addResults(testResults)
                                        )
                                        .build(),
                                0,
                                shardingSettings
                        ),
                        new CheckTaskMessage(
                                taskId,
                                Common.CheckTaskType.CTT_AUTOCHECK,
                                TaskMessages.TaskMessage.newBuilder()
                                        .setFullTaskId(fullTaskId)
                                        .setAutocheckTestResults(
                                                TaskMessages.AutocheckTestResults
                                                        .newBuilder()
                                                        .addResults(testResults)
                                        )
                                        .build(),
                                0,
                                shardingSettings
                        )
                )
        );

        assertThat(result.getNumberOfResults()).isEqualTo(3);
        assertThat(result.getNumberOfChunks()).isEqualTo(1);
        assertThat(result.getMessagesToWrite().size()).isEqualTo(3);
    }
}
