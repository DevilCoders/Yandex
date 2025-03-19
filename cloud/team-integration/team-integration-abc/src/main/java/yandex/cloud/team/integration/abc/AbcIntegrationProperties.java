package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.Nullable;

public record AbcIntegrationProperties(
        @Nullable String abcServiceCloudOrganizationId
) {
}
