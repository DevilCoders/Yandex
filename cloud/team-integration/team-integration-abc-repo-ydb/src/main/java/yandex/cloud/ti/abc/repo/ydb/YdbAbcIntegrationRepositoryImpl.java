package yandex.cloud.ti.abc.repo.ydb;

import java.time.Instant;
import java.util.Locale;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.db.AbstractTable;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.list.ListRequest;
import yandex.cloud.repository.db.list.ListResult;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.AbcServiceCloudCreateOperationReference;
import yandex.cloud.ti.abc.AbcServiceCloudStubOperationReference;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.ListPage;

class YdbAbcIntegrationRepositoryImpl implements AbcIntegrationRepository {

    private static final @NotNull ListPager<AbcServiceCloudEntityByCloudId> pager = new ListPager<>(AbcServiceCloudEntityByCloudId.class);


    private <T extends Entity<T>> AbstractTable<T> table(Class<T> entity) {
        RepositoryTransaction transaction = Tx.Current.get().getRepositoryTransaction();
        return (AbstractTable<T>) transaction.table(entity);
    }


    @Override
    public @NotNull AbcServiceCloud createAbcServiceCloud(
            @NotNull AbcServiceCloud abcServiceCloud
    ) {
        return toAbcServiceCloud(table(AbcServiceCloudEntityByCloudId.class)
                .save(new AbcServiceCloudEntityByCloudId(
                        AbcServiceCloudEntityByCloudId.Id.of(
                                abcServiceCloud.cloudId()
                        ),
                        abcServiceCloud.defaultFolderId(),
                        abcServiceCloud.abcServiceId(),
                        abcServiceCloud.abcServiceSlug().toLowerCase(Locale.ROOT),
                        abcServiceCloud.abcdFolderId(),
                        currentTransactionTime()
                ))
        );
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByCloudId(
            @NotNull String cloudId
    ) {
        return toAbcServiceCloud(table(AbcServiceCloudEntityByCloudId.class)
                .find(AbcServiceCloudEntityByCloudId.Id.of(cloudId))
        );
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceId(
            long abcServiceId
    ) {
        return toAbcServiceCloud(table(AbcServiceCloudEntityByAbcServiceId.class)
                .find(AbcServiceCloudEntityByAbcServiceId.Id.of(abcServiceId))
        );
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceSlug(
            @NotNull String abcServiceSlug
    ) {
        AbcServiceCloud abcServiceCloud = toAbcServiceCloud(table(AbcServiceCloudEntityByAbcServiceSlug.class)
                .find(AbcServiceCloudEntityByAbcServiceSlug.Id.of(abcServiceSlug.toLowerCase(Locale.ROOT)))
        );
        if (abcServiceCloud == null) {
            // fallback to search by the exact slug value given in the request
            // https://st.yandex-team.ru/CLOUD-100436
            abcServiceCloud = toAbcServiceCloud(table(AbcServiceCloudEntityByAbcServiceSlug.class)
                    .find(AbcServiceCloudEntityByAbcServiceSlug.Id.of(abcServiceSlug))
            );
        }
        return abcServiceCloud;
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcdFolderId(
            @NotNull String abcdFolderId
    ) {
        return toAbcServiceCloud(table(AbcServiceCloudEntityByAbcdFolderId.class)
                .find(AbcServiceCloudEntityByAbcdFolderId.Id.of(abcdFolderId))
        );
    }

    @Override
    public @NotNull ListPage<AbcServiceCloud> listAbcServiceClouds(
            long pageSize,
            @Nullable String pageToken
    ) {
        ListRequest<AbcServiceCloudEntityByCloudId> request = pager.createListRequest(pageSize, pageToken);
        ListResult<AbcServiceCloudEntityByCloudId> result = table(AbcServiceCloudEntityByCloudId.class)
                .list(request);
        return pager.processListResult(result)
                .map(YdbAbcIntegrationRepositoryImpl::toAbcServiceCloud);
    }


    @Override
    public @Nullable AbcServiceCloudCreateOperationReference findCreateOperationByAbcServiceId(
            long abcServiceId
    ) {
        return toAbcServiceCloudCreateOperationReference(table(AbcServiceCreateOperation.class)
                .find(AbcServiceCreateOperation.Id.of(abcServiceId))
        );
    }

    @Override
    public @NotNull AbcServiceCloudCreateOperationReference saveCreateOperation(
            long abcServiceId,
            @NotNull Operation.Id createOperationId
    ) {
        return toAbcServiceCloudCreateOperationReference(table(AbcServiceCreateOperation.class)
                .save(new AbcServiceCreateOperation(
                        AbcServiceCreateOperation.Id.of(abcServiceId),
                        createOperationId
                ))
        );
    }

    @Override
    public @NotNull AbcServiceCloudStubOperationReference saveStubOperation(
            @NotNull Operation.Id stubOperationId,
            @NotNull Operation.Id createOperationId
    ) {
        return toAbcServiceCloudStubOperationReference(table(AbcServiceStubOperation.class)
                .save(new AbcServiceStubOperation(
                        AbcServiceStubOperation.Id.of(stubOperationId),
                        createOperationId
                ))
        );
    }

    @Override
    public @Nullable AbcServiceCloudStubOperationReference findStubOperationByOperationId(
            @NotNull Operation.Id stubOperationId
    ) {
        return toAbcServiceCloudStubOperationReference(table(AbcServiceStubOperation.class)
                .find(AbcServiceStubOperation.Id.of(stubOperationId))
        );
    }


    private @NotNull Instant currentTransactionTime() {
        // todo inject Clock and use it instead of static call
        //  or get the current transaction time
        //  see yandex.cloud.iam.repository.TransactionTime
        //  see yandex.cloud.iam.repository.TransactionTimeImpl
        return Instant.now();
    }


    private static @Nullable AbcServiceCloud toAbcServiceCloud(
            @Nullable AbcServiceCloudEntity entity
    ) {
        if (entity == null) {
            return null;
        }
        return new AbcServiceCloud(
                entity.getAbcId(),
                entity.getAbcSlug(),
                entity.getAbcFolderId(),
                entity.getCloudId(),
                entity.getDefaultFolderId()
        );
    }

    private static @Nullable AbcServiceCloudCreateOperationReference toAbcServiceCloudCreateOperationReference(
            @Nullable AbcServiceCloudCreateOperationReferenceEntity entity
    ) {
        if (entity == null) {
            return null;
        }
        return new AbcServiceCloudCreateOperationReference(
                entity.getAbcServiceId(),
                entity.getOperationId()
        );
    }

    private static @Nullable AbcServiceCloudStubOperationReference toAbcServiceCloudStubOperationReference(
            @Nullable AbcServiceCloudStubOperationReferenceEntity entity
    ) {
        if (entity == null) {
            return null;
        }
        return new AbcServiceCloudStubOperationReference(
                entity.getStubOperationId(),
                entity.getMainOperationId()
        );
    }

}
