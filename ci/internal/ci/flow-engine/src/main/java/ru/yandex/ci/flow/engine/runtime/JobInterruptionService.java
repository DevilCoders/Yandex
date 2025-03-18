package ru.yandex.ci.flow.engine.runtime;

import java.nio.charset.StandardCharsets;

import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.recipes.shared.EphemeralSharedValue;

import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.InterruptJobCommand;
import ru.yandex.ci.flow.zookeeper.CuratorFactory;

@Slf4j
public class JobInterruptionService {
    private final CuratorFactory factory;

    public JobInterruptionService(CuratorFactory factory) {
        this.factory = factory;
    }

    public void notifyExecutor(InterruptJobCommand command) {
        notifyExecutor(command.getJobLaunchId(), command.getInterruptMethod());
    }

    public void notifyExecutor(FullJobLaunchId fullJobLaunchId, InterruptMethod interruptMethod) {
        log.info("Interrupting executor, job launch id: {}, method: {}", fullJobLaunchId, interruptMethod);
        String fullJobId = JobUtils.getFullJobId(
            fullJobLaunchId.getFlowLaunchId(),
            fullJobLaunchId.getJobId(),
            fullJobLaunchId.getJobLaunchNumber()
        );

        EphemeralSharedValue node = factory.createSharedValue(SyncJobExecutor.getInterruptNodePath(fullJobId));
        try {
            node.start();
            try (node) {
                node.setValue(interruptMethod.name().getBytes(StandardCharsets.UTF_8));
            }
        } catch (Exception e) {
            if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            }

            throw new RuntimeException(
                "Exception occurred when tried to interrupt job " + fullJobLaunchId, e
            );
        }
    }
}
