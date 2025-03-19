package yandex.cloud.ti.abc.repo.ydb;

import java.util.List;

import yandex.cloud.repository.db.Entity;

public class AbcIntegrationEntities {

    @SuppressWarnings("rawtypes")
    public static final List<Class<? extends Entity>> entities = List.of(
            AbcServiceCloudEntityByCloudId.class,
            AbcServiceCloudEntityByAbcServiceId.class,
            AbcServiceCloudEntityByAbcdFolderId.class,
            AbcServiceCloudEntityByAbcServiceSlug.class,
            AbcServiceCreateOperation.class,
            AbcServiceStubOperation.class
    );

}
