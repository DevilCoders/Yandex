package ru.yandex.ci.flow.engine.runtime;

import java.time.Duration;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Supplier;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.bazinga.BazingaJobScheduler;
import ru.yandex.ci.flow.engine.runtime.temporal.TemporalJobScheduler;

@AllArgsConstructor
@Slf4j
public class DispatcherJobScheduler implements JobScheduler {
    public static final String KV_NAMESPACE = DispatcherJobScheduler.class.getSimpleName() + "_useTemporal";
    public static final String DEFAULT_KEY = "__default__";
    public static final String LARGE_KEY = "__large__";

    private final TemporalJobScheduler temporalJobScheduler;
    private final BazingaJobScheduler bazingaJobScheduler;
    private final CiDb db;
    private final AtomicInteger bazingaLaunchCount = new AtomicInteger();
    private final AtomicInteger temporalLaunchCount = new AtomicInteger();

    private final Cache<String, Boolean> cache = CacheBuilder.newBuilder()
            .expireAfterWrite(Duration.ofMinutes(2))
            .build();

    @Override
    public void scheduleFlow(String project, LaunchId launchId, FullJobLaunchId fullJobLaunchId) {

        boolean useTemporal = isTemporalEnabled(project, launchId);

        if (useTemporal) {
            log.info("Using temporalJobScheduler for project: {}, job {}", project, fullJobLaunchId);
            temporalJobScheduler.scheduleFlow(project, launchId, fullJobLaunchId);
            temporalLaunchCount.incrementAndGet();
        } else {
            log.info("Using bazingaJobScheduler for project: {}, job {}", project, fullJobLaunchId);
            bazingaJobScheduler.scheduleFlow(project, launchId, fullJobLaunchId);
            bazingaLaunchCount.incrementAndGet();
        }

    }

    @Override
    public void scheduleStageRecalc(String project, FlowLaunchId flowLaunchId) {
        //Temporal not supported yet
        bazingaJobScheduler.scheduleStageRecalc(project, flowLaunchId);
    }


    private boolean isTemporalEnabled(String project, LaunchId launchId) {
        return db.readOnly(
                () -> isTemporalEnabledForProcess(project, launchId.getProcessId())
        );
    }

    private boolean isTemporalEnabledForProcess(String project, CiProcessId processId) {

        var virtualType = VirtualCiProcessId.VirtualType.of(processId);
        if (virtualType != null) {
            return isTemporalEnabledForLargeTests();
        } else {
            return getBoolean(processId.asString(), () -> isTemporalEnabledForProject(project));
        }
    }

    private boolean isTemporalEnabledForLargeTests() {
        return getBoolean(LARGE_KEY, this::isTemporalEnabledByDefault);
    }

    private boolean isTemporalEnabledForProject(String project) {
        return getBoolean(project, this::isTemporalEnabledByDefault);
    }

    private boolean isTemporalEnabledByDefault() {
        return getBoolean(DEFAULT_KEY, () -> false);
    }

    public boolean getBoolean(String key, Supplier<Boolean> defaultValueSupplier) {
        try {
            return cache.get(key, () -> db.keyValue().getBoolean(KV_NAMESPACE, key, defaultValueSupplier));
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    public int getBazingaLaunchCount() {
        return bazingaLaunchCount.get();
    }

    public int getTemporalLaunchCount() {
        return temporalLaunchCount.get();
    }

    @VisibleForTesting
    public void flush() {
        bazingaLaunchCount.set(0);
        temporalLaunchCount.set(0);
        cache.invalidateAll();
    }
}
