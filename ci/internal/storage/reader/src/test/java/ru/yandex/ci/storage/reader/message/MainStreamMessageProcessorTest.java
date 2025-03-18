package ru.yandex.ci.storage.reader.message;

import java.time.Duration;
import java.util.List;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.reader.StorageReaderYdbTestBase;
import ru.yandex.ci.storage.reader.check.CheckAnalysisService;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.check.listeners.ArcanumCheckEventsListener;
import ru.yandex.ci.storage.reader.export.TestsExporter;
import ru.yandex.ci.storage.reader.message.main.MainStreamMessageProcessor;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.main.TaskResultDistributor;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.storage.reader.other.MetricAggregationService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class MainStreamMessageProcessorTest extends StorageReaderYdbTestBase {

    @Autowired
    ArcanumCheckEventsListener arcanumCheckEventsListener;

    @Autowired
    ShardInMessageWriter shardInMessageWriter;

    @Autowired
    TestsExporter exporter;

    @Test
    public void test() {
        db.currentOrTx(() -> {
            db.checks().save(sampleCheck);
            db.checkIterations().save(sampleIteration);
            db.checkTasks().save(sampleTask);
        });

        var shardInWriter = mock(ShardInMessageWriter.class);
        var shardOutWriter = mock(ShardOutMessageWriter.class);
        var readerStatistics = new ReaderStatistics(
                mock(MainStreamStatistics.class),
                mock(ShardOutStreamStatistics.class),
                mock(EventsStreamStatistics.class),
                meterRegistry
        );

        var requirementsService = mock(RequirementsService.class);
        var checkService = new ReaderCheckService(
                requirementsService, readerCache, readerStatistics, db, badgeEventsProducer,
                mock(MetricAggregationService.class)
        );

        var checkFinalizationService = new CheckFinalizationService(
                readerCache,
                readerStatistics,
                db,
                List.of(arcanumCheckEventsListener),
                shardInMessageWriter,
                mock(BazingaTaskManager.class),
                mock(CheckAnalysisService.class),
                checkService
        );

        var shardingSettings = ShardingSettings.DEFAULT;
        var chunkDistributor = new ChunkDistributor(db, shardingSettings, 1);
        var distributor = new TaskResultDistributor(chunkDistributor, readerStatistics, 2);

        var processor = new MainStreamMessageProcessor(
                checkService, checkFinalizationService, distributor, shardInWriter, shardOutWriter,
                exporter, readerStatistics, readerCache,
                shardingSettings, Duration.ofSeconds(0)
        );

        var protoTaskId = CheckProtoMappers.toProtoCheckTaskId(sampleTask.getId());
        processor.process(
                List.of(
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setTraceStage(
                                        Common.TraceStage.newBuilder().build()
                                )
                                .build(),
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setAutocheckTestResults(
                                        TaskMessages.AutocheckTestResults.newBuilder()
                                                .addResults(
                                                        TaskMessages.AutocheckTestResult.newBuilder()
                                                                .setId(
                                                                        TaskMessages.AutocheckTestId.newBuilder()
                                                                                .setHid(1L)
                                                                                .setSuiteHid(1L)
                                                                                .setToolchain("a")
                                                                                .build()
                                                                )
                                                                .setOldId("test")
                                                                .setOldSuiteId("a")
                                                                .setResultType(Common.ResultType.RT_BUILD)
                                                                .build()
                                                )
                                                .build()
                                )
                                .build()
                ),
                timeTraceService.createTrace(""));

        processor.process(
                List.of(
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setFinished(
                                        Actions.Finished.newBuilder().build()
                                )
                                .build(),
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setAutocheckTestResults(
                                        TaskMessages.AutocheckTestResults.newBuilder()
                                                .addResults(
                                                        TaskMessages.AutocheckTestResult.newBuilder()
                                                                .setId(
                                                                        TaskMessages.AutocheckTestId.newBuilder()
                                                                                .setHid(2L)
                                                                                .setSuiteHid(2L)
                                                                                .setToolchain("b")
                                                                                .build()
                                                                )
                                                                .setOldId("test2")
                                                                .setOldSuiteId("b")
                                                                .setResultType(Common.ResultType.RT_STYLE_CHECK)
                                                                .build()
                                                )
                                                .build()
                                )
                                .build(),
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setTestTypeFinished(
                                        Actions.TestTypeFinished.newBuilder()
                                                .setTestType(Actions.TestType.CONFIGURE)
                                                .build()
                                )
                                .build(),
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setTestTypeFinished(
                                        Actions.TestTypeFinished.newBuilder()
                                                .setTestType(Actions.TestType.BUILD)
                                                .build()
                                )
                                .build(),
                        TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(protoTaskId)
                                .setPartition(0)
                                .setTestTypeSizeFinished(
                                        Actions.TestTypeSizeFinished.newBuilder()
                                                .setTestType(Actions.TestType.TEST)
                                                .setSize(Actions.TestTypeSizeFinished.Size.MEDIUM)
                                                .build()
                                )
                                .build()
                ),
                timeTraceService.createTrace(""));

        db.currentOrReadOnly(() -> {
            var all = db.checkTaskStatistics().readTable().collect(Collectors.toList());
            assertThat(all).hasSize(1);
            var statistics = all.get(0).getStatistics();
            assertThat(statistics.getNumberOfMessagesByType()).hasSize(5);
            assertThat(statistics.getAffectedChunks().getChunks()).hasSize(2);
            assertThat(statistics.getFinishedChunkTypes()).hasSize(3);
        });
    }
}
