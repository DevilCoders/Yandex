package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.operation.IamAbstractOperationMethod;
import yandex.cloud.task.TaskProcessor;
import yandex.cloud.task.model.Task;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;

public class CreateCloudOperationMethod extends IamAbstractOperationMethod {

    private final @NotNull TaskProcessor taskProcessor;
    private final @NotNull TeamAbcService abcService;
    private final @NotNull TeamAbcdFolder abcdFolder;


    public CreateCloudOperationMethod(
            @NotNull TaskProcessor taskProcessor,
            @NotNull TeamAbcService abcService,
            @NotNull TeamAbcdFolder abcdFolder
    ) {
        this.taskProcessor = taskProcessor;
        this.abcService = abcService;
        this.abcdFolder = abcdFolder;
    }


    @Override
    protected void runOperation() {
        getOperationContext().update(op -> op
                .withMetadata(new CreateCloudMetadata(abcService.id(), abcService.slug(), abcdFolder.id())));

        taskProcessor.schedule(Task.create(new CreateCloudJob(abcService, abcdFolder)));
    }

    @Override
    protected String getOperationDescription() {
        return "Create cloud for ABC service";
    }

}
