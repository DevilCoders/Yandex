package ru.yandex.ci.flow.engine.runtime;

import java.time.Clock;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.util.CiJson;

@Slf4j
public class JobResourcesValidator {

    public static final String DEFAULT = "default";

    private final CiDb db;
    private final Clock clock;
    private final Map<String, Limits> projectLimits;
    private final Limits defaultLimits;

    public JobResourcesValidator(@Nonnull CiDb db, @Nonnull Clock clock, @Nonnull Map<String, Limits> projectLimits) {
        this.db = db;
        this.clock = clock;
        this.projectLimits = projectLimits;
        this.defaultLimits = projectLimits.get(DEFAULT);
        Preconditions.checkState(defaultLimits != null, "Unable to find %s limits", DEFAULT);

        log.info("Job resource validator using limits: {}", projectLimits);
    }

    public void validateAndSaveResources(String projectId, FlowLaunchId flowLaunchId, StoredResourceContainer resources)
            throws InvalidResourceException {

        var limits = projectLimits.getOrDefault(projectId, defaultLimits);

        log.info("Saving resources for project [{}] using {}", projectId, limits);

        var resourceList = resources.getResources();
        log.info("Total resources to save: {}", resourceList.size());

        // Самый простой способ проверить размер ресурсы - отрендерить его и посмотреть на результат
        // Пока мы ограничены ORM, мы не сможем переиспользовать результат рендеринга
        int artifactSize = limits.getArtifactSize();
        if (artifactSize > 0) {
            for (var resource : resourceList) {
                int size = 0;
                try {
                    size = CiJson.mapper().writeValueAsString(resource.getObject()).length();
                } catch (JsonProcessingException e) {
                    log.error("Unable to verify json size", e); // Не падаем
                }
                if (size > artifactSize) {
                    throw new InvalidResourceException(
                            String.format("Unable to save resources. Max artifact size is %s bytes, " +
                                    "found %s of %s bytes", artifactSize, resource.getResourceType(), size));
                }
            }
        }

        int artifactsPerType = limits.getArtifactsPerType();
        if (artifactsPerType > 0) {
            var resourceMap = resourceList.stream()
                    .collect(Collectors.groupingBy(StoredResource::getResourceType, Collectors.counting()));
            var invalidResources = resourceMap.entrySet().stream()
                    .filter(e -> e.getValue() > artifactsPerType)
                    .map(e -> e.getKey().getMessageName() + "=" + e.getValue())
                    .collect(Collectors.joining(", "));
            if (!invalidResources.isEmpty()) {
                throw new InvalidResourceException(
                        String.format("Unable to save resources. Artifacts per resource limit is %s, found %s",
                                artifactsPerType, invalidResources));
            }
        }

        var resourceClass = ResourceEntity.ResourceClass.DYNAMIC;

        // У нас нет возможности сначала сохранить количество артефактов,
        // а потом в этой же транзакции проверить их количество
        db.currentOrTx(() -> {
            // Так что либо сначала сохраняем все, а потом проверяем - либо сначала проверяем, а потом сохраняем
            // Считаем, что новые артефакты всегда новые (т.е. не может быть такого, что мы их обновляем)
            int totalArtifacts = limits.getTotalArtifacts();
            if (totalArtifacts > 0) {
                int count = resourceList.size() +
                        db.currentOrReadOnly(() ->
                                db.resources().getResourceCount(flowLaunchId, resourceClass));
                if (count > totalArtifacts) {
                    throw new InvalidResourceException(
                            String.format("Unable to save resources. Total artifact count limit is %s, stored %s",
                                    totalArtifacts, count));
                }
            }
            db.resources().saveResources(resources, resourceClass, clock);
        });
    }

    @Value
    public static class Limits {
        int artifactSize; // Максимальный размер одного артефакта в байтах
        int artifactsPerType; // Максимальное количество артефактов одного типа
        int totalArtifacts; // Максимальное количество артефактов
    }
}
