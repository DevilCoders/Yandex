package yandex.cloud.team.integration.repository;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.repository.EntityNamingOverrides;
import yandex.cloud.iam.repository.operation.OperationNamingOverrides;
import yandex.cloud.iam.repository.task.TaskNamingOverrides;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.ti.repository.TeamIntegrationRepositoryProperties;

public final class TeamIntegrationEntitiesHelper {

    private static final @NotNull EntityNamingOverrides operationEntities = new OperationNamingOverrides(TeamIntegrationRepositoryProperties.TABLE_NAME_PREFIX);
    private static final @NotNull EntityNamingOverrides taskEntities = new TaskNamingOverrides(TeamIntegrationRepositoryProperties.TABLE_NAME_PREFIX);

    @SuppressWarnings("rawtypes")
    public static @NotNull List<Class<? extends Entity>> collectEntities(
            @NotNull Collection<? extends Collection<Class<? extends Entity>>> entities
    ) {
        Set<Class<? extends Entity>> result = entities.stream()
                .flatMap(Collection::stream)
                .collect(Collectors.toSet());
        result.addAll(operationEntities.getEntityTypes());
        result.addAll(taskEntities.getEntityTypes());
        return List.copyOf(result);
    }


    private TeamIntegrationEntitiesHelper() {
    }

}
