package ru.yandex.ci.core.logbroker;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;
import ru.yandex.kikimr.persqueue.producer.AsyncProducer;
import ru.yandex.kikimr.persqueue.producer.async.AsyncProducerConfig;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public class LogbrokerWriterImpl implements LogbrokerWriter {

    private static final Logger log = LoggerFactory.getLogger(LogbrokerWriterImpl.class);

    private final LogbrokerClientFactory logbrokerClientFactory;
    private final AsyncProducerConfig asyncProducerConfig;
    private final String sourceId;

    @Nullable
    private AsyncProducer asyncProducer;

    public LogbrokerWriterImpl(
            String topic,
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider
    ) throws UnknownHostException {
        this(
                topic,
                InetAddress.getLocalHost().getHostName(),
                CompressionCodec.RAW,
                null,
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

    public LogbrokerWriterImpl(
            String topic,
            String sourceId,
            CompressionCodec codec,
            @Nullable Integer partitionGroup,
            LogbrokerClientFactory logbrokerClientFactory,
            LogbrokerCredentialsProvider credentialsProvider
    ) {
        Preconditions.checkState(
                partitionGroup == null || partitionGroup > 0,
                "Partition must be greater then zero or null"
        );

        this.logbrokerClientFactory = logbrokerClientFactory;
        this.sourceId = sourceId;

        this.asyncProducerConfig = AsyncProducerConfig.builder(topic, sourceId.getBytes(StandardCharsets.UTF_8))
                .setCredentialsProvider(credentialsProvider)
                .setGroup(partitionGroup == null ? 0 : partitionGroup)
                .setCodec(codec)
                .build();
    }

    @Override
    public CompletableFuture<ProducerWriteResponse> write(byte[] data) {
        return getOrCreateAsyncProducer().write(data);
    }

    @Override
    public String getSourceId() {
        return sourceId;
    }

    private synchronized AsyncProducer getOrCreateAsyncProducer() {
        var producer = this.asyncProducer;
        if (producer == null) {
            producer = createAndInitAsyncProducer();
            this.asyncProducer = producer;
        }

        return producer;
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    private AsyncProducer createAndInitAsyncProducer() {
        AsyncProducer producer = null;

        try {
            producer = this.logbrokerClientFactory.asyncProducer(asyncProducerConfig);

            producer.closeFuture()
                    .whenComplete((result, exception) -> {
                        if (exception != null) {
                            log.warn("Current async producer was closed unexpectedly: {}", exception.getMessage());
                        }
                        closeAsyncProducer();
                    });

            var init = producer.init().get();

            log.info(
                    "Initialized new async producer with sessionId={}, partition={}",
                    init.getSessionId(), init.getPartition()
            );

            return producer;
        } catch (InterruptedException e) {
            if (producer != null) {
                producer.close();
            }
            throw new RuntimeException("Interrupted", e);
        } catch (ExecutionException e) {
            throw new RuntimeException(e.getCause());
        }
    }

    private synchronized void closeAsyncProducer() {
        var producer = this.asyncProducer;
        this.asyncProducer = null;

        if (producer != null) {
            producer.close();
        }
    }
}
