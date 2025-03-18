package ru.yandex.ci.storage.core.logbroker;

import java.util.concurrent.atomic.AtomicReference;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerWriterImpl;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;

@Slf4j
@RequiredArgsConstructor
public class LogbrokerWriterFactoryImpl implements LogbrokerWriterFactory {

    @Nonnull
    private final String topic;

    @Nonnull
    private final String streamName;

    @Nonnull
    private final LogbrokerClientFactory logbrokerClientFactory;

    @Nonnull
    private final LogbrokerCredentialsProvider credentialsProvider;

    @Override
    public LogbrokerWriterImpl create(int partition) {
        var ref = new AtomicReference<LogbrokerWriterImpl>();

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> ref.set(createWriterInternal(partition)),
                (r) -> log.error("Failed to create writer", r)
        );

        return ref.get();
    }

    private LogbrokerWriterImpl createWriterInternal(int partition) {
        var partitionGroup = partition + 1;
        log.info(
                "Creating produced for partition: {}, partition group: {}",
                partition, partitionGroup
        );

        var sourceId = "%s_%d_p_%d".formatted(
                streamName,
                Math.abs(HostnameUtils.getShortHostname().hashCode()),
                partition
        );

        return new LogbrokerWriterImpl(
                topic,
                sourceId,
                CompressionCodec.ZSTD,
                partitionGroup,
                this.logbrokerClientFactory,
                this.credentialsProvider
        );
    }

    @Override
    public String getStreamName() {
        return streamName;
    }
}
