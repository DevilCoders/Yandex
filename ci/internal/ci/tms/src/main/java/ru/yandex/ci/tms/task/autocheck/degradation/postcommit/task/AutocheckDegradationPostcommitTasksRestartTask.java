package ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task;

import java.util.List;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class AutocheckDegradationPostcommitTasksRestartTask extends AutocheckDegradationPostcommitTasksBaseTask {
    private BazingaTaskManager bazingaTaskManager;

    public AutocheckDegradationPostcommitTasksRestartTask(
            boolean dryRun,
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager) {
        super(dryRun, sandboxClient);

        this.bazingaTaskManager = bazingaTaskManager;
    }

    public AutocheckDegradationPostcommitTasksRestartTask(Parameters parameters) {
        super(parameters);
    }

    @Override
    protected void execute(Parameters params, ExecutionContext context) throws Exception {
        executeTaskAction(params.getSandboxTasksIds(), SandboxBatchAction.STOP);

        bazingaTaskManager.schedule(new AutocheckDegradationPostcommitTasksStartTask(params));
    }

    @Override
    protected List<SandboxTaskStatus> getPreProcessedStatuses() {
        return List.of(
                SandboxTaskStatus.ASSIGNED,
                SandboxTaskStatus.EXECUTING,
                SandboxTaskStatus.WAIT_TASK,
                SandboxTaskStatus.PREPARING
        );
    }
}
