package ru.yandex.ci.storage.tests.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.post_processor.logbroker.PostProcessorInReadProcessor;
import ru.yandex.ci.storage.reader.message.events.EventsStreamReadProcessor;
import ru.yandex.ci.storage.reader.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.reader.message.shard.ShardOutReadProcessor;
import ru.yandex.ci.storage.shard.message.ShardInReadProcessor;
import ru.yandex.ci.storage.tests.logbroker.StorageTopic;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.tests.logbroker.TestsMainStreamWriter;

@Configuration
public class StorageTestsLogbrokerWritersConfig {

    @Bean
    public LogbrokerWriterFactory notLocalShardInLogbrokerWriterFactory(TestLogbrokerService logbrokerService) {
        return new TestLogbrokerWriterFactoryImpl(StorageTopic.SHARD_IN, "shard_in", logbrokerService);
    }

    @Bean
    public LogbrokerWriterFactory notLocalShardOutLogbrokerWriterFactory(TestLogbrokerService logbrokerService) {
        return new TestLogbrokerWriterFactoryImpl(StorageTopic.SHARD_OUT, "shard_out", logbrokerService);
    }

    @Bean
    public LogbrokerWriterFactory storageEventLogbrokerWriterFactory(TestLogbrokerService logbrokerService) {
        return new TestLogbrokerWriterFactoryImpl(StorageTopic.EVENTS, "events", logbrokerService);
    }

    @Bean
    public TestsMainStreamWriter testsMainStreamWriter(TestLogbrokerService testLogbrokerService) {
        return new TestsMainStreamWriter(testLogbrokerService);
    }

    @Bean
    public LogbrokerWriterFactory notLocalPostProcessorLogbrokerWriterFactory(TestLogbrokerService logbrokerService) {
        return new TestLogbrokerWriterFactoryImpl(StorageTopic.POST_PROCESSOR, "post_processor_in", logbrokerService);
    }

    @Bean
    public String storageLogbrokerServiceReadersSetter(
            TestLogbrokerService testLogbrokerService,
            MainStreamReadProcessor mainStreamReadProcessor,
            ShardInReadProcessor shardInProcessor,
            ShardOutReadProcessor shardOutReadProcessor,
            EventsStreamReadProcessor eventsStreamReadProcessor,
            PostProcessorInReadProcessor postProcessorInReadProcessor
    ) {
        testLogbrokerService.registerMainConsumer(mainStreamReadProcessor);
        testLogbrokerService.registerShardInConsumer(shardInProcessor);
        testLogbrokerService.registerShardOutConsumer(shardOutReadProcessor);
        testLogbrokerService.registerEventsConsumer(eventsStreamReadProcessor);
        testLogbrokerService.registerPostProcessorInConsumer(postProcessorInReadProcessor);

        return "";
    }

}
