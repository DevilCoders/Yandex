package ru.yandex.ci.storage.reader.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriterImpl;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;

@Configuration
@Import({LogbrokerConfig.class, StorageReaderCacheConfig.class})
public class ShardInStreamConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ShardInMessageWriter notLocalShardInMessageWriter(
            @Value("${storage.notLocalShardInMessageWriter.numberOfPartitions}") int numberOfPartitions,
            LogbrokerWriterFactory notLocalShardInLogbrokerWriterFactory
    ) {
        return new ShardInMessageWriterImpl(numberOfPartitions, meterRegistry, notLocalShardInLogbrokerWriterFactory);
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerWriterFactory notLocalShardInLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.notLocalShardInMessageWriter.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "shard_in",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

}
