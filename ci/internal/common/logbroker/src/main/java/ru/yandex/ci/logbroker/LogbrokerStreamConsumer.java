package ru.yandex.ci.logbroker;

import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

import lombok.extern.slf4j.Slf4j;
import org.springframework.context.SmartLifecycle;

import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;
import ru.yandex.kikimr.persqueue.consumer.ConsumerSessionConfig;
import ru.yandex.kikimr.persqueue.consumer.StreamConsumer;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;
import ru.yandex.kikimr.persqueue.consumer.internal.read.ReaderConfig;
import ru.yandex.kikimr.persqueue.consumer.stream.StreamConsumerConfig;
import ru.yandex.kikimr.persqueue.consumer.stream.retry.RetryConfig;

@Slf4j
public class LogbrokerStreamConsumer implements SmartLifecycle {
    private static final int SETTINGS_UPDATE_PERIOD_SECONDS = 30;

    private final StreamConsumer consumer;
    private final StreamListener listener;
    private final boolean onlyNewData;
    private volatile boolean running;
    private volatile boolean consuming;

    public LogbrokerStreamConsumer(
            LogbrokerConfiguration configuration,
            StreamListener listener,
            boolean onlyNewData,
            int maxUncommittedReads
    ) throws InterruptedException {
        this.onlyNewData = onlyNewData;
        var logbrokerClientFactory = new LogbrokerClientFactory(configuration.getProxyHolder().getProxyBalancer());

        // https://wiki.yandex-team.ru/users/rammee/klient-logbroker-kikimr/
        consumer = logbrokerClientFactory.streamConsumer(
                StreamConsumerConfig.builder(
                                configuration.getTopics().get(), configuration.getProperties().getConsumer()
                        )
                        .setCredentialsProvider(configuration.getCredentialsProvider())
                        .configureRetries(RetryConfig.Builder::enable)
                        .configureReader(this::configureReader)
                        // prevent read ahead a lot.
                        .configureCommiter(commiter -> commiter.setMaxUncommittedReads(maxUncommittedReads))
                        .configureSession(this::configureSession)
                        .build()
        );

        this.listener = listener;

        new Timer(this.getClass().getSimpleName()).schedule(
                new TimerTask() {
                    @Override
                    public void run() {
                        try {
                            updateReadSettings();
                        } catch (RuntimeException e) {
                            log.error("Failed to update read settings", e);
                        }
                    }
                },
                TimeUnit.SECONDS.toMillis(SETTINGS_UPDATE_PERIOD_SECONDS),
                TimeUnit.SECONDS.toMillis(SETTINGS_UPDATE_PERIOD_SECONDS)
        );
    }

    protected boolean isReadStopped() {
        return false;
    }

    private void configureSession(ConsumerSessionConfig.Builder builder) {
        builder.setClientSideLocksAllowed(true);
    }

    private void updateReadSettings() {
        if (!running) {
            return;
        }

        var isReadStopped = isReadStopped();
        if (isReadStopped && consuming) {
            log.info("Stopping consume on settings change...");
            consumer.stopConsume();
            consuming = false;
        } else if (!isReadStopped && !consuming) {
            log.info("Starting consume on settings change...");
            consumer.startConsume(listener);
            consuming = true;
        }
    }

    private void configureReader(ReaderConfig.Builder configurator) {
        if (onlyNewData) {
            configurator.setReadDataOnlyAfterTimestampMs(System.currentTimeMillis());
        }
    }

    @Override
    public void start() {
        var isReadStopped = isReadStopped();
        if (isReadStopped) {
            log.info("Consume disabled by settings.");
            running = true;
            return;
        }

        log.info("Starting consume...");
        consumer.startConsume(listener);
        running = true;
        consuming = true;
    }

    @Override
    public void stop() {
        log.info("Stopping consume...");
        consumer.stopConsume();
        running = false;
        consuming = false;
    }

    @Override
    public boolean isRunning() {
        return running;
    }
}
