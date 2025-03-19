package yandex.cloud.ti.abc.repo;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.AbcServiceCloudCreateOperationReference;
import yandex.cloud.ti.abc.AbcServiceCloudStubOperationReference;

public interface AbcIntegrationRepository {

    @NotNull AbcServiceCloud createAbcServiceCloud(
            @NotNull AbcServiceCloud abcServiceCloud
    );

    @Nullable AbcServiceCloud findAbcServiceCloudByCloudId(
            @NotNull String cloudId
    );

    @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceId(
            long abcServiceId
    );

    @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceSlug(
            @NotNull String abcServiceSlug
    );

    @Nullable AbcServiceCloud findAbcServiceCloudByAbcdFolderId(
            @NotNull String abcdFolderId
    );

    @NotNull ListPage<AbcServiceCloud> listAbcServiceClouds(
            long pageSize,
            @Nullable String pageToken
    );


    @NotNull AbcServiceCloudCreateOperationReference saveCreateOperation(
            long abcServiceId,
            @NotNull Operation.Id createOperationId
    );

    @Nullable AbcServiceCloudCreateOperationReference findCreateOperationByAbcServiceId(
            long abcServiceId
    );


    @NotNull AbcServiceCloudStubOperationReference saveStubOperation(
            @NotNull Operation.Id stubOperationId,
            @NotNull Operation.Id createOperationId
    );

    @Nullable AbcServiceCloudStubOperationReference findStubOperationByOperationId(
            @NotNull Operation.Id stubOperationId
    );

}
