package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;

/**
 * Информация о запуске флоу. Один из юзкейсов, когда этот класс полезен: инстанса {@link FlowLaunchEntity} ещё нет,
 * однако уже нужно сохранить ресурсы, которые будут привязаны к этому запуску (в ресурсах есть ссылка на запуск).
 */
public class FlowLaunchRefImpl implements FlowLaunchRef {
    private final FlowLaunchId id;
    private final FlowFullId flowId;

    @JsonCreator
    public FlowLaunchRefImpl(FlowLaunchId id, FlowFullId flowId) {
        this.id = id;
        this.flowId = flowId;
    }

    @VisibleForTesting
    public static FlowLaunchRef create(FlowFullId flowId) {
        Preconditions.checkNotNull(flowId);
        return new FlowLaunchRefImpl(FlowLaunchId.of(UUID.randomUUID().toString()), flowId);
    }

    public static FlowLaunchRef create(FlowLaunchId flowLaunchId, FlowFullId flowFullId) {
        Preconditions.checkNotNull(flowLaunchId);
        Preconditions.checkNotNull(flowFullId);

        return new FlowLaunchRefImpl(flowLaunchId, flowFullId);
    }

    @Override
    public FlowLaunchId getFlowLaunchId() {
        return id;
    }

    @Override
    public FlowFullId getFlowFullId() {
        return flowId;
    }
}
