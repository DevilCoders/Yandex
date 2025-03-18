package ru.yandex.ci.flow.engine.runtime;

import java.nio.file.Path;
import java.util.UUID;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.flow.engine.definition.Flow;

public interface FlowProvider {
    default FlowFullId register(Flow flow, Path path) {
        return register(flow, path, UUID.randomUUID().toString());
    }

    default FlowFullId register(Flow flow, FlowFullId flowFullId) {
        return register(flow, flowFullId.getPath(), flowFullId.getId());
    }

    FlowFullId register(Flow flow, Path path, String flowId);

    Flow get(FlowFullId flowId) throws AYamlValidationException;

    void clear();
}
