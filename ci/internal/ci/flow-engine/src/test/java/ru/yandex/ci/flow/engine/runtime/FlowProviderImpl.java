package ru.yandex.ci.flow.engine.runtime;

import java.nio.file.Path;
import java.util.concurrent.ConcurrentHashMap;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.flow.engine.definition.Flow;

public class FlowProviderImpl implements FlowProvider {
    private final ConcurrentHashMap<FlowFullId, Flow> flows = new ConcurrentHashMap<>();

    @Override
    public FlowFullId register(Flow flow, Path path, String flowId) {
        var flowFullId = FlowFullId.of(path, flowId);
        if (flows.containsKey(flowFullId)) {
            throw new IllegalArgumentException("%s is already registered: old %s, new %s".formatted(
                    flowFullId, flows.get(flowFullId), flow
            ));
        }
        flows.put(flowFullId, flow);
        return flowFullId;
    }

    @Override
    public Flow get(FlowFullId flowFullId) throws AYamlValidationException {
        if (!flows.containsKey(flowFullId)) {
            throw new IllegalStateException("no registered flow %s".formatted(flowFullId));
        }
        return flows.get(flowFullId);
    }

    @Override
    public void clear() {
        flows.clear();
    }
}
