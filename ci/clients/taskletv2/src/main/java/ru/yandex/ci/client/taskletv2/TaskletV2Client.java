package ru.yandex.ci.client.taskletv2;

import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaRequest;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelResponse;

public interface TaskletV2Client extends AutoCloseable {

    GetLabelResponse getLabel(GetLabelRequest request);

    GetBuildResponse getBuild(GetBuildRequest request);

    GetSchemaResponse getSchema(GetSchemaRequest request);

    //

    AuthenticatedExecutor getExecutor(String oauthToken);

    interface AuthenticatedExecutor {
        ExecuteResponse execute(ExecuteRequest request);

        GetExecutionResponse getExecution(GetExecutionRequest request);
    }
}
