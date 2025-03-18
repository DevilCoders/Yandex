package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.Collections;

import javax.annotation.Nonnull;

import com.google.gson.JsonObject;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceId;
import ru.yandex.ci.util.gson.CiGson;

/**
 * Минимальная информация о запуске флоу.
 */
public interface FlowLaunchRef {
    FlowLaunchId getFlowLaunchId();

    FlowFullId getFlowFullId();

    default StoredResource fromResource(@Nonnull Resource resource) {
        StoredResourceId id = StoredResourceId.generate();
        JsonObject object = CiGson.instance().toJsonTree(resource).getAsJsonObject();
        return new StoredResource(
                id,
                getFlowFullId().asString(),
                getFlowLaunchId(),
                resource.getSourceCodeId(),
                object,
                resource.getResourceType(),
                Collections.emptyList()
        );
    }
}
