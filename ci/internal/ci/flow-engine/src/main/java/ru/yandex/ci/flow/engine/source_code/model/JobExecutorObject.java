package ru.yandex.ci.flow.engine.source_code.model;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.UUID;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

public class JobExecutorObject extends SourceCodeObject<JobExecutor> implements ResourceConsumer {
    private final String title;
    private final String description;
    private final String owner;
    private final boolean recommended;
    private final boolean deprecated;

    private final Map<UUID, ProducedResource> producedResources;
    private final Map<UUID, ConsumedResource> consumedResources;

    public JobExecutorObject(
            @Nonnull UUID id,
            @Nullable Class<? extends JobExecutor> clazz,
            @Nonnull List<ProducedResource> produced,
            @Nonnull List<ConsumedResource> consumed
    ) {
        super(id, clazz, SourceCodeObjectType.JOB_EXECUTOR);

        if (clazz != null) {
            ExecutorInfo info = clazz.getAnnotation(ExecutorInfo.class);
            if (info != null) {
                title = StringUtils.isBlank(info.title()) ? clazz.getSimpleName() : info.title();
                description = info.description();
                owner = StringUtils.isBlank(info.owner()) ? ExecutorOwnerExtractor.extract(clazz) : info.owner();
                recommended = info.recommended();
                deprecated = info.deprecated() || clazz.getAnnotation(Deprecated.class) != null;
            } else {
                title = clazz.getSimpleName();
                description = "";
                owner = ExecutorOwnerExtractor.extract(clazz);
                recommended = false;
                deprecated = clazz.getAnnotation(Deprecated.class) != null;
            }
        } else {
            title = "Generic Executor";
            description = "Выполняет задачи любого характера";
            owner = "CI";
            recommended = true;
            deprecated = false;
        }

        this.producedResources = produced.stream().collect(
                Collectors.toMap(r -> r.getResource().getId(), Function.identity())
        );

        this.consumedResources = consumed.stream().collect(
                Collectors.toMap(r -> r.getResource().getId(), Function.identity())
        );
    }

    public String getTitle() {
        return title;
    }

    public String getDescription() {
        return description;
    }

    public String getOwner() {
        return owner;
    }

    @Nonnull
    @Override
    public Class<? extends JobExecutor> getClazz() {
        return Objects.requireNonNull(super.getClazz(), "Class cannot be null");
    }

    public Map<UUID, ProducedResource> getProducedResources() {
        return producedResources;
    }

    @Override
    public Map<UUID, ConsumedResource> getConsumedResources() {
        return consumedResources;
    }

    public boolean getRecommended() {
        return recommended;
    }

    public boolean getDeprecated() {
        return deprecated;
    }

    public Map<JobResourceType, ConsumedResource> getResourceDependencyMap() {
        return this.getConsumedResources().values().stream()
                .collect(Collectors.toMap(
                        r -> r.getResource().getResourceType(),
                        Function.identity())
                );
    }

}
