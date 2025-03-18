package ru.yandex.ci.storage.reader.spring;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.spring.BadgeEventsConfig;
import ru.yandex.ci.storage.reader.check.listeners.ArcanumCheckEventsListener;
import ru.yandex.ci.storage.reader.export.TestsExporter;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;

@Configuration
@Import({
        StorageReaderCacheConfig.class,
        StorageReaderPropertiesTestConfig.class,
        BadgeEventsConfig.class
})
public class StorageReaderTestConfig {

    @Bean
    public ArcanumCheckEventsListener arcanumCheckEventsListener() {
        return Mockito.mock(ArcanumCheckEventsListener.class);
    }

    @Bean
    public ShardInMessageWriter shardInMessageWriter() {
        return Mockito.mock(ShardInMessageWriter.class);
    }

    @Bean
    public TestsExporter exporter() {
        return Mockito.mock(TestsExporter.class);
    }
}
