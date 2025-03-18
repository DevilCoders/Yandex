package ru.yandex.ci.storage.reader.spring;

import java.time.Clock;
import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.core.spring.stream.MainStreamCoreConfig;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.export.TestsExporter;
import ru.yandex.ci.storage.reader.message.ReaderStreamConsumer;
import ru.yandex.ci.storage.reader.message.main.MainStreamMessageProcessor;
import ru.yandex.ci.storage.reader.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.main.TaskResultDistributor;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;


@Configuration
@Import({
        StorageReaderCoreConfig.class,
        MainStreamCoreConfig.class,
        ShardInStreamConfig.class,
        ShardOutStreamConfig.class,
        ExportConfig.class
})
public class MainStreamConfig {

    @Bean
    public TaskResultDistributor taskResultDistributor(
            ChunkDistributor chunkDistributor,
            ReaderStatistics readerStatistics,
            @Value("${storage.taskResultDistributor.maxNumberOfResultsPerWrite}") int maxNumberOfResultsPerWrite
    ) {
        return new TaskResultDistributor(chunkDistributor, readerStatistics, maxNumberOfResultsPerWrite);
    }

    @Bean
    public MainStreamMessageProcessor taskMessageProcessor(
            ReaderCheckService readerCheckService,
            CheckFinalizationService checkFinalizationService,
            TaskResultDistributor distributor,
            ShardInMessageWriter shardInMessageWriter,
            ShardOutMessageWriter shardOutMessageWriter,
            TestsExporter exporter,
            ReaderStatistics readerStatistics,
            ReaderCache readerCache,
            ShardingSettings shardingSettings,
            @Value("${storage.taskMessageProcessor.waitForRegistration}") Duration waitForRegistration
    ) {
        return new MainStreamMessageProcessor(
                readerCheckService, checkFinalizationService, distributor, shardInMessageWriter,
                shardOutMessageWriter, exporter,
                readerStatistics, readerCache, shardingSettings, waitForRegistration
        );
    }

    @Bean
    public MainStreamReadProcessor mainStreamReadProcessor(
            TimeTraceService timeTraceService,
            MainStreamMessageProcessor mainStreamMessageProcessor,
            ReaderCache cache,
            ReaderStatistics statistics
    ) {
        return new MainStreamReadProcessor(
                timeTraceService,
                mainStreamMessageProcessor,
                cache.settings(),
                statistics.getMain()
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamListener mainStreamListener(
            MainStreamReadProcessor messageProcessor,
            ReaderStatistics statistics,
            Clock clock,
            @Value("${storage.mainStreamListener.readQueues}") int readQueues,
            @Value("${storage.mainStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${storage.mainStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "main",
                messageProcessor,
                statistics.getMain(),
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public ReaderStreamConsumer mainStreamConsumer(
            LogbrokerConfiguration mainStreamLogbrokerConfiguration,
            LogbrokerStreamListener mainStreamListener,
            ReaderCache cache,
            @Value("${storage.mainStreamConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${storage.mainStreamConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ReaderStreamConsumer(
                mainStreamLogbrokerConfiguration,
                mainStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }

}
