package yandex.cloud.team.integration.abc;

import java.beans.ConstructorProperties;
import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.operation.job.BaseOperationJob;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;

public class CreateCloudJob extends BaseOperationJob<CreateCloudProcessState> {

    @Serial
    private static final long serialVersionUID = 7761533570372184898L;

    private final @NotNull TeamAbcService abcService;
    private final @NotNull TeamAbcdFolder abcdFolder;

    @ConstructorProperties({"abcService", "abcdFolder"})
    public CreateCloudJob(
            @NotNull TeamAbcService abcService,
            @NotNull TeamAbcdFolder abcdFolder
    ) {
        super(new CreateCloudProcessState());
        this.abcService = abcService;
        this.abcdFolder = abcdFolder;
    }

    @Override
    protected @NotNull CreateCloudProcess createProcess(
            @NotNull CreateCloudProcessState processState
    ) {
        return new CreateCloudProcess(processState, abcService, abcdFolder);
    }

}
