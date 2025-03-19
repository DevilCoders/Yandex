package yandex.cloud.team.integration.idm;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.team.integration.abc.CreateCloudServiceFacade;

public class FakeCreateCloudServiceFacade implements CreateCloudServiceFacade {

    @Override
    public @NotNull Operation createByAbcServiceId(
            long abcServiceId
    ) {
        throw new UnsupportedOperationException("CreateCloudService.createByAbcServiceId not implemented");
    }

    @Override
    public @NotNull Operation createByAbcServiceSlug(
            @NotNull String abcServiceSlug
    ) {
        throw new UnsupportedOperationException("CreateCloudService.createByAbcServiceSlug not implemented");
    }

    @Override
    public @NotNull Operation createByAbcdFolderId(
            @NotNull String abcdFolderId
    ) {
        throw new UnsupportedOperationException("CreateCloudService.createByAbcdFolderId not implemented");
    }

}
