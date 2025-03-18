package ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task;

import java.util.List;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class AutocheckDegradationPostcommitTasksStartTask extends AutocheckDegradationPostcommitTasksBaseTask {

    public AutocheckDegradationPostcommitTasksStartTask(
            boolean dryRun,
            SandboxClient sandboxClient) {
        super(dryRun, sandboxClient);
    }

    public AutocheckDegradationPostcommitTasksStartTask(Parameters parameters) {
        super(parameters);
    }

    @Override
    protected void execute(Parameters params, ExecutionContext context) throws Exception {
        executeTaskAction(params.getSandboxTasksIds(), SandboxBatchAction.START);
    }

    @Override
    protected List<SandboxTaskStatus> getPreProcessedStatuses() {
        return List.of(SandboxTaskStatus.STOPPING, SandboxTaskStatus.STOPPED);
    }
}
