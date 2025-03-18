package ru.yandex.ci.engine.notification.xiva;

import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.jvm.ExecutorServiceMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.xiva.XivaClient;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.engine.proto.ProtoMappers;

@Slf4j
@SuppressWarnings("FutureReturnValueIgnored")
public class XivaNotifierImpl implements AutoCloseable, XivaNotifier {

    private static final String METRICS_GROUP = "xiva-notifier";

    private final XivaClient xivaClient;
    private final ExecutorService executor;
    private final Counter sendSuccess;
    private final Counter sendErrors;

    public XivaNotifierImpl(XivaClient xivaClient, MeterRegistry meterRegistry) {
        this.xivaClient = xivaClient;
        this.executor = ExecutorServiceMetrics.monitor(
                meterRegistry,
                Executors.newSingleThreadExecutor(
                        new ThreadFactoryBuilder().setNameFormat("xiva-notifier-%d").build()
                ),
                "xiva_notifier_executor"
        );
        this.sendSuccess = Counter.builder(METRICS_GROUP)
                .tag("method", "send")
                .tag("result", "success")
                .register(meterRegistry);
        this.sendErrors = Counter.builder(METRICS_GROUP)
                .tag("method", "send")
                .tag("result", "error")
                .register(meterRegistry);
    }

    @Override
    public void onLaunchStateChanged(@Nonnull Launch updatedLaunch, @Nullable Launch oldLaunch) {
        sendOnLaunchStateChanged(updatedLaunch, oldLaunch);
    }

    @VisibleForTesting
    CompletableFuture<Void> sendOnLaunchStateChanged(@Nonnull Launch updatedLaunch, @Nullable Launch oldLaunch) {
        var futures = new ArrayList<CompletableFuture<Void>>();
        ProjectStatisticsChangedEvent.onLaunchStateChanged(updatedLaunch, oldLaunch)
                .map(this::send)
                .ifPresent(futures::add);
        ReleasesTimelineChangedEvent.onLaunchStateChanged(updatedLaunch, oldLaunch)
                .map(this::send)
                .ifPresent(futures::add);
        return CompletableFuture.allOf(futures.toArray(CompletableFuture[]::new));
    }

    private CompletableFuture<Void> send(XivaBaseEvent event) {
        var topic = event.getTopic();
        var request = event.toXivaSendRequest();
        var shortTitle = event.getType().name().toLowerCase();

        // TODO: replace `CompletableFuture.runAsync` after CI-2975 that should
        //  add support for CompletableFuture to http-client-base
        return CompletableFuture.runAsync(() -> xivaClient.send(topic, request, shortTitle), executor)
                .whenComplete((result, exception) -> {
                    if (exception == null) {
                        sendSuccess.increment();
                    } else {
                        sendErrors.increment();
                        log.error("Failed to send: {} {} {}", topic, shortTitle, request);
                    }
                });
    }

    @Override
    public Common.XivaSubscription toXivaSubscription(@Nonnull XivaBaseEvent event) {
        return ProtoMappers.toXivaSubscription(
                getServiceWithTags(event),
                event.getTopic()
        );
    }

    private String getServiceWithTags(XivaBaseEvent event) {
        if (event.getTags().isEmpty()) {
            return xivaClient.getService();
        }
        return xivaClient.getService() + ":" + String.join("+", event.getTags());
    }

    @Override
    @SuppressWarnings("ResultOfMethodCallIgnored")
    public void close() throws Exception {
        executor.shutdown();
        executor.awaitTermination(15, TimeUnit.SECONDS);
    }

}
