package yandex.cloud.ti.abc.repo;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.AbcServiceCloudCreateOperationReference;
import yandex.cloud.ti.abc.AbcServiceCloudStubOperationReference;

public class InMemoryAbcIntegrationRepository implements AbcIntegrationRepository {

    private final @NotNull List<AbcServiceCloud> abcServiceClouds;
    private final @NotNull Map<Long, AbcServiceCloudCreateOperationReference> abcServiceCloudCreateOperationReferences;
    private final @NotNull Map<String, AbcServiceCloudStubOperationReference> abcServiceCloudStubOperationReferences;


    public InMemoryAbcIntegrationRepository() {
        abcServiceClouds = new CopyOnWriteArrayList<>();
        abcServiceCloudCreateOperationReferences = new ConcurrentHashMap<>();
        abcServiceCloudStubOperationReferences = new ConcurrentHashMap<>();
    }


    public void clear() {
        abcServiceClouds.clear();
        abcServiceCloudCreateOperationReferences.clear();
        abcServiceCloudStubOperationReferences.clear();
    }


    @Override
    public @NotNull AbcServiceCloud createAbcServiceCloud(@NotNull AbcServiceCloud abcServiceCloud) {
        abcServiceClouds.add(abcServiceCloud);
        return abcServiceCloud;
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByCloudId(@NotNull String cloudId) {
        for (AbcServiceCloud abcServiceCloud : abcServiceClouds) {
            if (abcServiceCloud.cloudId().equals(cloudId)) {
                return abcServiceCloud;
            }
        }
        return null;
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceId(long abcServiceId) {
        for (AbcServiceCloud abcServiceCloud : abcServiceClouds) {
            if (abcServiceCloud.abcServiceId() == abcServiceId) {
                return abcServiceCloud;
            }
        }
        return null;
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcServiceSlug(@NotNull String abcServiceSlug) {
        for (AbcServiceCloud abcServiceCloud : abcServiceClouds) {
            if (abcServiceCloud.abcServiceSlug().equals(abcServiceSlug)) {
                return abcServiceCloud;
            }
        }
        return null;
    }

    @Override
    public @Nullable AbcServiceCloud findAbcServiceCloudByAbcdFolderId(@NotNull String abcdFolderId) {
        for (AbcServiceCloud abcServiceCloud : abcServiceClouds) {
            if (abcServiceCloud.abcdFolderId().equals(abcdFolderId)) {
                return abcServiceCloud;
            }
        }
        return null;
    }

    @Override
    public @NotNull ListPage<AbcServiceCloud> listAbcServiceClouds(long pageSize, @Nullable String pageToken) {
        int i = 0;
        if (pageToken != null) {
            while (i < abcServiceClouds.size() && !abcServiceClouds.get(i).cloudId().equals(pageToken)) {
                i++;
            }
        }
        int remaining = Math.toIntExact(pageSize);
        List<AbcServiceCloud> result = new ArrayList<>();
        while (i < abcServiceClouds.size() && remaining > 0) {
            result.add(abcServiceClouds.get(i));
            i++;
            remaining--;
        }
        return new ListPage<>(
                result,
                i < abcServiceClouds.size() ? abcServiceClouds.get(i).cloudId() : null
        );
    }

    @Override
    public @NotNull AbcServiceCloudCreateOperationReference saveCreateOperation(long abcServiceId, Operation.@NotNull Id createOperationId) {
        AbcServiceCloudCreateOperationReference ref = new AbcServiceCloudCreateOperationReference(abcServiceId, createOperationId);
        abcServiceCloudCreateOperationReferences.put(abcServiceId, ref);
        return ref;
    }

    @Override
    public @Nullable AbcServiceCloudCreateOperationReference findCreateOperationByAbcServiceId(long abcServiceId) {
        return abcServiceCloudCreateOperationReferences.get(abcServiceId);
    }

    @Override
    public @NotNull AbcServiceCloudStubOperationReference saveStubOperation(Operation.@NotNull Id stubOperationId, Operation.@NotNull Id createOperationId) {
        AbcServiceCloudStubOperationReference ref = new AbcServiceCloudStubOperationReference(stubOperationId, createOperationId);
        abcServiceCloudStubOperationReferences.put(stubOperationId.getValue(), ref);
        return ref;
    }

    @Override
    public @Nullable AbcServiceCloudStubOperationReference findStubOperationByOperationId(Operation.@NotNull Id stubOperationId) {
        return abcServiceCloudStubOperationReferences.get(stubOperationId.getValue());
    }

}
