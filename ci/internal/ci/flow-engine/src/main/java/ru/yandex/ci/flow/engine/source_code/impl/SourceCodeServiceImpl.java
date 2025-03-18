package ru.yandex.ci.flow.engine.source_code.impl;

import java.time.Duration;
import java.util.Comparator;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.UUID;
import java.util.function.Supplier;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Suppliers;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationListener;
import org.springframework.context.event.ContextRefreshedEvent;

import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.ExecutorType;
import ru.yandex.ci.flow.engine.runtime.JobExecutorObjectProvider;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.source_code.SourceCodeProvider;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObjectType;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
public class SourceCodeServiceImpl implements SourceCodeService, ApplicationListener<ContextRefreshedEvent> {
    private static final Duration SOURCE_CODE_CACHE_EXPIRATION = Duration.ofHours(1);
    private static final int SOURCE_CODE_CACHE_SIZE = 100_000;

    private final Supplier<Map<UUID, SourceCodeObject<?>>> idToObject;
    private final LoadingCache<ExecutorContext, JobExecutorObject> executorObjects;
    private final ApplicationContext applicationContext;
    private final boolean resolveBeansOnRefresh;

    public SourceCodeServiceImpl(
            SourceCodeProvider provider,
            JobExecutorObjectProvider executorObjectProvider,
            ApplicationContext applicationContext,
            boolean resolveBeansOnRefresh,
            @Nullable MeterRegistry meterRegistry
    ) {
        this.idToObject = Suppliers.memoize(() ->
                SourceCodeLoader.loadIdToEntityClassMap(provider));
        this.executorObjects = CacheBuilder.newBuilder()
                .expireAfterWrite(SOURCE_CODE_CACHE_EXPIRATION)
                .maximumSize(SOURCE_CODE_CACHE_SIZE)
                .recordStats()
                .build(CacheLoader.from(executorObjectProvider::createExecutorObject));
        this.applicationContext = applicationContext;
        this.resolveBeansOnRefresh = resolveBeansOnRefresh;
        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, executorObjects, "executors-cache");
        }
    }

    public void reset() {
        this.executorObjects.invalidateAll();
    }

    @Override
    public void onApplicationEvent(ContextRefreshedEvent event) {
        if (!resolveBeansOnRefresh) {
            log.info("Skip Jobs availability in Spring context on refresh, will check upon first access to beans");
            return; // ---
        }
        log.info("Checking Jobs availability in Spring context on refresh");
        tryResolveBeans();
    }

    @Override
    public Optional<SourceCodeObject<?>> lookup(@Nonnull UUID uuid) {
        return Optional.ofNullable(idToObject.get().get(uuid));
    }

    @Override
    public void validateJobExecutor(ExecutorContext executorContext) {
        if (ExecutorType.selectFor(executorContext) == ExecutorType.CLASS) {
            getJobExecutor(executorContext); // Проверим, что можем найти указанный UUID
        }
    }

    @Override
    public Optional<JobExecutorObject> lookupJobExecutor(ExecutorContext executorContext) {
        try {
            if (ExecutorType.selectFor(executorContext) != ExecutorType.CLASS) {
                return Optional.of(executorObjects.get(executorContext));
            }
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }

        Objects.requireNonNull(executorContext.getInternal(),
                "Internal configuration for executor must be configured");
        var uuid = executorContext.getInternal().getExecutor();
        return lookup(uuid)
                .map(type(SourceCodeObjectType.JOB_EXECUTOR))
                .map(cast(JobExecutorObject.class));
    }

    @Override
    public JobExecutorObject getJobExecutor(@Nonnull ExecutorContext executorContext) {
        return lookupJobExecutor(executorContext)
                .orElseThrow(() -> new RuntimeException("Unable to find executor " + executorContext));
    }


    @Override
    public JobExecutor resolveJobExecutorBean(JobExecutorObject jobExecutorObject) {
        return resolveJobExecutorBeanImpl(jobExecutorObject);
    }

    private void tryResolveBeans() {
        RuntimeException exception = null;
        var lookup = idToObject.get().values().stream()
                .filter(object -> object.getType() == SourceCodeObjectType.JOB_EXECUTOR)
                .map(object -> (JobExecutorObject) object)
                .sorted(Comparator.comparing(s -> s.getClazz().getName()))
                .toList();

        for (var executorObject : lookup) {
            try {
                resolveJobExecutorBeanImpl(executorObject);
                log.info("Job [{}] is available", executorObject.getClazz());
            } catch (Exception e) {
                var message = "Unable to resolve Job " + executorObject.getClazz() + " in Spring context";
                log.error(message);
                if (exception == null) {
                    exception = new RuntimeException(message, e);
                }
            }
        }
        if (exception != null) {
            throw exception;
        }
    }

    private JobExecutor resolveJobExecutorBeanImpl(JobExecutorObject jobExecutorObject) {
        var type = jobExecutorObject.getClazz();

        var map = applicationContext.getBeansOfType(type);
        if (map.isEmpty()) {
            throw new RuntimeException("No bean of " + type + " found in context");
        }
        if (map.size() > 1) {
            var matchedBeans = map.values().stream()
                    .filter(bean -> Objects.equals(type, bean.getClass()))
                    .toList();
            if (matchedBeans.size() == 0) {
                throw new RuntimeException("No bean of " + type + " found in context");
            }
            if (matchedBeans.size() > 1) {
                throw new RuntimeException("Found more than 1 bean of type " + type);
            }
            return matchedBeans.get(0);
        } else {
            return map.values().iterator().next();
        }
    }
}
