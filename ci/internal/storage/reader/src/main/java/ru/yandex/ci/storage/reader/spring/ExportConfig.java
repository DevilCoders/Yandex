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
import ru.yandex.ci.storage.core.converter.TskvConverter;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.export.ExportStreamWriter;
import ru.yandex.ci.storage.reader.export.ExportStreamWriterEmptyImpl;
import ru.yandex.ci.storage.reader.export.ExportStreamWriterImpl;
import ru.yandex.ci.storage.reader.export.TestsExporter;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;

@Configuration
@Import({
        StorageReaderCoreConfig.class
})
public class ExportConfig {
    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public TskvConverter tskvConverter() {
        return new TskvConverter();
    }

    @Bean
    @Profile(CiProfile.STABLE_PROFILE)
    public ExportStreamWriter exportStreamWriter(
            @Value("${storage.exportStreamWriter.numberOfPartitions}") int numberOfPartitions,
            TskvConverter tskvConverter,
            LogbrokerWriterFactory exportStreamLogbrokerWriterFactory
    ) {
        return new ExportStreamWriterImpl(
                meterRegistry, numberOfPartitions, tskvConverter, exportStreamLogbrokerWriterFactory
        );
    }

    @Bean
    @Profile(CiProfile.STABLE_PROFILE)
    public LogbrokerWriterFactory exportStreamLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.exportStreamWriter.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "export",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

    @Bean
    @Profile(CiProfile.NOT_STABLE_PROFILE)
    public ExportStreamWriter emptyExportStreamWriter() {
        return new ExportStreamWriterEmptyImpl();
    }

    @Bean
    public TestsExporter exporter(
            ExportStreamWriter exportStreamWriter,
            ReaderCache readerCache
    ) {
        return new TestsExporter(exportStreamWriter, readerCache);
    }
}
