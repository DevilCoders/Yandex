package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;

public interface CreateCloudServiceFacade {

    @NotNull Operation createByAbcServiceId(
            long abcServiceId
    );

    @NotNull Operation createByAbcServiceSlug(
            @NotNull String abcServiceSlug
    );

    @NotNull Operation createByAbcdFolderId(
            @NotNull String abcdFolderId
    );

}
