package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Duration;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.flow.engine.runtime.events.StageGroupChangeEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class StageRecalcTask extends AbstractOnetimeTask<StageRecalcTaskParameters> {

    private FlowStateService flowStateService;
    private Duration timeout;

    public StageRecalcTask(
            FlowStateService flowStateService,
            Duration timeout
    ) {
        super(StageRecalcTaskParameters.class);
        this.flowStateService = flowStateService;
        this.timeout = timeout;
    }

    public StageRecalcTask(StageRecalcTaskParameters params) {
        super(params);
    }

    @Override
    protected void execute(StageRecalcTaskParameters params, ExecutionContext context) throws Exception {
        log.info("Try to recalc stages for: {}", params);
        flowStateService.recalc(params.getFlowLaunchId(), StageGroupChangeEvent.INSTANCE);
    }

    @Override
    public Duration getTimeout() {
        return timeout;
    }

}
