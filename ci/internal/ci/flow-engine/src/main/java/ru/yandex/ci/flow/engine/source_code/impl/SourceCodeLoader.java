package ru.yandex.ci.flow.engine.source_code.impl;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;
import org.springframework.objenesis.Objenesis;
import org.springframework.objenesis.ObjenesisStd;

import ru.yandex.ci.core.common.SourceCodeEntity;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.source_code.SourceCodeProvider;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObjectType;

@Slf4j
public class SourceCodeLoader {
    private SourceCodeLoader() {
        //
    }

    @SuppressWarnings("unchecked")
    public static Map<UUID, SourceCodeObject<?>> loadIdToEntityClassMap(SourceCodeProvider provider) {
        var stopWatch = Stopwatch.createStarted();
        Objenesis objenesis = new ObjenesisStd();
        Map<Class<? extends SourceCodeEntity>, UUID> entityIdByClass = provider.load().stream()
                .collect(
                        Collectors.toMap(
                                Function.identity(),
                                clazz -> Preconditions.checkNotNull(
                                        objenesis.getInstantiatorOf(clazz).newInstance().getSourceCodeId(),
                                        "%s doesn't have getSourceCodeId implementation",
                                        clazz.getName()
                                )
                        )
                );

        log.info("Loaded {} source code entities within {} msec",
                entityIdByClass.size(), stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));

        var resources = entityIdByClass.entrySet().stream()
                .filter(entry -> getObjectType(entry.getKey()).equals(SourceCodeObjectType.RESOURCE))
                .collect(Collectors.toList());
        if (!resources.isEmpty()) {
            log.error("Only protobuf resources are expected, but found something else: {}", resources);
        }

        Map<UUID, JobExecutorObject> jobById = entityIdByClass.entrySet().stream()
                .filter(entry -> getObjectType(entry.getKey()).equals(SourceCodeObjectType.JOB_EXECUTOR))
                .collect(Collectors.toMap(
                        Map.Entry::getValue,
                        entry -> toJobObject(
                                entry.getValue(), (Class<? extends JobExecutor>) entry.getKey())
                        )
                );

        log.info("Total jobs: {}", jobById.size());
        return new HashMap<>(jobById);
    }

    private static JobExecutorObject toJobObject(UUID id, Class<? extends JobExecutor> clazz) {
        List<ProducedResource> produced = ProducedResourcesLoader.load(clazz);
        List<ConsumedResource> consumed = ConsumedResourcesLoader.load(clazz);

        return new JobExecutorObject(id, clazz, produced, consumed);
    }

    private static SourceCodeObjectType getObjectType(Class<? extends SourceCodeEntity> clazz) {
        if (Resource.class.isAssignableFrom(clazz)) {
            return SourceCodeObjectType.RESOURCE;
        }
        if (JobExecutor.class.isAssignableFrom(clazz)) {
            return SourceCodeObjectType.JOB_EXECUTOR;
        }
        throw new RuntimeException("Unknown source code type, class " + clazz.getName());
    }



}
